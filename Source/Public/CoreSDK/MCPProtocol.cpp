#include "MCPProtocol.h"

#include "MCPMessages.h"

// MCPProtocol Implementation
MCPProtocol::MCPProtocol(std::shared_ptr<ITransport> InTransport) : m_Transport(InTransport) {
    if (!m_Transport) { throw std::invalid_argument("Transport cannot be null"); }

    // Set up transport handlers
    m_Transport->SetRequestHandler([this](const std::string& InMethod,
                                          const nlohmann::json& InParams,
                                          const std::string& InRequestID) {
        HandleIncomingRequest(InMethod, InParams, InRequestID);
    });

    m_Transport->SetResponseHandler(
        [this](const std::string& InResponseData) { HandleIncomingResponse(InResponseData); });

    m_Transport->SetNotificationHandler(
        [this](const std::string& InMethod, const nlohmann::json& InParams) {
            HandleIncomingNotification(InMethod, InParams);
        });

    m_Transport->SetErrorHandler(
        [this](const std::string& InError) { HandleTransportError(InError); });
}

MCPProtocol::~MCPProtocol() {
    if (m_IsInitialized) {
        try {
            Shutdown().GetResult();
        } catch (...) {
            // Ignore errors during destruction
        }
    }
}

MCPTaskVoid MCPProtocol::Initialize(const MCPClientInfo& InClientInfo,
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

        auto responseJson = co_await SendRequestImpl("initialize", nlohmann::json(initRequest));
        auto response = responseJson.get<InitializeResponse>();

        // Store negotiated capabilities
        m_ClientCapabilities = response.Capabilities;
        m_ServerInfo = response.ServerInfo;

        // Send initialized notification
        co_await SendNotificationImpl("initialized", nlohmann::json::object());

        m_IsInitialized = true;

        if (m_InitializedHandler) { m_InitializedHandler(response); }

    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to initialize protocol: " + std::string(e.what()));
    }

    co_return;
}

MCPTaskVoid MCPProtocol::Shutdown() {
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

MCPTask<nlohmann::json> MCPProtocol::SendRequest(const std::string& InMethod,
                                                 const nlohmann::json& InParams) {
    if (!m_IsInitialized) { throw std::runtime_error("Protocol not initialized"); }

    co_return co_await SendRequestImpl(InMethod, InParams);
}

MCPTaskVoid MCPProtocol::SendResponse(const std::string& InRequestID,
                                      const nlohmann::json& InResult) {
    if (!m_IsInitialized) { throw std::runtime_error("Protocol not initialized"); }

    co_await m_Transport->SendResponse(InRequestID, InResult);
}

MCPTaskVoid MCPProtocol::SendErrorResponse(const std::string& InRequestID, int64_t InErrorCode,
                                           const std::string& InErrorMessage,
                                           const nlohmann::json& InErrorData) {
    if (!m_IsInitialized) { throw std::runtime_error("Protocol not initialized"); }

    co_await m_Transport->SendErrorResponse(InRequestID, InErrorCode, InErrorMessage, InErrorData);
}

MCPTaskVoid MCPProtocol::SendNotification(const std::string& InMethod,
                                          const nlohmann::json& InParams) {
    if (!m_IsInitialized) { throw std::runtime_error("Protocol not initialized"); }

    co_await SendNotificationImpl(InMethod, InParams);
}

void MCPProtocol::SetRequestHandler(const std::string& InMethod, RequestHandler InHandler) {
    std::lock_guard<std::mutex> lock(m_HandlersMutex);
    m_RequestHandlers[InMethod] = InHandler;
}

void MCPProtocol::SetNotificationHandler(const std::string& InMethod,
                                         NotificationHandler InHandler) {
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

MCPTask<nlohmann::json> MCPProtocol::SendRequestImpl(const std::string& InMethod,
                                                     const nlohmann::json& InParams) {
    std::string requestID = GenerateRequestID();

    // Create promise for response
    auto pendingRequest = std::make_unique<PendingRequest>();
    pendingRequest->Method = InMethod;
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
        auto response = nlohmann::json::parse(responseStr);
        co_return response;

    } catch (const std::exception&) {
        // Remove from pending requests if send failed
        std::lock_guard<std::mutex> lock(m_RequestsMutex);
        m_PendingRequests.erase(requestID);
        throw;
    }
}

MCPTaskVoid MCPProtocol::SendNotificationImpl(const std::string& InMethod,
                                              const nlohmann::json& InParams) {
    co_await m_Transport->SendNotification(InMethod, InParams);
}

void MCPProtocol::HandleIncomingRequest(const std::string& InMethod, const nlohmann::json& InParams,
                                        const std::string& InRequestID) {
    try {
        std::lock_guard<std::mutex> lock(m_HandlersMutex);
        auto handler = m_RequestHandlers.find(InMethod);
        if (handler != m_RequestHandlers.end()) {
            handler->second(InParams, InRequestID);
        } else {
            // Send method not found error
            SendErrorResponse(InRequestID, -32601, "Method not found", nlohmann::json::object());
        }
    } catch (const std::exception& e) {
        // Send internal error
        SendErrorResponse(InRequestID, -32603, "Internal error",
                          nlohmann::json::object({{"details", e.what()}}));
    }
}

void MCPProtocol::HandleIncomingResponse(const std::string& InResponseData) {
    try {
        auto response = nlohmann::json::parse(InResponseData);

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

void MCPProtocol::HandleIncomingNotification(const std::string& InMethod,
                                             const nlohmann::json& InParams) {
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

void MCPProtocol::HandleTransportError(const std::string& InError) {
    if (m_ErrorHandler) { m_ErrorHandler("Transport error: " + InError); }
}

std::string MCPProtocol::GenerateRequestID() {
    uint64_t counter = m_RequestCounter.fetch_add(1);
    return "req_" + std::to_string(counter);
}