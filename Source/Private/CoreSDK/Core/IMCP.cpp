#include "CoreSDK/Core/IMCP.h"

MCP_NAMESPACE_BEGIN

MCPProtocol::MCPProtocol(std::shared_ptr<ITransport> InTransport) : m_Transport(InTransport) {
    if (!m_Transport) { throw std::invalid_argument("Transport cannot be null"); }

    // Set up transport handlers
    m_Transport->SetRequestHandler([this](const RequestBase& InRequest) {
        (void)this;
        HandleIncomingRequest(InRequest);
    });

    m_Transport->SetResponseHandler([this](const ResponseBase& InResponse) {
        (void)this;
        HandleIncomingResponse(InResponse);
    });

    m_Transport->SetNotificationHandler([this](const NotificationBase& InNotification) {
        (void)this;
        HandleIncomingNotification(InNotification);
    });

    m_Transport->SetErrorHandler([this](const ErrorBase& InError) {
        (void)this;
        HandleTransportError(InError);
    });
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
    if (m_IsInitialized) { throw std::runtime_error("Protocol already initialized"); }

    try {
        // Start transport
        co_await m_Transport->Start();

        // Send initialize request
        InitializeRequest initRequest;
        initRequest.ProtocolVersion = PROTOCOL_VERSION;
        initRequest.ClientInfo = InClientInfo;

        if (InServerInfo.has_value()) { initRequest.ServerInfo = InServerInfo.value(); }

        auto responseJson = co_await SendRequestImpl("initialize", JSONValue(initRequest));
        auto response = responseJson.get<InitializeResponse>();

        // Store negotiated capabilities
        m_ClientCapabilities = response.Capabilities;
        m_ServerInfo = response.ServerInfo;

        // Send initialized notification
        co_await SendNotificationImpl("initialized", JSONValue::object());

        m_IsInitialized = true;

        if (m_InitializedHandler) { m_InitializedHandler(response); }

    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to initialize protocol: " + std::string(e.what()));
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
        if (m_ErrorHandler) { m_ErrorHandler("Error during shutdown: " + std::string(e.what())); }
    }

    co_return;
}

bool MCPProtocol::IsInitialized() const {
    return m_IsInitialized;
}

MCPTask<JSONValue> MCPProtocol::SendRequest(std::string_view InMethod, const JSONValue& InParams) {
    if (!m_IsInitialized) { throw std::runtime_error("Protocol not initialized"); }

    co_return co_await SendRequestImpl(InMethod, InParams);
}

MCPTask_Void MCPProtocol::SendResponse(std::string_view InRequestID, const JSONValue& InResult) {
    if (!m_IsInitialized) { throw std::runtime_error("Protocol not initialized"); }

    co_await m_Transport->SendResponse(InRequestID, InResult);
}

MCPTask_Void MCPProtocol::SendErrorResponse(std::string_view InRequestID, int64_t InErrorCode,
                                            std::string_view InErrorMessage,
                                            const JSONValue& InErrorData) {
    if (!m_IsInitialized) { throw std::runtime_error("Protocol not initialized"); }

    co_await m_Transport->SendErrorResponse(InRequestID, InErrorCode, InErrorMessage, InErrorData);
}

MCPTask_Void MCPProtocol::SendNotification(std::string_view InMethod, const JSONValue& InParams) {
    if (!m_IsInitialized) { throw std::runtime_error("Protocol not initialized"); }

    co_await SendNotificationImpl(InMethod, InParams);
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

void MCPProtocol::SetErrorHandler(ErrorHandler InHandler) {
    m_ErrorHandler = InHandler;
}

const std::optional<MCPCapabilities>& MCPProtocol::GetClientCapabilities() const {
    return m_ClientCapabilities;
}

const std::optional<MCPServerInfo>& MCPProtocol::GetServerInfo() const {
    return m_ServerInfo;
}

MCPTask<const ResponseBase&> MCPProtocol::SendRequestImpl(const RequestBase& InRequest) {
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
        std::string responseStr = co_await m_Transport->SendRequest(InMethod, InParams);

        // Parse response
        auto response = JSONValue::parse(responseStr);
        co_return response;

    } catch (const std::exception&) {
        // Remove from pending requests if send failed
        std::lock_guard<std::mutex> lock(m_RequestsMutex);
        m_PendingRequests.erase(requestID);
        throw;
    }
}

MCPTask_Void MCPProtocol::SendNotificationImpl(std::string_view InMethod,
                                               const JSONValue& InParams) {
    co_await m_Transport->SendNotification(InMethod, InParams);
}

void MCPProtocol::HandleIncomingRequest(std::string_view InMethod, const JSONValue& InParams,
                                        std::string_view InRequestID) {
    try {
        std::lock_guard<std::mutex> lock(m_HandlersMutex);
        auto handler = m_RequestHandlers.find(InMethod);
        if (handler != m_RequestHandlers.end()) {
            handler->second(InParams, InRequestID);
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

void MCPProtocol::HandleIncomingResponse(std::string_view InResponseData) {
    try {
        auto response = JSONValue::parse(InResponseData);

        if (!response.contains("id")) {
            return; // Invalid response without ID
        }

        std::string requestID = response["id"].get<std::string>();

        std::lock_guard<std::mutex> lock(m_RequestsMutex);
        auto it = m_PendingRequests.find(requestID);
        if (it != m_PendingRequests.end()) {
            if (response.contains("result")) {
                it->second->Promise.set_value(response["result"]);
            } else if (response.contains("error")) {
                auto error = response["error"];
                std::string errorMsg = error["message"].get<std::string>();
                it->second->Promise.set_exception(
                    std::make_exception_ptr(std::runtime_error(errorMsg)));
            }
            m_PendingRequests.erase(it);
        }
    } catch (const std::exception& e) {
        if (m_ErrorHandler) { m_ErrorHandler("Error handling response: " + std::string(e.what())); }
    }
}

void MCPProtocol::HandleIncomingNotification(std::string_view InMethod, const JSONValue& InParams) {
    try {
        std::lock_guard<std::mutex> lock(m_HandlersMutex);
        auto handler = m_NotificationHandlers.find(InMethod);
        if (handler != m_NotificationHandlers.end()) { handler->second(InParams); }
        // Notifications without handlers are simply ignored
    } catch (const std::exception& e) {
        if (m_ErrorHandler) {
            m_ErrorHandler("Error handling notification: " + std::string(e.what()));
        }
    }
}

void MCPProtocol::HandleTransportError(std::string_view InError) {
    if (m_ErrorHandler) { m_ErrorHandler("Transport error: " + std::string(InError)); }
}

MCP_NAMESPACE_END