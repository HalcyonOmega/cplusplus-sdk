#include "CoreSDK/Core/IMCP.h"

#include "CoreSDK/Common/EventSignatures.h"
#include "CoreSDK/Messages/NotificationBase.h"

MCP_NAMESPACE_BEGIN

MCPProtocol::MCPProtocol(std::unique_ptr<ITransport> InTransport)
    : m_Transport(std::move(InTransport)), m_RequestManager(std::make_unique<RequestManager>(true)),
      m_ResponseManager(std::make_unique<ResponseManager>(true)),
      m_NotificationManager(std::make_unique<NotificationManager>(true)),
      m_ErrorManager(std::make_unique<ErrorManager>(true)) {
    SetupTransportRouters();
}

bool MCPProtocol::IsInitialized() const {
    return m_State == MCPProtocolState::Initialized;
}

MCPProtocolState MCPProtocol::GetState() const {
    return m_State;
}

void MCPProtocol::SetState(MCPProtocolState InNewState) {
    m_State = InNewState;
}

ITransport* MCPProtocol::GetTransport() const {
    return m_Transport.get();
}

bool MCPProtocol::IsConnected() const {
    return m_Transport->IsConnected();
}

void MCPProtocol::RegisterRequestHandler(const RequestBase& InRequest) {
    if (InRequest.Handler.has_value()) {
        m_RequestManager->RegisterRequestHandler(
            InRequest.Method, [handler = InRequest.Handler.value()](
                                  const RequestBase& InReq, MCPContext* InCtx) { handler(InReq); });
    }
}

void MCPProtocol::RegisterResponseHandler(const ResponseBase& InResponse) {
    if (InResponse.Handler.has_value()) {
        m_ResponseManager->RegisterPendingRequest(
            InResponse.ID, [handler = InResponse.Handler.value()](
                               const ResponseBase& InResp, MCPContext* InCtx) { handler(InResp); });
    }
}

void MCPProtocol::RegisterNotificationHandler(const NotificationBase& InNotification) {
    if (InNotification.Handler.has_value()) {
        m_NotificationManager->RegisterNotificationHandler(
            InNotification.Method,
            [handler = InNotification.Handler.value()](const NotificationBase& InNotif,
                                                       MCPContext* InCtx) { handler(InNotif); });
    }
}

void MCPProtocol::RegisterErrorResponseHandler(const ErrorResponseBase& InErrorResponse) {
    if (InErrorResponse.Handler.has_value()) {
        m_ErrorManager->RegisterRequestErrorHandler(
            InErrorResponse.ID,
            [handler = InErrorResponse.Handler.value()](const ErrorResponseBase& InErr,
                                                        MCPContext* InCtx) { handler(InErr); });
    }
}

MCPTask_Void MCPProtocol::SendRequest(const RequestBase& InRequest) {
    if (!m_Transport->IsConnected()) {
        HandleRuntimeError("Transport not connected");
        co_return;
    }

    // Create promise for response
    auto pendingResponse = std::make_unique<PendingResponse>();
    pendingResponse->RequestID = InRequest.ID;
    pendingResponse->StartTime = std::chrono::steady_clock::now();

    auto future = pendingResponse->Promise.get_future();
    std::string requestIDStr = InRequest.ID.ToString();

    {
        std::lock_guard<std::mutex> lock(m_ResponsesMutex);
        m_PendingResponses[requestIDStr] = std::move(pendingResponse);
    }

    try {
        // Send request via transport
        JSONValue requestJSON;
        to_json(requestJSON, InRequest);
        co_await m_Transport->TransmitMessage(requestJSON);

    } catch (const std::exception&) {
        // Remove from pending requests if send failed
        std::lock_guard<std::mutex> lock(m_ResponsesMutex);
        m_PendingResponses.erase(requestIDStr);
        throw;
    }

    co_return;
}

