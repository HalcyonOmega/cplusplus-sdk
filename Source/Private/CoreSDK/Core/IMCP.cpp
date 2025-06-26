#include "CoreSDK/Core/IMCP.h"

#include "CoreSDK/Common/EventSignatures.h"
#include "CoreSDK/Messages/NotificationBase.h"

MCP_NAMESPACE_BEGIN

MCPProtocol::MCPProtocol(std::unique_ptr<ITransport> InTransport)
    : m_Transport(std::move(InTransport)) {
    SetupTransportHandlers();
}

MCPProtocol::~MCPProtocol() noexcept {
    if (IsInitialized()) {
        try {
            // TODO: @HalcyonOmega - Verify this is the correct implementation
            Stop().await_resume();
        } catch (...) {
            // Ignore errors during destruction
        }
    }
}

MCPTask_Void MCPProtocol::Start() {
    co_await m_Transport->Connect();
    SetState(MCPProtocolState::Initialized);
}

MCPTask_Void MCPProtocol::Stop() {
    if (!IsInitialized()) { co_return; }

    try {
        // Clear all pending requests
        {
            std::lock_guard<std::mutex> lock(m_ResponsesMutex);
            for (auto& [id, request] : m_PendingResponses) {
                request->Promise.set_exception(
                    std::make_exception_ptr(std::runtime_error("Protocol shutdown")));
            }
            m_PendingResponses.clear();
        }

        // Stop transport
        co_await m_Transport->Disconnect();
        SetState(MCPProtocolState::Shutdown);

    } catch (const std::exception& e) {
        // Log error but don't throw during shutdown
        HandleRuntimeError("Error during shutdown: " + std::string(e.what()));
    }

    co_return;
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
    std::lock_guard<std::mutex> lock(m_HandlersMutex);
    if (InRequest.Handler.has_value()) { m_RegisteredRequests.emplace_back(InRequest); }
}

void MCPProtocol::RegisterResponseHandler(const ResponseBase& InResponse) {
    std::lock_guard<std::mutex> lock(m_HandlersMutex);
    if (InResponse.Handler.has_value()) { m_RegisteredResponses.emplace_back(InResponse); }
}

void MCPProtocol::RegisterNotificationHandler(const NotificationBase& InNotification) {
    std::lock_guard<std::mutex> lock(m_HandlersMutex);
    if (InNotification.Handler.has_value()) {
        m_RegisteredNotifications.emplace_back(InNotification);
    }
}

void MCPProtocol::RegisterErrorResponseHandler(const ErrorResponseBase& InErrorResponse) {
    std::lock_guard<std::mutex> lock(m_HandlersMutex);
    if (InErrorResponse.Handler.has_value()) {
        m_RegisteredErrorResponses.emplace_back(InErrorResponse);
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
        std::lock_guard<std::mutex> lock(m_HandlersMutex);
        for (const auto& registeredRequest : m_RegisteredRequests) {
            if (registeredRequest.Method == InRequest.Method) {
                registeredRequest.Handler.value()(InRequest);
                return;
            }
        }
        // Send method not found error
        // TODO: Implement error response creation
    } catch (const std::exception& e) {
        // TODO: Send internal error
        HandleRuntimeError("Error handling request: " + std::string(e.what()));
    }
}

void MCPProtocol::RouteResponse(const ResponseBase& InResponse) {
    try {
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
        std::lock_guard<std::mutex> lock(m_HandlersMutex);
        for (const auto& registeredNotification : m_RegisteredNotifications) {
            if (registeredNotification.Method == InNotification.Method) {
                registeredNotification.Handler.value()(InNotification);
                return;
            }
        }
        // Notifications without handlers are simply ignored per JSON-RPC spec
    } catch (const std::exception& e) {
        HandleRuntimeError("Error handling notification: " + std::string(e.what()));
    }
}

void MCPProtocol::RouteErrorResponse(const ErrorResponseBase& InError) {
    try {
        std::string requestID = InError.ID.ToString();

        std::lock_guard<std::mutex> lock(m_ResponsesMutex);
        auto it = m_PendingResponses.find(requestID);
        if (it != m_PendingResponses.end()) {
            std::string errorMessage = "Error response: " + InError.Error.Message;

            it->second->Promise.set_exception(
                std::make_exception_ptr(std::runtime_error(errorMessage)));
            m_PendingResponses.erase(it);
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