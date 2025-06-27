#include "CoreSDK/Core/MCPServer.h"

#include "CoreSDK/Common/Progress.h"
#include "CoreSDK/Messages/MCPMessages.h"

MCP_NAMESPACE_BEGIN

MCPServer::MCPServer(TransportType InTransportType,
                     std::optional<std::unique_ptr<TransportOptions>> InOptions,
                     const Implementation& InServerInfo, const ServerCapabilities& InCapabilities)
    : MCPProtocol(TransportFactory::CreateTransport(InTransportType, TransportSide::Server,
                                                    std::move(InOptions))) {
    SetupDefaultHandlers();
    SetServerInfo(InServerInfo);
    SetServerCapabilities(InCapabilities);
}

MCPTask_Void MCPServer::Start() {
    if (m_IsRunning) {
        HandleRuntimeError("Server already running");
        co_return;
    }

    try {
        // Start transport
        co_await m_Transport->Connect();
        m_IsRunning = true;

        // Server doesn't need to call Initialize - it responds to client initialization

    } catch (const std::exception& e) {
        HandleRuntimeError("Failed to start server: " + std::string(e.what()));
        co_return;
    }

    co_return;
}

MCPTask_Void MCPServer::Stop() {
    if (!m_IsRunning) {
        HandleRuntimeError("Server already stopped");
        co_return;
    }

    try {
        co_await m_Transport->Disconnect();
        m_IsRunning = false;

    } catch (const std::exception& e) {
        HandleRuntimeError("Failed to stop server: " + std::string(e.what()));
        co_return;
    }

    co_return;
}

bool MCPServer::IsRunning() const {
    return m_IsRunning;
}

// Tool management - delegate to ToolManager
void MCPServer::AddTool(const Tool& InTool) {
    if (m_IsRunning) {
        HandleRuntimeError("Cannot add tools while server is running");
        return;
    }

    m_ToolManager->AddTool(InTool);
}

void MCPServer::RemoveTool(const Tool& InTool) {
    if (m_IsRunning) {
        HandleRuntimeError("Cannot remove tools while server is running");
        return;
    }

    m_ToolManager->RemoveTool(InTool.Name);
}

// Prompt management - delegate to PromptManager
void MCPServer::AddPrompt(const Prompt& InPrompt) {
    if (m_IsRunning) {
        HandleRuntimeError("Cannot add prompts while server is running");
        return;
    }

    m_PromptManager->AddPrompt(InPrompt);
}

void MCPServer::RemovePrompt(const Prompt& InPrompt) {
    if (m_IsRunning) {
        HandleRuntimeError("Cannot remove prompts while server is running");
        return;
    }

    m_PromptManager->RemovePrompt(InPrompt);
}

// Resource management - delegate to ResourceManager
void MCPServer::AddResource(const Resource& InResource) {
    if (m_IsRunning) {
        HandleRuntimeError("Cannot add resources while server is running");
        return;
    }

    m_ResourceManager->AddResource(InResource);
}

void MCPServer::AddResourceTemplate(const ResourceTemplate& InTemplate) {
    if (m_IsRunning) {
        HandleRuntimeError("Cannot add resource templates while server is running");
        return;
    }

    m_ResourceManager->AddTemplate(InTemplate);
}

void MCPServer::RemoveResource(const Resource& InResource) {
    if (m_IsRunning) {
        HandleRuntimeError("Cannot remove resources while server is running");
        return;
    }

    m_ResourceManager->RemoveResource(InResource);
}

void MCPServer::RemoveResourceTemplate(const ResourceTemplate& InTemplate) {
    if (m_IsRunning) {
        HandleRuntimeError("Cannot remove resource templates while server is running");
        return;
    }

    m_ResourceManager->RemoveTemplate(InTemplate);
}

MCPTask_Void MCPServer::NotifyResourceUpdated(const Resource& InResource) {
    ResourceUpdatedNotification notification;
    notification.Params.URI = MCP::URI(InURI);

    co_await SendNotification("notifications/resources/updated", JSONValue(notification));
}

MCPTask_Void MCPServer::NotifyResourceListChanged() {
    ResourceListChangedNotification notification;

    co_await SendNotification("notifications/resources/list_changed", JSONValue(notification));
}

MCPTask_Void MCPServer::NotifyToolListChanged() {
    ToolListChangedNotification notification;

    co_await SendNotification("notifications/tools/list_changed", JSONValue(notification));
}

