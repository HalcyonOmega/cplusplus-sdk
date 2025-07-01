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

void MCPProtocol::SetupTransportRouters() {
    if (!m_Transport) { throw std::invalid_argument("Transport cannot be null"); }

    // Set up transport handlers
    m_Transport->SetMessageRouter(
        [this](const JSONValue& InMessage) { m_MessageManager->RouteMessage(InMessage); });
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