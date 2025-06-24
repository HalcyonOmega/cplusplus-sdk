#include "CoreSDK/Core/IMCP.h"

MCP_NAMESPACE_BEGIN

MCPProtocol::MCPProtocol(std::unique_ptr<ITransport> InTransport)
    : m_Transport(std::move(InTransport)) {
    SetupTransportHandlers();
}

MCPProtocol::~MCPProtocol() noexcept {
    if (m_IsInitialized) {
        try {
            Shutdown().GetResult();
        } catch (...) {
            // Ignore errors during destruction
        }
    }
}

MCPTask_Void MCPProtocol::Initialize(const MCPClientInfo& InClientInfo,
                                     const std::optional<MCPServerInfo>& InServerInfo) {
    if (m_IsInitialized) {
        HandleRuntimeError("Protocol already initialized");
        co_return;
    }

    try {
        // Start transport
        co_await m_Transport->Start();

        // Send initialize request
        InitializeRequest initRequest;
        initRequest.ProtocolVersion = PROTOCOL_VERSION;
        initRequest.ClientInfo = InClientInfo;

        if (InServerInfo.has_value()) { initRequest.ServerInfo = InServerInfo.value(); }

        auto responseJson = co_await SendRequest(RequestBase("initialize", initRequest));
        auto response = responseJson.get<InitializeResponse>();

        // Store negotiated capabilities
        m_ClientCapabilities = response.Capabilities;
        m_ServerInfo = response.ServerInfo;

        // Send initialized notification
        co_await SendNotification(NotificationBase("initialized"));

        m_IsInitialized = true;

        if (m_InitializedHandler) { m_InitializedHandler(response); }

    } catch (const std::exception& e) {
        HandleRuntimeError("Failed to initialize protocol: " + std::string(e.what()));
    }

    co_return;
}

MCPTask_Void MCPProtocol::Shutdown() {
    if (!m_IsInitialized) { co_return; }

    try {
        // Clear all pending requests
        {
            std::lock_guard<std::mutex> lock(m_RequestsMutex);
            for (auto& [id, request] : m_PendingRequests) {
                request->Promise.set_exception(
                    std::make_exception_ptr(std::runtime_error("Protocol shutdown")));
            }
            m_PendingRequests.clear();
        }

        // Stop transport
        co_await m_Transport->Stop();

        m_IsInitialized = false;

        if (m_ShutdownHandler) { m_ShutdownHandler(); }

    } catch (const std::exception& e) {
        // Log error but don't throw during shutdown
        HandleRuntimeError("Error during shutdown: " + std::string(e.what()));
    }

    co_return;
}

bool MCPProtocol::IsInitialized() const {
    return m_IsInitialized;
}

void MCPProtocol::SetRequestHandler(std::string_view InMethod, RequestHandler InHandler) {
    std::lock_guard<std::mutex> lock(m_HandlersMutex);
    m_RequestHandlers[InMethod] = InHandler;
}

void MCPProtocol::SetNotificationHandler(std::string_view InMethod, NotificationHandler InHandler) {
    std::lock_guard<std::mutex> lock(m_HandlersMutex);
    m_NotificationHandlers[InMethod] = InHandler;
}

void MCPProtocol::SetInitializedHandler(InitializedHandler InHandler) {
    m_InitializedHandler = InHandler;
}

void MCPProtocol::SetShutdownHandler(ShutdownHandler InHandler) {
    m_ShutdownHandler = InHandler;
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
    // Create promise for response
    auto pendingRequest = std::make_unique<PendingRequest>();
    pendingRequest->Method = InRequest.Method;
    pendingRequest->Params = InRequest.Params;
    pendingRequest->ID = InRequest.ID;
    pendingRequest->StartTime = std::chrono::steady_clock::now();

    auto future = pendingRequest->Promise.get_future();

    {
        std::lock_guard<std::mutex> lock(m_RequestsMutex);
        m_PendingRequests[requestID] = std::move(pendingRequest);
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
        m_PendingRequests.erase(requestID);
        throw;
    }

    // TODO: @HalcyonOmega - BEGIN TIMEOUT IMPLEMENTATION
    if (!IsConnected()) { HandleRuntimeError("Transport not connected"); }

    RequestID requestID = InRequest.ID;

    // Create promise for response
    auto pendingRequest = std::make_unique<PendingRequest>();
    pendingRequest->RequestID = requestID;
    pendingRequest->StartTime = std::chrono::steady_clock::now();

    auto future = pendingRequest->Promise.get_future();

    {
        Poco::Mutex::ScopedLock lock(m_RequestsMutex);
        m_PendingRequests[requestID] = std::move(pendingRequest);
    }

    // Send the request
    co_await m_Transport->TransmitMessage(InRequest);

    // Wait for response with timeout
    static constexpr std::chrono::seconds DEFAULT_RESPONSE_WAIT_FOR{30};
    auto status = future.wait_for(std::chrono::seconds(DEFAULT_RESPONSE_WAIT_FOR));
    if (status == std::future_status::timeout) {
        Poco::Mutex::ScopedLock lock(m_RequestsMutex);
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