MCPTask_Void MCPServer::NotifyPromptListChanged() {
    PromptListChangedNotification notification;

    co_await SendNotification("notifications/prompts/list_changed", JSONValue(notification));
}

MCPTask_Void MCPServer::ReportProgress(const RequestID& InRequestID, double InProgress,
                                       int64_t InTotal) {
    ProgressNotification notification;
    notification.Params.ProgressToken = ProgressToken(InRequestID);
    notification.Params.Progress = InProgress;
    if (InTotal >= 0) { notification.Params.Total = InTotal; }

    co_await SendNotification("notifications/progress", JSONValue(notification));
}

MCPTask_Void MCPServer::LogMessage(LoggingLevel InLevel, const std::string& InLogger,
                                   const JSONValue& InData) {
    LoggingMessageNotification notification;
    notification.Params.Level = InLevel;
    notification.Params.Data = InData;
    if (!InLogger.empty()) { notification.Params.Logger = InLogger; }

    co_await SendNotification("notifications/message", JSONValue(notification));
}

void MCPServer::SetupDefaultHandlers() {
    // Register request handlers using the RegisterRequestHandler method
    RegisterRequestHandler(
        RequestBase("initialize", std::nullopt, [this](const RequestBase& InRequest) {
            try {
                JSONValue paramsJSON;
                if (InRequest.Params.has_value()) {
                    to_json(paramsJSON, InRequest.Params.value());
                } else {
                    paramsJSON = JSONValue::object();
                }
                HandleInitialize(paramsJSON, InRequest.ID);
            } catch (const std::exception& e) {
                SendErrorResponse(InRequest.ID, -32603, "Internal error",
                                  JSONValue::object({{"details", e.what()}}));
            }
        }));

    // Tools handlers
    RegisterRequestHandler(RequestBase("tools/list", std::nullopt,
        "initialize", [this](const RequestBase& InRequest, MCPContext* InContext) {
        try {
            JSONValue paramsJSON;
            if (InRequest.Params.has_value()) {
                to_json(paramsJSON, InRequest.Params.value());
            } else {
                paramsJSON = JSONValue::object();
            }
            HandleInitialize(paramsJSON, InRequest.ID);
        } catch (const std::exception& e) {
            SendErrorResponse(InRequest.ID, -32603, "Internal error",
                              JSONValue::object({{"details", e.what()}}));
        }
        });

    // Tools handlers
    GetRequestManager().RegisterRequestHandler(
        "tools/list", [this](const RequestBase& InRequest, MCPContext* InContext) {
        try {
            JSONValue paramsJSON;
            if (InRequest.Params.has_value()) {
                to_json(paramsJSON, InRequest.Params.value());
            } else {
                paramsJSON = JSONValue::object();
            }
            HandleToolsList(paramsJSON, InRequest.ID);
        } catch (const std::exception& e) {
            SendErrorResponse(InRequest.ID, -32603, "Internal error",
                              JSONValue::object({{"details", e.what()}}));
        }
        });

    GetRequestManager().RegisterRequestHandler(
        "tools/call", [this](const RequestBase& InRequest, MCPContext* InContext) {
        try {
            JSONValue paramsJSON;
            if (InRequest.Params.has_value()) {
                to_json(paramsJSON, InRequest.Params.value());
            } else {
                paramsJSON = JSONValue::object();
            }
            HandleToolCall(paramsJSON, InRequest.ID);
        } catch (const std::exception& e) {
            SendErrorResponse(InRequest.ID, -32603, "Internal error",
                              JSONValue::object({{"details", e.what()}}));
        }
        });

    // Prompts handlers
    GetRequestManager().RegisterRequestHandler(
        "prompts/list", [this](const RequestBase& InRequest, MCPContext* InContext) {
        try {
            JSONValue paramsJSON;
            if (InRequest.Params.has_value()) {
                to_json(paramsJSON, InRequest.Params.value());
            } else {
                paramsJSON = JSONValue::object();
            }
            HandlePromptsList(paramsJSON, InRequest.ID);
        } catch (const std::exception& e) {
            SendErrorResponse(InRequest.ID, -32603, "Internal error",
                              JSONValue::object({{"details", e.what()}}));
        }
        });

    GetRequestManager().RegisterRequestHandler(
        "prompts/get", [this](const RequestBase& InRequest, MCPContext* InContext) {
        try {
            JSONValue paramsJSON;
            if (InRequest.Params.has_value()) {
                to_json(paramsJSON, InRequest.Params.value());
            } else {
                paramsJSON = JSONValue::object();
            }
            HandlePromptGet(paramsJSON, InRequest.ID);
        } catch (const std::exception& e) {
            SendErrorResponse(InRequest.ID, -32603, "Internal error",
                              JSONValue::object({{"details", e.what()}}));
        }
        });

    // Resources handlers
    GetRequestManager().RegisterRequestHandler(
        "resources/list", [this](const RequestBase& InRequest, MCPContext* InContext) {
        try {
            JSONValue paramsJSON;
            if (InRequest.Params.has_value()) {
                to_json(paramsJSON, InRequest.Params.value());
            } else {
                paramsJSON = JSONValue::object();
            }
            HandleResourcesList(paramsJSON, InRequest.ID);
        } catch (const std::exception& e) {
            SendErrorResponse(InRequest.ID, -32603, "Internal error",
                              JSONValue::object({{"details", e.what()}}));
        }
        });

    GetRequestManager().RegisterRequestHandler(
        "resources/read", [this](const RequestBase& InRequest, MCPContext* InContext) {
        try {
            JSONValue paramsJSON;
            if (InRequest.Params.has_value()) {
                to_json(paramsJSON, InRequest.Params.value());
            } else {
                paramsJSON = JSONValue::object();
            }
            HandleResourceRead(paramsJSON, InRequest.ID);
        } catch (const std::exception& e) {
            SendErrorResponse(InRequest.ID, -32603, "Internal error",
                              JSONValue::object({{"details", e.what()}}));
        }
        });

    GetRequestManager().RegisterRequestHandler(
        "resources/subscribe", [this](const RequestBase& InRequest, MCPContext* InContext) {
        try {
            JSONValue paramsJSON;
            if (InRequest.Params.has_value()) {
                to_json(paramsJSON, InRequest.Params.value());
            } else {
                paramsJSON = JSONValue::object();
            }
            HandleResourceSubscribe(paramsJSON, InRequest.ID);
        } catch (const std::exception& e) {
            SendErrorResponse(InRequest.ID, -32603, "Internal error",
                              JSONValue::object({{"details", e.what()}}));
        }
        });

    GetRequestManager().RegisterRequestHandler(
        "resources/unsubscribe", [this](const RequestBase& InRequest, MCPContext* InContext) {
        try {
            JSONValue paramsJSON;
            if (InRequest.Params.has_value()) {
                to_json(paramsJSON, InRequest.Params.value());
            } else {
                paramsJSON = JSONValue::object();
            }
            HandleResourceUnsubscribe(paramsJSON, InRequest.ID);
        } catch (const std::exception& e) {
            SendErrorResponse(InRequest.ID, -32603, "Internal error",
                              JSONValue::object({{"details", e.what()}}));
        }
        });

    // Sampling handler
    GetRequestManager().RegisterRequestHandler(
        "sampling/createMessage", [this](const RequestBase& InRequest, MCPContext* InContext) {
        try {
            JSONValue paramsJSON;
            if (InRequest.Params.has_value()) {
                to_json(paramsJSON, InRequest.Params.value());
            } else {
                paramsJSON = JSONValue::object();
            }
            HandleSamplingCreateMessage(paramsJSON, InRequest.ID);
        } catch (const std::exception& e) {
            SendErrorResponse(InRequest.ID, -32603, "Internal error",
                              JSONValue::object({{"details", e.what()}}));
        }
        });

    // Completion handler
    GetRequestManager().RegisterRequestHandler(
        "completion/complete", [this](const RequestBase& InRequest, MCPContext* InContext) {
        try {
            JSONValue paramsJSON;
            if (InRequest.Params.has_value()) {
                to_json(paramsJSON, InRequest.Params.value());
            } else {
                paramsJSON = JSONValue::object();
            }
            HandleCompletionComplete(paramsJSON, InRequest.ID);
        } catch (const std::exception& e) {
            SendErrorResponse(InRequest.ID, -32603, "Internal error",
                              JSONValue::object({{"details", e.what()}}));
        }
        });
}

void MCPServer::HandleInitialize(const JSONValue& InParams, const RequestID& InRequestID) {
    try {
        auto request = InParams.get<InitializeRequest::Params>();

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

            SendErrorResponse(InRequestID, -32602,
                              "Unsupported protocol version: " + request.ProtocolVersion
                                  + ". Supported versions: " + supportedVersions,
                              JSONValue::object());
            return;
        }

        InitializeResponse::Result response;
        response.ProtocolVersion = request.ProtocolVersion; // Use negotiated version
        response.ServerInfo = m_ServerInfo;

        // Set up capabilities based on what we support
        ServerCapabilities capabilities;

        // Tools capability
        if (m_ToolManager.HasTools()) {
            capabilities.Tools = ToolsCapability{};
            capabilities.Tools->ListChanged = true;
        }

        // Prompts capability
        if (m_PromptManager.HasPrompts()) {
            capabilities.Prompts = PromptsCapability{};
            capabilities.Prompts->ListChanged = true;
        }

        // Resources capability
        if (m_ResourceManager.HasResources()) {
            capabilities.Resources = ResourcesCapability{};
            capabilities.Resources->Subscribe = true;
            capabilities.Resources->ListChanged = true;
        }

        // Logging capability
        capabilities.Logging = LoggingCapability{};

        // Sampling capability if handler is set
        if (m_SamplingHandler) { capabilities.Sampling = SamplingCapability{}; }

        response.Capabilities = capabilities;

        SendResponse(InRequestID, JSONValue(response));

    } catch (const std::exception& e) {
        SendErrorResponse(InRequestID, -32603, "Internal error",
                          JSONValue::object({{"details", e.what()}}));
    }
}

