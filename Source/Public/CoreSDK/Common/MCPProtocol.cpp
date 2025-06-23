#include "MCPProtocol.h"

#include <string_view>

#include "MCPMessages.h"

MCP_NAMESPACE_BEGIN

// MCPProtocol Implementation
MCPProtocol::MCPProtocol(std::shared_ptr<ITransport> InTransport) : m_Transport(InTransport) {
    if (!m_Transport) { throw std::invalid_argument("Transport cannot be null"); }

    // Set up transport handlers
    m_Transport->SetRequestHandler(
        [this](std::string_view InMethod, const JSONValue& InParams, std::string_view InRequestID) {
            HandleIncomingRequest(InMethod, InParams, InRequestID);
        });

    m_Transport->SetResponseHandler(
        [this](std::string_view InResponseData) { HandleIncomingResponse(InResponseData); });

    m_Transport->SetNotificationHandler(
        [this](std::string_view InMethod, const JSONValue& InParams) {
            HandleIncomingNotification(InMethod, InParams);
        });

    m_Transport->SetErrorHandler(
        [this](std::string_view InError) { HandleTransportError(InError); });
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

MCPTask<JSONValue> MCPProtocol::SendRequestImpl(std::string_view InMethod,
                                                const JSONValue& InParams) {
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

std::string MCPProtocol::GenerateRequestID() {
    uint64_t counter = m_RequestCounter.fetch_add(1);
    return "req_" + std::to_string(counter);
}

// MCPClient Implementation
MCPClient::MCPClient(TransportType InTransportType, std::unique_ptr<TransportOptions> InOptions)
    : m_TransportType(InTransportType), m_TransportOptions(std::move(InOptions)) {
    CreateTransport();
    CreateProtocol();
}

MCPClient::~MCPClient() {
    if (m_Protocol && m_Protocol->IsInitialized()) {
        try {
            m_Protocol->Shutdown().GetResult();
        } catch (...) {
            // Ignore errors during cleanup
        }
    }
}

MCPTask_Void MCPClient::Connect(const MCPClientInfo& InClientInfo) {
    if (m_IsConnected) { throw std::runtime_error("Client already connected"); }

    try {
        co_await m_Protocol->Initialize(InClientInfo, std::nullopt);
        m_IsConnected = true;
        m_ClientInfo = InClientInfo;
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to connect: " + std::string(e.what()));
    }

    co_return;
}

MCPTask_Void MCPClient::Disconnect() {
    if (!m_IsConnected) { co_return; }

    try {
        co_await m_Protocol->Shutdown();
        m_IsConnected = false;
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to disconnect: " + std::string(e.what()));
    }

    co_return;
}

bool MCPClient::IsConnected() const {
    return m_IsConnected;
}

MCPTask<ToolListResponse> MCPClient::ListTools(const std::optional<std::string>& InCursor) {
    if (!m_IsConnected) { throw std::runtime_error("Client not connected"); }

    ToolListRequest request;
    if (InCursor.has_value()) { request.Cursor = InCursor.value(); }

    auto response = co_await m_Protocol->SendRequest("tools/list", JSONValue(request));
    co_return response.get<ToolListResponse>();
}

MCPTask<ToolCallResponse> MCPClient::CallTool(const std::string& InToolName,
                                              const JSONValue& InArguments) {
    if (!m_IsConnected) { throw std::runtime_error("Client not connected"); }

    ToolCallRequest request;
    request.Name = InToolName;
    request.Arguments = InArguments;

    auto response = co_await m_Protocol->SendRequest("tools/call", JSONValue(request));
    co_return response.get<ToolCallResponse>();
}

MCPTask<PromptListResponse> MCPClient::ListPrompts(const std::optional<std::string>& InCursor) {
    if (!m_IsConnected) { throw std::runtime_error("Client not connected"); }

    PromptListRequest request;
    if (InCursor.has_value()) { request.Cursor = InCursor.value(); }

    auto response = co_await m_Protocol->SendRequest("prompts/list", JSONValue(request));
    co_return response.get<PromptListResponse>();
}

MCPTask<PromptGetResponse> MCPClient::GetPrompt(const std::string& InPromptName,
                                                const std::optional<JSONValue>& InArguments) {
    if (!m_IsConnected) { throw std::runtime_error("Client not connected"); }

    PromptGetRequest request;
    request.Name = InPromptName;
    if (InArguments.has_value()) { request.Arguments = InArguments.value(); }

    auto response = co_await m_Protocol->SendRequest("prompts/get", JSONValue(request));
    co_return response.get<PromptGetResponse>();
}

MCPTask<ResourceListResponse> MCPClient::ListResources(const std::optional<std::string>& InCursor) {
    if (!m_IsConnected) { throw std::runtime_error("Client not connected"); }

    ResourceListRequest request;
    if (InCursor.has_value()) { request.Cursor = InCursor.value(); }

    auto response = co_await m_Protocol->SendRequest("resources/list", JSONValue(request));
    co_return response.get<ResourceListResponse>();
}

MCPTask<ResourceReadResponse> MCPClient::ReadResource(const std::string& InResourceURI) {
    if (!m_IsConnected) { throw std::runtime_error("Client not connected"); }

    ResourceReadRequest request;
    request.URI = InResourceURI;

    auto response = co_await m_Protocol->SendRequest("resources/read", JSONValue(request));
    co_return response.get<ResourceReadResponse>();
}

MCPTask_Void MCPClient::SubscribeToResource(const std::string& InResourceURI) {
    if (!m_IsConnected) { throw std::runtime_error("Client not connected"); }

    ResourceSubscribeRequest request;
    request.URI = InResourceURI;

    co_await m_Protocol->SendRequest("resources/subscribe", JSONValue(request));
}

MCPTask_Void MCPClient::UnsubscribeFromResource(const std::string& InResourceURI) {
    if (!m_IsConnected) { throw std::runtime_error("Client not connected"); }

    ResourceUnsubscribeRequest request;
    request.URI = InResourceURI;

    co_await m_Protocol->SendRequest("resources/unsubscribe", JSONValue(request));
}

MCPTask<SamplingCreateMessageResponse>
MCPClient::CreateMessage(const SamplingCreateMessageRequest& InRequest) {
    if (!m_IsConnected) { throw std::runtime_error("Client not connected"); }

    auto response =
        co_await m_Protocol->SendRequest("sampling/createMessage", JSONValue(InRequest));
    co_return response.get<SamplingCreateMessageResponse>();
}

MCPTask<CompletionCompleteResponse>
MCPClient::CompleteText(const CompletionCompleteRequest& InRequest) {
    if (!m_IsConnected) { throw std::runtime_error("Client not connected"); }

    auto response = co_await m_Protocol->SendRequest("completion/complete", JSONValue(InRequest));
    co_return response.get<CompletionCompleteResponse>();
}

void MCPClient::SetResourceUpdatedHandler(ResourceUpdatedHandler InHandler) {
    m_ResourceUpdatedHandler = InHandler;

    // Set up protocol notification handler
    m_Protocol->SetNotificationHandler(
        "notifications/resources/updated", [this](const JSONValue& InParams) {
            if (m_ResourceUpdatedHandler) {
                auto notification = InParams.get<ResourceUpdatedNotification>();
                m_ResourceUpdatedHandler(notification);
            }
        });
}

void MCPClient::SetResourceListChangedHandler(ResourceListChangedHandler InHandler) {
    m_ResourceListChangedHandler = InHandler;

    m_Protocol->SetNotificationHandler(
        "notifications/resources/list_changed", [this](const JSONValue& InParams) {
            if (m_ResourceListChangedHandler) {
                auto notification = InParams.get<ResourceListChangedNotification>();
                m_ResourceListChangedHandler(notification);
            }
        });
}

void MCPClient::SetToolListChangedHandler(ToolListChangedHandler InHandler) {
    m_ToolListChangedHandler = InHandler;

    m_Protocol->SetNotificationHandler(
        "notifications/tools/list_changed", [this](const JSONValue& InParams) {
            if (m_ToolListChangedHandler) {
                auto notification = InParams.get<ToolListChangedNotification>();
                m_ToolListChangedHandler(notification);
            }
        });
}

void MCPClient::SetPromptListChangedHandler(PromptListChangedHandler InHandler) {
    m_PromptListChangedHandler = InHandler;

    m_Protocol->SetNotificationHandler(
        "notifications/prompts/list_changed", [this](const JSONValue& InParams) {
            if (m_PromptListChangedHandler) {
                auto notification = InParams.get<PromptListChangedNotification>();
                m_PromptListChangedHandler(notification);
            }
        });
}

void MCPClient::SetProgressHandler(ProgressHandler InHandler) {
    m_ProgressHandler = InHandler;

    m_Protocol->SetNotificationHandler("notifications/progress", [this](const JSONValue& InParams) {
        if (m_ProgressHandler) {
            auto notification = InParams.get<ProgressNotification>();
            m_ProgressHandler(notification);
        }
    });
}

void MCPClient::SetLogHandler(LogHandler InHandler) {
    m_LogHandler = InHandler;

    m_Protocol->SetNotificationHandler("notifications/message", [this](const JSONValue& InParams) {
        if (m_LogHandler) {
            auto notification = InParams.get<LoggingMessageNotification>();
            m_LogHandler(notification);
        }
    });
}

void MCPClient::CreateTransport() {
    switch (m_TransportType) {
        case TransportType::Stdio: {
            auto stdioOptions = dynamic_cast<StdioTransportOptions*>(m_TransportOptions.get());
            if (!stdioOptions) {
                throw std::invalid_argument("Invalid options for stdio transport");
            }
            m_Transport = std::make_unique<StdioTransport>(*stdioOptions);
            break;
        }
        case TransportType::StreamableHTTP: {
            auto httpOptions = dynamic_cast<HTTPTransportOptions*>(m_TransportOptions.get());
            if (!httpOptions) { throw std::invalid_argument("Invalid options for HTTP transport"); }
            m_Transport = std::make_unique<HTTPTransportClient>(*httpOptions);
            break;
        }
        default: throw std::invalid_argument("Unsupported transport type");
    }
}

void MCPClient::CreateProtocol() {
    m_Protocol = std::make_unique<MCPProtocol>(m_Transport);
}

// MCPServer Implementation
MCPServer::MCPServer(TransportType InTransportType, std::unique_ptr<TransportOptions> InOptions)
    : m_TransportType(InTransportType), m_TransportOptions(std::move(InOptions)) {
    CreateTransport();
    CreateProtocol();
    SetupDefaultHandlers();
}

MCPServer::~MCPServer() {
    if (m_Protocol && m_Protocol->IsInitialized()) {
        try {
            m_Protocol->Shutdown().GetResult();
        } catch (...) {
            // Ignore errors during cleanup
        }
    }
}

MCPTask_Void MCPServer::Start(const MCPServerInfo& InServerInfo) {
    if (m_IsRunning) { throw std::runtime_error("Server already running"); }

    try {
        m_ServerInfo = InServerInfo;

        // Start transport
        co_await m_Transport->Start();

        m_IsRunning = true;

        // Server doesn't need to call Initialize - it responds to client initialization

    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to start server: " + std::string(e.what()));
    }

    co_return;
}

MCPTask_Void MCPServer::Stop() {
    if (!m_IsRunning) { co_return; }

    try {
        co_await m_Transport->Stop();
        m_IsRunning = false;
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to stop server: " + std::string(e.what()));
    }

    co_return;
}

bool MCPServer::IsRunning() const {
    return m_IsRunning;
}

void MCPServer::AddTool(const std::string& InName, const Tool& InTool, ToolHandler InHandler) {
    if (m_IsRunning) { throw std::runtime_error("Cannot add tools while server is running"); }

    std::lock_guard<std::mutex> lock(m_ToolsMutex);
    m_Tools[InName] = InTool;
    m_ToolHandlers[InName] = InHandler;
}

void MCPServer::RemoveTool(const std::string& InName) {
    if (m_IsRunning) { throw std::runtime_error("Cannot remove tools while server is running"); }

    std::lock_guard<std::mutex> lock(m_ToolsMutex);
    m_Tools.erase(InName);
    m_ToolHandlers.erase(InName);
}

void MCPServer::AddPrompt(const std::string& InName, const Prompt& InPrompt,
                          PromptHandler InHandler) {
    if (m_IsRunning) { throw std::runtime_error("Cannot add prompts while server is running"); }

    std::lock_guard<std::mutex> lock(m_PromptsMutex);
    m_Prompts[InName] = InPrompt;
    m_PromptHandlers[InName] = InHandler;
}

void MCPServer::RemovePrompt(const std::string& InName) {
    if (m_IsRunning) { throw std::runtime_error("Cannot remove prompts while server is running"); }

    std::lock_guard<std::mutex> lock(m_PromptsMutex);
    m_Prompts.erase(InName);
    m_PromptHandlers.erase(InName);
}

void MCPServer::AddResource(const std::string& InURI, const Resource& InResource,
                            ResourceHandler InHandler) {
    if (m_IsRunning) { throw std::runtime_error("Cannot add resources while server is running"); }

    std::lock_guard<std::mutex> lock(m_ResourcesMutex);
    m_Resources[InURI] = InResource;
    m_ResourceHandlers[InURI] = InHandler;
}

void MCPServer::RemoveResource(const std::string& InURI) {
    if (m_IsRunning) {
        throw std::runtime_error("Cannot remove resources while server is running");
    }

    std::lock_guard<std::mutex> lock(m_ResourcesMutex);
    m_Resources.erase(InURI);
    m_ResourceHandlers.erase(InURI);
}

MCPTask_Void MCPServer::NotifyResourceUpdated(const std::string& InURI) {
    ResourceUpdatedNotification notification;
    notification.URI = InURI;

    co_await m_Protocol->SendNotification("notifications/resources/updated",
                                          JSONValue(notification));
}

MCPTask_Void MCPServer::NotifyResourceListChanged() {
    ResourceListChangedNotification notification;

    co_await m_Protocol->SendNotification("notifications/resources/list_changed",
                                          JSONValue(notification));
}

MCPTask_Void MCPServer::NotifyToolListChanged() {
    ToolListChangedNotification notification;

    co_await m_Protocol->SendNotification("notifications/tools/list_changed",
                                          JSONValue(notification));
}

MCPTask_Void MCPServer::NotifyPromptListChanged() {
    PromptListChangedNotification notification;

    co_await m_Protocol->SendNotification("notifications/prompts/list_changed",
                                          JSONValue(notification));
}

MCPTask_Void MCPServer::SendProgress(const std::string& InProgressToken, double InProgress,
                                     double InTotal) {
    ProgressNotification notification;
    notification.ProgressToken = InProgressToken;
    notification.Progress = InProgress;
    notification.Total = InTotal;

    co_await m_Protocol->SendNotification("notifications/progress", JSONValue(notification));
}

MCPTask_Void MCPServer::SendLog(LoggingLevel InLevel, const std::string& InMessage,
                                const std::optional<std::string>& InLogger) {
    LoggingMessageNotification notification;
    notification.Level = InLevel;
    notification.Data = InMessage;
    if (InLogger.has_value()) { notification.Logger = InLogger.value(); }

    co_await m_Protocol->SendNotification("notifications/message", JSONValue(notification));
}

void MCPServer::SetSamplingHandler(SamplingHandler InHandler) {
    m_SamplingHandler = InHandler;
}

void MCPServer::SetCompletionHandler(CompletionHandler InHandler) {
    m_CompletionHandler = InHandler;
}

void MCPServer::CreateTransport() {
    switch (m_TransportType) {
        case TransportType::Stdio: {
            m_Transport = std::make_unique<StdioServerTransport>();
            break;
        }
        case TransportType::StreamableHTTP: {
            auto httpOptions = dynamic_cast<HTTPTransportOptions*>(m_TransportOptions.get());
            if (!httpOptions) { throw std::invalid_argument("Invalid options for HTTP transport"); }
            m_Transport = std::make_unique<HTTPTransportServer>(*httpOptions);
            break;
        }
        default: throw std::invalid_argument("Unsupported transport type");
    }
}

void MCPServer::CreateProtocol() {
    m_Protocol = std::make_unique<MCPProtocol>(m_Transport);
}

void MCPServer::SetupDefaultHandlers() {
    // Initialize request handler
    m_Protocol->SetRequestHandler(
        "initialize", [this](const JSONValue& InParams, const std::string& InRequestID) {
            HandleInitialize(InParams, InRequestID);
        });

    // Tools handlers
    m_Protocol->SetRequestHandler(
        "tools/list", [this](const JSONValue& InParams, const std::string& InRequestID) {
            HandleToolsList(InParams, InRequestID);
        });

    m_Protocol->SetRequestHandler(
        "tools/call", [this](const JSONValue& InParams, const std::string& InRequestID) {
            HandleToolCall(InParams, InRequestID);
        });

    // Prompts handlers
    m_Protocol->SetRequestHandler(
        "prompts/list", [this](const JSONValue& InParams, const std::string& InRequestID) {
            HandlePromptsList(InParams, InRequestID);
        });

    m_Protocol->SetRequestHandler(
        "prompts/get", [this](const JSONValue& InParams, const std::string& InRequestID) {
            HandlePromptGet(InParams, InRequestID);
        });

    // Resources handlers
    m_Protocol->SetRequestHandler(
        "resources/list", [this](const JSONValue& InParams, const std::string& InRequestID) {
            HandleResourcesList(InParams, InRequestID);
        });

    m_Protocol->SetRequestHandler(
        "resources/read", [this](const JSONValue& InParams, const std::string& InRequestID) {
            HandleResourceRead(InParams, InRequestID);
        });

    m_Protocol->SetRequestHandler(
        "resources/subscribe", [this](const JSONValue& InParams, const std::string& InRequestID) {
            HandleResourceSubscribe(InParams, InRequestID);
        });

    m_Protocol->SetRequestHandler(
        "resources/unsubscribe", [this](const JSONValue& InParams, const std::string& InRequestID) {
            HandleResourceUnsubscribe(InParams, InRequestID);
        });

    // Sampling handler
    m_Protocol->SetRequestHandler("sampling/createMessage", [this](const JSONValue& InParams,
                                                                   const std::string& InRequestID) {
        HandleSamplingCreateMessage(InParams, InRequestID);
    });

    // Completion handler
    m_Protocol->SetRequestHandler(
        "completion/complete", [this](const JSONValue& InParams, const std::string& InRequestID) {
            HandleCompletionComplete(InParams, InRequestID);
        });
}

void MCPServer::HandleInitialize(const JSONValue& InParams, const std::string& InRequestID) {
    try {
        auto request = InParams.get<InitializeRequest>();

        // CRITICAL: Validate protocol version first
        static const std::vector<std::string> SUPPORTED_PROTOCOL_VERSIONS = {"2024-11-05",
                                                                             "2025-03-26"};

        auto iter = std::find(SUPPORTED_PROTOCOL_VERSIONS.begin(),
                              SUPPORTED_PROTOCOL_VERSIONS.end(), request.ProtocolVersion);

        if (iter == SUPPORTED_PROTOCOL_VERSIONS.end()) {
            std::string supportedVersions;
            for (size_t i = 0; i < SUPPORTED_PROTOCOL_VERSIONS.size(); ++i) {
                if (i > 0) supportedVersions += ", ";
                supportedVersions += SUPPORTED_PROTOCOL_VERSIONS[i];
            }

            m_Protocol->SendErrorResponse(InRequestID, -32602,
                                          "Unsupported protocol version: " + request.ProtocolVersion
                                              + ". Supported versions: " + supportedVersions,
                                          JSONValue::object());
            return;
        }

        InitializeResponse response;
        response.ProtocolVersion = request.ProtocolVersion; // Use negotiated version
        response.ServerInfo = m_ServerInfo;

        // Set up capabilities based on what we support
        MCPCapabilities capabilities;

        // Tools capability
        if (!m_Tools.empty()) {
            capabilities.Tools = ToolsCapability{};
            capabilities.Tools->ListChanged = true;
        }

        // Prompts capability
        if (!m_Prompts.empty()) {
            capabilities.Prompts = PromptsCapability{};
            capabilities.Prompts->ListChanged = true;
        }

        // Resources capability
        if (!m_Resources.empty()) {
            capabilities.Resources = ResourcesCapability{};
            capabilities.Resources->Subscribe = true;
            capabilities.Resources->ListChanged = true;
        }

        // Logging capability
        capabilities.Logging = LoggingCapability{};

        // Sampling capability if handler is set
        if (m_SamplingHandler) { capabilities.Sampling = SamplingCapability{}; }

        response.Capabilities = capabilities;

        m_Protocol->SendResponse(InRequestID, JSONValue(response));

    } catch (const std::exception& e) {
        m_Protocol->SendErrorResponse(InRequestID, -32603, "Internal error",
                                      JSONValue::object({{"details", e.what()}}));
    }
}

void MCPServer::HandleToolsList(const JSONValue& InParams, const std::string& InRequestID) {
    try {
        auto request = InParams.get<ToolListRequest>();

        ToolListResponse response;

        std::lock_guard<std::mutex> lock(m_ToolsMutex);
        for (const auto& [name, tool] : m_Tools) { response.Tools.push_back(tool); }

        m_Protocol->SendResponse(InRequestID, JSONValue(response));

    } catch (const std::exception& e) {
        m_Protocol->SendErrorResponse(InRequestID, -32603, "Internal error",
                                      JSONValue::object({{"details", e.what()}}));
    }
}

void MCPServer::HandleToolCall(const JSONValue& InParams, const std::string& InRequestID) {
    try {
        auto request = InParams.get<ToolCallRequest>();

        std::lock_guard<std::mutex> lock(m_ToolsMutex);
        auto toolIter = m_Tools.find(request.Name);
        if (toolIter == m_Tools.end()) {
            m_Protocol->SendErrorResponse(InRequestID, -32601, "Tool not found",
                                          JSONValue::object());
            return;
        }

        const auto& tool = toolIter->second;
        auto handler = m_ToolHandlers.find(request.Name);
        if (handler == m_ToolHandlers.end()) {
            m_Protocol->SendErrorResponse(InRequestID, -32601, "Tool handler not found",
                                          JSONValue::object());
            return;
        }

        // CRITICAL: Validate arguments against tool's input schema
        if (request.Arguments.has_value()) {
            JSONValue argsJson = *request.Arguments;

            // Validate using JSONSchemaValidator
            auto validationResult =
                JSONSchemaValidator::ValidateAgainstSchema(argsJson, tool.InputSchema);
            if (!validationResult.IsValid) {
                std::string errorDetails = "Tool arguments validation failed: ";
                for (size_t i = 0; i < validationResult.Errors.size(); ++i) {
                    if (i > 0) errorDetails += "; ";
                    errorDetails += validationResult.Errors[i];
                }
                m_Protocol->SendErrorResponse(InRequestID, -32602, "Schema validation error",
                                              JSONValue::object({{"details", errorDetails}}));
                return;
            }
        } else {
            // Check if tool requires arguments
            if (tool.InputSchema.Required.has_value() && !tool.InputSchema.Required->empty()) {
                m_Protocol->SendErrorResponse(InRequestID, -32602, "Required arguments missing",
                                              JSONValue::object({{"tool", request.Name}}));
                return;
            }
        }

        // Call handler with validated arguments
        auto response = handler->second(request.Arguments);
        m_Protocol->SendResponse(InRequestID, JSONValue(response));

    } catch (const std::exception& e) {
        m_Protocol->SendErrorResponse(InRequestID, -32603, "Internal error",
                                      JSONValue::object({{"details", e.what()}}));
    }
}

void MCPServer::HandlePromptsList(const JSONValue& InParams, const std::string& InRequestID) {
    try {
        auto request = InParams.get<PromptListRequest>();

        PromptListResponse response;

        std::lock_guard<std::mutex> lock(m_PromptsMutex);
        for (const auto& [name, prompt] : m_Prompts) { response.Prompts.push_back(prompt); }

        m_Protocol->SendResponse(InRequestID, JSONValue(response));

    } catch (const std::exception& e) {
        m_Protocol->SendErrorResponse(InRequestID, -32603, "Internal error",
                                      JSONValue::object({{"details", e.what()}}));
    }
}

void MCPServer::HandlePromptGet(const JSONValue& InParams, const std::string& InRequestID) {
    try {
        auto request = InParams.get<PromptGetRequest>();

        std::lock_guard<std::mutex> lock(m_PromptsMutex);
        auto handler = m_PromptHandlers.find(request.Name);
        if (handler == m_PromptHandlers.end()) {
            m_Protocol->SendErrorResponse(InRequestID, -32601, "Prompt not found",
                                          JSONValue::object());
            return;
        }

        // Call handler
        auto response = handler->second(request.Arguments);
        m_Protocol->SendResponse(InRequestID, JSONValue(response));

    } catch (const std::exception& e) {
        m_Protocol->SendErrorResponse(InRequestID, -32603, "Internal error",
                                      JSONValue::object({{"details", e.what()}}));
    }
}

void MCPServer::HandleResourcesList(const JSONValue& InParams, const std::string& InRequestID) {
    try {
        auto request = InParams.get<ResourceListRequest>();

        ResourceListResponse response;
        size_t startIndex = 0;
        static constexpr size_t DEFAULT_PAGE_SIZE = 100;
        size_t pageSize = DEFAULT_PAGE_SIZE;

        // Decode cursor if provided
        if (request.Cursor.has_value()) {
            try {
                startIndex = DecodeCursor(*request.Cursor);
            } catch (const std::exception& e) {
                m_Protocol->SendErrorResponse(InRequestID, -32602, "Invalid cursor format",
                                              JSONValue::object({{"details", e.what()}}));
                return;
            }
        }

        // Get all available resources
        std::vector<Resource> allResources;
        {
            std::lock_guard<std::mutex> lock(m_ResourcesMutex);
            allResources.reserve(m_Resources.size());
            for (const auto& [uri, resource] : m_Resources) { allResources.push_back(resource); }
        }

        auto totalResources = allResources.size();
        auto endIndex = std::min(startIndex + pageSize, totalResources);

        // Extract page of resources
        if (startIndex < totalResources) {
            response.Resources.assign(allResources.begin() + startIndex,
                                      allResources.begin() + endIndex);
        }

        // Set next cursor if more resources available
        if (endIndex < totalResources) { response.NextCursor = EncodeCursor(endIndex); }

        m_Protocol->SendResponse(InRequestID, JSONValue(response));

    } catch (const std::exception& e) {
        m_Protocol->SendErrorResponse(InRequestID, -32603, "Internal error",
                                      JSONValue::object({{"details", e.what()}}));
    }
}

void MCPServer::HandleResourceRead(const JSONValue& InParams, const std::string& InRequestID) {
    try {
        auto request = InParams.get<ResourceReadRequest>();

        std::lock_guard<std::mutex> lock(m_ResourcesMutex);
        auto handler = m_ResourceHandlers.find(request.URI);
        if (handler == m_ResourceHandlers.end()) {
            m_Protocol->SendErrorResponse(InRequestID, -32601, "Resource not found",
                                          JSONValue::object());
            return;
        }

        // Call handler
        auto response = handler->second();
        m_Protocol->SendResponse(InRequestID, JSONValue(response));

    } catch (const std::exception& e) {
        m_Protocol->SendErrorResponse(InRequestID, -32603, "Internal error",
                                      JSONValue::object({{"details", e.what()}}));
    }
}

void MCPServer::HandleResourceSubscribe(const JSONValue& InParams, const std::string& InRequestID) {
    try {
        auto request = InParams.get<ResourceSubscribeRequest>();
        std::string uri = request.URI;
        std::string clientID =
            GetCurrentClientID(); // This needs to be implemented based on transport

        // Validate resource exists
        {
            std::lock_guard<std::mutex> lock(m_ResourcesMutex);
            if (m_Resources.find(uri) == m_Resources.end()) {
                m_Protocol->SendErrorResponse(InRequestID, -32601, "Resource not found",
                                              JSONValue::object({{"uri", uri}}));
                return;
            }
        }

        // Add subscription with proper client tracking
        {
            std::lock_guard<std::mutex> lock(m_ResourceSubscriptionsMutex);
            m_ResourceSubscriptions[uri].insert(clientID);
        }

        // Send empty response to indicate success
        m_Protocol->SendResponse(InRequestID, JSONValue::object());

    } catch (const std::exception& e) {
        m_Protocol->SendErrorResponse(InRequestID, -32603, "Internal error",
                                      JSONValue::object({{"details", e.what()}}));
    }
}

void MCPServer::HandleResourceUnsubscribe(const JSONValue& InParams,
                                          const std::string& InRequestID) {
    try {
        auto request = InParams.get<ResourceUnsubscribeRequest>();
        std::string uri = request.URI;
        std::string clientID = GetCurrentClientID();

        // Remove from subscriptions
        {
            std::lock_guard<std::mutex> lock(m_ResourceSubscriptionsMutex);
            auto iter = m_ResourceSubscriptions.find(uri);
            if (iter != m_ResourceSubscriptions.end()) {
                iter->second.erase(clientID);
                // Clean up empty subscription sets
                if (iter->second.empty()) { m_ResourceSubscriptions.erase(iter); }
            }
        }

        // Send empty response
        m_Protocol->SendResponse(InRequestID, JSONValue::object());

    } catch (const std::exception& e) {
        m_Protocol->SendErrorResponse(InRequestID, -32603, "Internal error",
                                      JSONValue::object({{"details", e.what()}}));
    }
}

// Enhanced resource change notification
MCPTask_Void MCPServer::NotifyResourceSubscribers(const std::string& InURI) {
    std::set<std::string> subscribers;

    {
        std::lock_guard<std::mutex> lock(m_ResourceSubscriptionsMutex);
        auto iter = m_ResourceSubscriptions.find(InURI);
        if (iter != m_ResourceSubscriptions.end()) {
            subscribers = iter->second; // Copy the subscriber set
        }
    }

    if (!subscribers.empty()) {
        ResourceUpdatedNotification notification;
        notification.URI = InURI;

        // Send notification to all subscribers
        for (const auto& clientID : subscribers) {
            try {
                co_await SendNotificationToClient(clientID, notification);
            } catch (const std::exception&) {
                // Continue with other clients if one fails
                // TODO: Consider removing failed clients from subscription
            }
        }
    }

    co_return;
}

// Client identification helper (simplified - in production would use transport session data)
std::string MCPServer::GetCurrentClientID() const {
    // For now, return a default client ID
    // In a real implementation, this would extract the client ID from the current transport session
    return "default_client";
}

// Send notification to specific client (simplified implementation)
MCPTask_Void
MCPServer::SendNotificationToClient(const std::string& InClientID,
                                    const ResourceUpdatedNotification& InNotification) {
    // For now, send to all clients via the protocol
    // In a real implementation, this would route to the specific client
    co_await m_Protocol->SendNotification("notifications/resources/updated",
                                          JSONValue(InNotification));
    co_return;
}

void MCPServer::HandleSamplingCreateMessage(const JSONValue& InParams,
                                            const std::string& InRequestID) {
    try {
        auto request = InParams.get<SamplingCreateMessageRequest>();

        if (!m_SamplingHandler) {
            m_Protocol->SendErrorResponse(InRequestID, -32601, "Sampling not supported",
                                          JSONValue::object());
            return;
        }

        // Call handler
        auto response = m_SamplingHandler(request);
        m_Protocol->SendResponse(InRequestID, JSONValue(response));

    } catch (const std::exception& e) {
        m_Protocol->SendErrorResponse(InRequestID, -32603, "Internal error",
                                      JSONValue::object({{"details", e.what()}}));
    }
}

void MCPServer::HandleCompletionComplete(const JSONValue& InParams,
                                         const std::string& InRequestID) {
    try {
        auto request = InParams.get<CompletionCompleteRequest>();

        if (!m_CompletionHandler) {
            m_Protocol->SendErrorResponse(InRequestID, -32601, "Completion not supported",
                                          JSONValue::object());
            return;
        }

        // Call handler
        auto response = m_CompletionHandler(request);
        m_Protocol->SendResponse(InRequestID, JSONValue(response));

    } catch (const std::exception& e) {
        m_Protocol->SendErrorResponse(InRequestID, -32603, "Internal error",
                                      JSONValue::object({{"details", e.what()}}));
    }
}

std::string MCPServer::EncodeCursor(size_t InIndex) const {
    // Use base64 encoding for cursor
    std::string indexStr = std::to_string(InIndex);

    // Simple base64 encoding (for production, use a proper base64 library)
    static const std::string chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    int val = 0;
    int valb = -6;

    for (unsigned char c : indexStr) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            result.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) { result.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]); }
    while (result.size() % 4) { result.push_back('='); }

    return result;
}

