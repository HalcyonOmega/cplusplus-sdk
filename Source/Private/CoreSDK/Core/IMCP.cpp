#include "CoreSDK/Core/IMCP.h"

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

void MCPProtocol::SetNotificationHandler(std::string_view InMethod, NotificationHandler InHandler) {
    std::lock_guard<std::mutex> lock(m_HandlersMutex);
    m_NotificationHandlers[InMethod] = InHandler;
}

void MCPProtocol::SetInitializedHandler(InitializedHandler InHandler) {
    m_InitializedHandler = InHandler;
}

void MCPProtocol::SetErrorResponseHandler(ErrorResponseHandler InHandler) {
    m_ErrorResponseHandler = InHandler;
}

const std::optional<MCPCapabilities>& MCPProtocol::GetClientCapabilities() const {
    return m_ClientCapabilities;
}

const std::optional<MCPServerInfo>& MCPProtocol::GetServerInfo() const {
    return m_ServerInfo;
}

MCPTask_Void MCPProtocol::SendRequest(const RequestBase& InRequest) {
    if (!m_Transport->IsConnected()) { HandleRuntimeError("Transport not connected"); }

    // Create promise for response
    auto pendingResponse = std::make_unique<PendingResponse>();
    pendingResponse->RequestID = InRequest.ID;
    pendingResponse->StartTime = std::chrono::steady_clock::now();

    auto future = pendingResponse->Promise.get_future();

    {
        std::lock_guard<std::mutex> lock(m_ResponsesMutex);
        m_PendingResponses[InRequest.ID] = std::move(pendingResponse);
    }

    try {
        // Send request via transport
        co_await m_Transport->TransmitMessage(InRequest);

        // Parse response
        auto response = JSONValue::parse(responseStr);
        co_return response;

    } catch (const std::exception&) {
        // Remove from pending requests if send failed
        std::lock_guard<std::mutex> lock(m_RequestsMutex);
        m_PendingResponses.erase(InRequest.ID);
    }

    // TODO: @HalcyonOmega - BEGIN TIMEOUT IMPLEMENTATION

    // Send the request
    co_await m_Transport->TransmitMessage(InRequest);

    // Wait for response with timeout
    static constexpr std::chrono::seconds DEFAULT_RESPONSE_WAIT_FOR{30};
    auto status = future.wait_for(std::chrono::seconds(DEFAULT_RESPONSE_WAIT_FOR));
    if (status == std::future_status::timeout) {
        std::lock_guard<std::mutex> lock(m_RequestsMutex);
        m_PendingRequests.erase(requestID);
        HandleRuntimeError("Request timeout");
        co_return;
    }

    co_return future.get();
    // TODO: @HalcyonOmega - END TIMEOUT IMPLEMENTATION
}

MCPTask_Void MCPProtocol::SendResponse(const ResponseBase& InResponse) {
    co_await m_Transport->TransmitMessage(InResponse);
}

MCPTask_Void MCPProtocol::SendNotification(const NotificationBase& InNotification) {
    co_await m_Transport->TransmitMessage(InNotification);
}

MCPTask_Void MCPProtocol::SendErrorResponse(const ErrorResponseBase& InErrorResponse) {
    co_await m_Transport->TransmitMessage(InErrorResponse);
}

void MCPProtocol::HandleRequest(const RequestBase& InRequest) {
    try {
        std::lock_guard<std::mutex> lock(m_HandlersMutex);
        auto handler = m_RequestHandlers.find(InRequest.Method);
        if (handler != m_RequestHandlers.end()) {
            handler->second(InRequest);
        } else {
            // Send method not found error
            SendErrorResponse(InRequestID, -32601, "Method not found", JSONValue::object());
        }
    } catch (const std::exception& e) {
        // Send internal error
        SendErrorResponse(InRequestID, -32603, "Internal error",
                          JSONValue::object({{"details", e.what()}}));
    }
}

void MCPProtocol::HandleResponse(const ResponseBase& InResponse) {
    try {
        if (!InResponse.ID.has_value()) {
            return; // Invalid response without ID
        }

        std::string requestID = InResponse.ID.value();

        std::lock_guard<std::mutex> lock(m_RequestsMutex);
        auto it = m_PendingRequests.find(requestID);
        if (it != m_PendingRequests.end()) {
            if (InResponse.Result.has_value()) {
                it->second->Promise.set_value(InResponse.Result.value());
            } else if (InResponse.Error.has_value()) {
                auto error = InResponse.Error.value();
                std::string errorMsg = error["message"].get<std::string>();
                it->second->Promise.set_exception(
                    std::make_exception_ptr(std::runtime_error(errorMsg)));
            }
            m_PendingRequests.erase(it);
        }
    } catch (const std::exception& e) {
        HandleRuntimeError("Error handling response: " + std::string(e.what()));
    }
}

void MCPProtocol::HandleNotification(const NotificationBase& InNotification) {
    try {
        std::lock_guard<std::mutex> lock(m_HandlersMutex);
        auto handler = m_NotificationHandlers.find(InNotification.Method);
        if (handler != m_NotificationHandlers.end()) { handler->second(InNotification); }
        // Notifications without handlers are simply ignored
    } catch (const std::exception& e) {
        HandleRuntimeError("Error handling notification: " + std::string(e.what()));
    }
}

void MCPProtocol::HandleErrorResponse(const ErrorResponseBase& InError) {
    HandleRuntimeError("Transport error: " + InError.dump());
}

void MCPProtocol::SetupTransportHandlers() {
    if (!m_Transport) { throw std::invalid_argument("Transport cannot be null"); }

    // Set up transport handlers
    // TODO: @HalcyonOmega Update callbacks based on sync/async requirements. Below don't match
    // signatures
    m_Transport->SetMessageHandler(HandleMessage);
    m_Transport->SetRequestHandler(HandleRequest);
    m_Transport->SetResponseHandler(HandleResponse);
    m_Transport->SetNotificationHandler(HandleNotification);
    m_Transport->SetErrorResponseHandler(HandleErrorResponse);
    m_Transport->SetStateChangeHandler(HandleTransportStateChange);

    // Preserve alternate lambda callbacks for reference if advanced functionality needed in future.
    // m_Transport->SetRequestHandler(
    //     [](const RequestBase& InRequest) -> MCPTask<const ResponseBase&> {
    //         HandleIncomingRequest(InRequest);
    //     });

    // m_Transport->SetResponseHandler(
    //     [](const ResponseBase& InResponse) -> MCPTask_Void { HandleIncomingResponse(InResponse);
    //     });

    // m_Transport->SetNotificationHandler([](const NotificationBase& InNotification) ->
    // MCPTask_Void {
    //     HandleIncomingNotification(InNotification);
    // });

    // m_Transport->SetErrorResponseHandler(
    //     [](const ErrorResponseBase& InError) -> MCPTask_Void { HandleTransportError(InError); });
}

MCP_NAMESPACE_END