void MCPServer::HandleToolsList(const JSONValue& InParams, const RequestID& InRequestID) {
    try {
        auto request = InParams.get<ListToolsRequest::Params>();

        ListToolsResponse::ListToolsResult response;
        response.Tools = m_ToolManager.ListTools();

        SendResponse(InRequestID, JSONValue(response));

    } catch (const std::exception& e) {
        SendErrorResponse(InRequestID, -32603, "Internal error",
                          JSONValue::object({{"details", e.what()}}));
    }
}

void MCPServer::HandleToolCall(const JSONValue& InParams, const RequestID& InRequestID) {
    try {
        auto request = InParams.get<CallToolRequest::CallToolParams>();

        // Use ToolManager to call the tool
        auto result = m_ToolManager.CallToolSync(request.Name, request.Arguments);

        CallToolResponse::CallToolResult response;
        response.Content = result.Content;
        response.IsError = result.IsError;

        SendResponse(InRequestID, JSONValue(response));

    } catch (const std::exception& e) {
        SendErrorResponse(InRequestID, -32603, "Internal error",
                          JSONValue::object({{"details", e.what()}}));
    }
}

void MCPServer::HandlePromptsList(const JSONValue& InParams, const RequestID& InRequestID) {
    try {
        auto request = InParams.get<ListPromptsRequest::Params>();

        ListPromptsResponse::ListPromptsResult response;
        response.Prompts = m_PromptManager.ListPrompts();

        SendResponse(InRequestID, JSONValue(response));

    } catch (const std::exception& e) {
        SendErrorResponse(InRequestID, -32603, "Internal error",
                          JSONValue::object({{"details", e.what()}}));
    }
}