size_t MCPServer::DecodeCursor(const std::string& InCursor) const {
    try {
        // Simple base64 decoding
        static const int T[128] = {
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62,
            -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -2, -1, -1, -1, 0,
            1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
            23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
            39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1};

        std::string decoded;
        int val = 0;
        int valb = -8;

        for (unsigned char c : InCursor) {
            if (T[c] == -1) break;
            val = (val << 6) + T[c];
            valb += 6;
            if (valb >= 0) {
                decoded.push_back(char((val >> valb) & 0xFF));
                valb -= 8;
            }
        }

        return std::stoull(decoded);
    } catch (const std::exception&) { throw std::invalid_argument("Invalid cursor format"); }
}

// ProgressTracker Implementation
ProgressTracker::ProgressTracker(const std::string& InRequestID,
                                 std::shared_ptr<MCPProtocol> InProtocol)
    : m_RequestID(InRequestID), m_Protocol(InProtocol) {}

MCPTask_Void ProgressTracker::UpdateProgress(double InProgress, std::optional<int64_t> InTotal) {
    if (m_IsComplete.load() || !m_Protocol) { co_return; }

    try {
        ProgressNotification notification;
        notification.ProgressToken = m_RequestID;
        notification.Progress = InProgress;
        if (InTotal.has_value()) { notification.Total = *InTotal; }

        co_await m_Protocol->SendNotification("notifications/progress", JSONValue(notification));
    } catch (const std::exception&) {
        // Ignore progress reporting errors to not break main operation
    }

    co_return;
}