MCPTask_Void MCPProtocol::SendResponse(const ResponseBase& InResponse) {
    JSONValue responseJSON;
    to_json(responseJSON, InResponse);
    co_await m_Transport->TransmitMessage(responseJSON);
}

MCPTask_Void MCPProtocol::SendNotification(const NotificationBase& InNotification) {
    JSONValue notificationJSON;
    to_json(notificationJSON, InNotification);
    co_await m_Transport->TransmitMessage(notificationJSON);
}

MCPTask_Void MCPProtocol::SendErrorResponse(const ErrorResponseBase& InErrorResponse) {
    JSONValue errorJSON;
    to_json(errorJSON, InErrorResponse);
    co_await m_Transport->TransmitMessage(errorJSON);
}

void MCPProtocol::RouteRequest(const RequestBase& InRequest) {
    try {
        m_RequestManager->RouteRequest(InRequest, nullptr);
    } catch (const std::exception& e) {
        // Send internal error response
        ErrorResponseBase errorResponse(
            InRequest.ID, MCPError{ErrorCodes::INTERNAL_ERROR,
                                   "Internal error: " + std::string(e.what()), std::nullopt});
        SendErrorResponse(errorResponse);
        HandleRuntimeError("Error handling request: " + std::string(e.what()));
    }
}

void MCPProtocol::RouteResponse(const ResponseBase& InResponse) {
    try {
        // First handle any registered response handlers
        m_ResponseManager->RouteResponse(InResponse, nullptr);

        // Then handle pending request promises
        std::string requestID = InResponse.ID.ToString();
        std::lock_guard<std::mutex> lock(m_ResponsesMutex);
        auto it = m_PendingResponses.find(requestID);
        if (it != m_PendingResponses.end()) {
            // Convert ResultParams to JSONValue for the promise
            JSONValue resultJSON;
            to_json(resultJSON, InResponse.Result);
            it->second->Promise.set_value(resultJSON);
            m_PendingResponses.erase(it);
        }
    } catch (const std::exception& e) {
        HandleRuntimeError("Error handling response: " + std::string(e.what()));
    }
}

void MCPProtocol::RouteNotification(const NotificationBase& InNotification) {
    try {
        // Route notification to registered handlers
        // Notifications without handlers are simply ignored per JSON-RPC spec
        m_NotificationManager->RouteNotification(InNotification, nullptr);
    } catch (const std::exception& e) {
        HandleRuntimeError("Error handling notification: " + std::string(e.what()));
    }
}

void MCPProtocol::RouteErrorResponse(const ErrorResponseBase& InError) {
    try {
        // First handle any registered error handlers
        m_ErrorManager->RouteError(InError, nullptr);

        // Always handle pending request promises for built-in functionality
        std::string requestID = InError.ID.ToString();
        std::lock_guard<std::mutex> lock(m_ResponsesMutex);
        auto Iter = m_PendingResponses.find(requestID);
        if (Iter != m_PendingResponses.end()) {
            std::string errorMessage = "Error response: " + InError.Error.Message;
            Iter->second->Promise.set_exception(
                std::make_exception_ptr(std::runtime_error(errorMessage)));
            m_PendingResponses.erase(Iter);
        }
    } catch (const std::exception& e) {
        HandleRuntimeError("Error handling error response: " + std::string(e.what()));
    }
}

void MCPProtocol::SetupTransportRouters() {
    if (!m_Transport) { throw std::invalid_argument("Transport cannot be null"); }

    // Set up transport handlers
    m_Transport->SetRequestRouter(
        [this](const RequestBase& InRequest) { RouteRequest(InRequest); });
    m_Transport->SetResponseRouter(
        [this](const ResponseBase& InResponse) { RouteResponse(InResponse); });
    m_Transport->SetNotificationRouter(
        [this](const NotificationBase& InNotification) { RouteNotification(InNotification); });
    m_Transport->SetErrorResponseRouter(
        [this](const ErrorResponseBase& InError) { RouteErrorResponse(InError); });
}

MCP_NAMESPACE_END