void MCPServer::HandlePromptGet(const JSONValue& InParams, const RequestID& InRequestID) {
    try {
        auto request = InParams.get<GetPromptRequest::GetPromptParams>();

        // Use PromptManager to get the prompt
        auto result = m_PromptManager.GetPromptSync(request.Name, request.Arguments);

        GetPromptResponse::GetPromptResult response;
        response.Description = result.Description;
        response.Messages = result.Messages;

        SendResponse(InRequestID, JSONValue(response));

    } catch (const std::exception& e) {
        SendErrorResponse(InRequestID, -32603, "Internal error",
                          JSONValue::object({{"details", e.what()}}));
    }
}

void MCPServer::HandleResourcesList(const JSONValue& InParams, const RequestID& InRequestID) {
    try {
        auto request = InParams.get<ListResourcesRequest::Params>();

        ListResourcesResponse::ListResourcesResult response;
        response.Resources = m_ResourceManager.ListResources();

        SendResponse(InRequestID, JSONValue(response));

    } catch (const std::exception& e) {
        SendErrorResponse(InRequestID, -32603, "Internal error",
                          JSONValue::object({{"details", e.what()}}));
    }
}

void MCPServer::HandleResourceRead(const JSONValue& InParams, const RequestID& InRequestID) {
    try {
        auto request = InParams.get<ReadResourceRequest::ReadResourceParams>();

        // Use ResourceManager to read the resource
        auto result = m_ResourceManager.GetResourceSync(request.URI.ToString());

        ReadResourceResponse::ReadResourceResult response;
        response.Contents = result.Contents;

        SendResponse(InRequestID, JSONValue(response));

    } catch (const std::exception& e) {
        SendErrorResponse(InRequestID, -32603, "Internal error",
                          JSONValue::object({{"details", e.what()}}));
    }
}