MCPTask_Void ProgressTracker::CompleteProgress() {
    bool expected = false;
    if (m_IsComplete.compare_exchange_strong(expected, true)) {
        // Send 100% completion
        co_await UpdateProgress(1.0);
    }
    co_return;
}

// Enhanced tool execution with progress reporting
MCPTask<ToolCallResponse> MCPServer::ExecuteToolWithProgress(
    const Tool& InTool,
    const std::optional<std::unordered_map<std::string, JSONValue>>& InArguments,
    const std::string& InRequestID) {
    auto progressTracker = std::make_shared<ProgressTracker>(InRequestID, m_Protocol);

    try {
        // Update progress at 0%
        co_await progressTracker->UpdateProgress(0.0);

        // Find and execute tool handler
        auto handlerIter = m_ToolHandlers.find(InTool.Name);
        if (handlerIter != m_ToolHandlers.end()) {
            // Execute tool with progress capability
            auto result = co_await handlerIter->second(InArguments);

            // Complete progress
            co_await progressTracker->CompleteProgress();

            co_return result;
        } else {
            throw std::runtime_error("Tool handler not found");
        }
    } catch (const std::exception& e) {
        // Ensure progress is marked complete even on error
        co_await progressTracker->CompleteProgress();
        throw;
    }
}

MCP_NAMESPACE_END