void MCPServer::HandleResourceSubscribe(const JSONValue& InParams, const RequestID& InRequestID) {
    try {
        auto request = InParams.get<SubscribeRequest::SubscribeParams>();
        std::string uri = request.URI.ToString();
        std::string clientID = GetCurrentClientID();

        // Validate resource exists
        if (!m_ResourceManager.HasResource(uri)) {
            SendErrorResponse(InRequestID, -32601, "Resource not found",
                              JSONValue::object({{"uri", uri}}));
            return;
        }

        // Add subscription with proper client tracking
        {
            std::lock_guard<std::mutex> lock(m_ResourceSubscriptionsMutex);
            m_ResourceSubscriptions[uri].insert(clientID);
        }

        // Send empty response to indicate success
        SendResponse(InRequestID, JSONValue::object());

    } catch (const std::exception& e) {
        SendErrorResponse(InRequestID, -32603, "Internal error",
                          JSONValue::object({{"details", e.what()}}));
    }
}

void MCPServer::HandleResourceUnsubscribe(const JSONValue& InParams, const RequestID& InRequestID) {
    try {
        auto request = InParams.get<UnsubscribeRequest::UnsubscribeParams>();
        std::string uri = request.URI.ToString();
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
        SendResponse(InRequestID, JSONValue::object());

    } catch (const std::exception& e) {
        SendErrorResponse(InRequestID, -32603, "Internal error",
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
        notification.Params.URI = MCP::URI(InURI);

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
    co_await SendNotification("notifications/resources/updated", JSONValue(InNotification));
    co_return;
}

void MCPServer::HandleSamplingCreateMessage(const JSONValue& InParams,
                                            const std::string& InRequestID) {
    try {
        auto request = InParams.get<CreateMessageRequest::CreateMessageParams>();

        if (!m_SamplingHandler) {
            SendErrorResponse(InRequestID, -32601, "Sampling not supported", JSONValue::object());
            return;
        }

        // Call handler
        // auto response = m_SamplingHandler(request);
        // SendResponse(InRequestID, JSONValue(response));

        // TODO: Implement actual sampling handler call

    } catch (const std::exception& e) {
        SendErrorResponse(InRequestID, -32603, "Internal error",
                          JSONValue::object({{"details", e.what()}}));
    }
}

void MCPServer::HandleCompletionComplete(const JSONValue& InParams,
                                         const std::string& InRequestID) {
    try {
        auto request = InParams.get<CompleteRequest::CompleteParams>();

        if (!m_CompletionHandler) {
            SendErrorResponse(InRequestID, -32601, "Completion not supported", JSONValue::object());
            return;
        }

        // Call handler
        // auto response = m_CompletionHandler(request);
        // SendResponse(InRequestID, JSONValue(response));

        // TODO: Implement actual completion handler call

    } catch (const std::exception& e) {
        SendErrorResponse(InRequestID, -32603, "Internal error",
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

// Enhanced tool execution with progress reporting
MCPTask<CallToolResponse> MCPServer::ExecuteToolWithProgress(
    const Tool& InTool,
    const std::optional<std::unordered_map<std::string, JSONValue>>& InArguments,
    const RequestID& InRequestID) {
    try {
        // Update progress at 0%
        co_await UpdateProgress(0.0);

        // Use ToolManager to execute tool
        auto result = co_await m_ToolManager.CallTool(InTool.Name, InArguments);

        // Complete progress
        co_await UpdateProgress(1.0);

        CallToolResponse response(InRequestID, result);
        co_return response;
    } catch (const std::exception& e) {
        // Ensure progress is marked complete even on error
        co_await UpdateProgress(1.0);
        throw;
    }
}

MCPTask_Void MCPServer::UpdateProgress(double InProgress, std::optional<int64_t> InTotal) {
    try {
        ProgressNotification notification;
        notification.Params.ProgressToken = ProgressToken("current_request");
        notification.Params.Progress = InProgress;
        if (InTotal.has_value()) { notification.Params.Total = *InTotal; }

        co_await SendNotification("notifications/progress", JSONValue(notification));
    } catch (const std::exception&) {
        // Ignore progress reporting errors to not break main operation
    }

    co_return;
}

MCPTask_Void MCPServer::CompleteProgress() {
    co_await UpdateProgress(1.0);
    co_return;
}

MCP_NAMESPACE_END