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

MCPTask_Void MCPServer::NotifyResourceUpdated(const ResourceUpdatedNotification::Params& InParams) {
    co_await SendNotification(ResourceUpdatedNotification(InParams));
}

MCPTask_Void MCPServer::NotifyResourceListChanged() {
    co_await SendNotification(ResourceListChangedNotification());
}

MCPTask_Void MCPServer::NotifyToolListChanged() {
    co_await SendNotification(ToolListChangedNotification());
}

MCPTask_Void MCPServer::NotifyPromptListChanged() {
    co_await SendNotification(PromptListChangedNotification());
}

MCPTask_Void MCPServer::ReportProgress(const ProgressNotification::Params& InParams) {
    co_await SendNotification(ProgressNotification(InParams));
}

MCPTask_Void MCPServer::LogMessage(const LoggingMessageNotification::Params& InParams) {
    co_await SendNotification(LoggingMessageNotification(InParams));
}

void MCPServer::SetupDefaultHandlers() {
    // Register request handlers using the RegisterRequestHandler method
    m_MessageManager->RegisterRequestHandler<InitializeRequest>(
        [this](const auto& request, std::optional<MCPContext*> InContext) {
            return HandleInitialize(request, InContext);
        });
    m_MessageManager->RegisterRequestHandler<ListToolsRequest>(
        [this](const auto& request, std::optional<MCPContext*> InContext) {
            return HandleToolsList(request, InContext);
        });
    m_MessageManager->RegisterRequestHandler<CallToolRequest>(
        [this](const auto& request, std::optional<MCPContext*> InContext) {
            return HandleToolCall(request, InContext);
        });
    m_MessageManager->RegisterRequestHandler<ListPromptsRequest>(
        [this](const auto& request, std::optional<MCPContext*> InContext) {
            return HandlePromptsList(request, InContext);
        });
    m_MessageManager->RegisterRequestHandler<GetPromptRequest>(
        [this](const auto& request, std::optional<MCPContext*> InContext) {
            return HandlePromptGet(request, InContext);
        });
    m_MessageManager->RegisterRequestHandler<ListResourcesRequest>(
        [this](const auto& request, std::optional<MCPContext*> InContext) {
            return HandleResourcesList(request, InContext);
        });
    m_MessageManager->RegisterRequestHandler<ReadResourceRequest>(
        [this](const auto& request, std::optional<MCPContext*> InContext) {
            return HandleResourceRead(request, InContext);
        });
    m_MessageManager->RegisterRequestHandler<SubscribeRequest>(
        [this](const auto& request, std::optional<MCPContext*> InContext) {
            return HandleResourceSubscribe(request, InContext);
        });
    m_MessageManager->RegisterRequestHandler<UnsubscribeRequest>(
        [this](const auto& request, std::optional<MCPContext*> InContext) {
            return HandleResourceUnsubscribe(request, InContext);
        });
    m_MessageManager->RegisterRequestHandler<CreateMessageRequest>(
        [this](const auto& request, std::optional<MCPContext*> InContext) {
            return HandleSamplingCreateMessage(request, InContext);
        });
    m_MessageManager->RegisterRequestHandler<CompleteRequest>(
        [this](const auto& request, std::optional<MCPContext*> InContext) {
            return HandleCompletionComplete(request, InContext);
        });
}

void MCPServer::HandleInitialize(const InitializeRequest& InRequest,
                                 std::optional<MCPContext*> InContext) {
    (void)InContext;
    try {
        InitializeRequest::Params Request =
            GetRequestParams<InitializeRequest::Params>(InRequest).value();

        // CRITICAL: Validate protocol version first
        static const std::vector<std::string> SUPPORTED_PROTOCOL_VERSIONS = {"2024-11-05",
                                                                             "2025-03-26"};

        auto iter = std::find(SUPPORTED_PROTOCOL_VERSIONS.begin(),
                              SUPPORTED_PROTOCOL_VERSIONS.end(), Request.ProtocolVersion);

        if (iter == SUPPORTED_PROTOCOL_VERSIONS.end()) {
            std::string supportedVersions;
            for (size_t i = 0; i < SUPPORTED_PROTOCOL_VERSIONS.size(); ++i) {
                if (i > 0) supportedVersions += ", ";
                supportedVersions += SUPPORTED_PROTOCOL_VERSIONS[i];
            }

            SendErrorResponse(InRequestID, -32602,
                              "Unsupported protocol version: " + Request.ProtocolVersion
                                  + ". Supported versions: " + supportedVersions,
                              JSONData::object());
            return;
        }

        InitializeResponse::Result Result;
        Result.ProtocolVersion = Request.ProtocolVersion; // Use negotiated version
        Result.ServerInfo = m_ServerInfo;

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

        Result.Capabilities = capabilities;

        SendResponse(ResponseBase(InRequest.GetRequestID(), Result));

    } catch (const std::exception& e) {
        SendErrorResponse(ErrorResponseBase(InRequest.GetRequestID(),
                                            MCPError(ErrorCodes::INTERNAL_ERROR, e.what())));
    }
}

void MCPServer::HandleToolsList(const ListToolsRequest& InRequest,
                                std::optional<MCPContext*> InContext) {
    (void)InContext;
    try {
        auto RequestParams = GetRequestParams<PaginatedRequestParams>(InRequest);

        ListToolsResponse::Result Result;
        Result.Tools = m_ToolManager->ListTools();

        SendResponse(ResponseBase(InRequest.GetRequestID(), Result));

    } catch (const std::exception& e) {
        SendErrorResponse(ErrorResponseBase(InRequest.GetRequestID(),
                                            MCPError(ErrorCodes::INTERNAL_ERROR, e.what())));
    }
}

void MCPServer::HandleToolCall(const CallToolRequest& InRequest,
                               std::optional<MCPContext*> InContext) {
    (void)InContext;
    try {
        auto Request = GetRequestParams<CallToolRequest::Params>(InRequest);

        // Use ToolManager to call the tool
        auto Result = m_ToolManager->CallToolSync(Request.Name, Request.Arguments);

        CallToolResponse::Result ResponseResult;
        ResponseResult.Content = Result.Content;
        ResponseResult.IsError = Result.IsError;

        SendResponse(ResponseBase(InRequest.GetRequestID(), ResponseResult));

    } catch (const std::exception& e) {
        SendErrorResponse(ErrorResponseBase(InRequest.GetRequestID(),
                                            MCPError(ErrorCodes::INTERNAL_ERROR, e.what())));
    }
}

void MCPServer::HandlePromptsList(const ListPromptsRequest& InRequest,
                                  std::optional<MCPContext*> InContext) {
    (void)InContext;
    try {
        auto Request = GetRequestParams<ListPromptsRequest::Params>(InRequest);

        ListPromptsResponse::Result Result;
        Result.Prompts = m_PromptManager.ListPrompts();

        SendResponse(ResponseBase(InRequest.GetRequestID(), Result));

    } catch (const std::exception& e) {
        SendErrorResponse(ErrorResponseBase(InRequest.GetRequestID(),
                                            MCPError(ErrorCodes::INTERNAL_ERROR, e.what())));
    }
}

void MCPServer::HandlePromptGet(const GetPromptRequest& InRequest,
                                std::optional<MCPContext*> InContext) {
    (void)InContext;
    try {
        auto Request = GetRequestParams<GetPromptRequest::Params>(InRequest);

        // Use PromptManager to get the prompt
        auto Result = m_PromptManager->GetPromptSync(Request.Name, Request.Arguments);

        GetPromptResponse::Result ResponseResult;
        ResponseResult.Description = Result.Description;
        ResponseResult.Messages = Result.Messages;

        SendResponse(ResponseBase(InRequest.GetRequestID(), ResponseResult));

    } catch (const std::exception& e) {
        SendErrorResponse(ErrorResponseBase(InRequest.GetRequestID(),
                                            MCPError(ErrorCodes::INTERNAL_ERROR, e.what())));
    }
}

void MCPServer::HandleResourcesList(const ListResourcesRequest& InRequest,
                                    std::optional<MCPContext*> InContext) {
    (void)InContext;
    try {
        auto Request = GetRequestParams<ListResourcesRequest::Params>(InRequest);

        ListResourcesResponse::Result Result;
        Result.Resources = m_ResourceManager.ListResources();

        SendResponse(ResponseBase(InRequest.GetRequestID(), Result));

    } catch (const std::exception& e) {
        SendErrorResponse(ErrorResponseBase(InRequest.GetRequestID(),
                                            MCPError(ErrorCodes::INTERNAL_ERROR, e.what())));
    }
}

void MCPServer::HandleResourceRead(const ReadResourceRequest& InRequest,
                                   std::optional<MCPContext*> InContext) {
    (void)InContext;
    try {
        auto Request = GetRequestParams<ReadResourceRequest::Params>(InRequest);

        // Use ResourceManager to read the resource
        auto Result = m_ResourceManager.GetResourceSync(Request.URI.ToString());

        ReadResourceResponse::Result ResponseResult;
        ResponseResult.Contents = Result.Contents;

        SendResponse(ResponseBase(InRequest.GetRequestID(), ResponseResult));

    } catch (const std::exception& e) {
        SendErrorResponse(ErrorResponseBase(InRequest.GetRequestID(),
                                            MCPError(ErrorCodes::INTERNAL_ERROR, e.what())));
    }
}

void MCPServer::HandleResourceSubscribe(const SubscribeRequest& InRequest,
                                        std::optional<MCPContext*> InContext) {
    (void)InContext;
    try {
        auto Request = GetRequestParams<SubscribeRequest::Params>(InRequest);
        std::string uri = Request.URI.ToString();
        std::string clientID = GetCurrentClientID();

        // Validate resource exists
        if (!m_ResourceManager.HasResource(uri)) {
            SendErrorResponse(
                ErrorResponseBase(InRequest.GetRequestID(),
                                  MCPError(ErrorCodes::INVALID_REQUEST, "Resource not found",
                                           JSONData::object({{"uri", uri}}))));
            return;
        }

        // Add subscription with proper client tracking
        {
            std::lock_guard<std::mutex> lock(m_ResourceSubscriptionsMutex);
            m_ResourceSubscriptions[uri].insert(clientID);
        }

        // Send empty response to indicate success
        SendResponse(ResponseBase(InRequest.GetRequestID(), JSONData::object()));

    } catch (const std::exception& e) {
        SendErrorResponse(ErrorResponseBase(InRequest.GetRequestID(),
                                            MCPError(ErrorCodes::INTERNAL_ERROR, e.what())));
    }
}

void MCPServer::HandleResourceUnsubscribe(const UnsubscribeRequest& InRequest,
                                          std::optional<MCPContext*> InContext) {
    (void)InContext;
    try {
        auto Request = GetRequestParams<UnsubscribeRequest::Params>(InRequest);
        std::string uri = Request.URI.ToString();
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
        SendResponse(ResponseBase(InRequest.GetRequestID(), JSONData::object()));

    } catch (const std::exception& e) {
        SendErrorResponse(ErrorResponseBase(InRequest.GetRequestID(),
                                            MCPError(ErrorCodes::INTERNAL_ERROR, e.what())));
    }
}

void MCPServer::HandleSamplingCreateMessage(const CreateMessageRequest& InRequest,
                                            std::optional<MCPContext*> InContext) {
    (void)InContext;
    try {
        auto Request = GetRequestParams<CreateMessageRequest::Params>(InRequest);

        if (!m_SamplingHandler) {
            SendErrorResponse(
                ErrorResponseBase(InRequest.GetRequestID(), MCPError(ErrorCodes::METHOD_NOT_FOUND,
                                                                     "Sampling not supported")));
            return;
        }

        // Call handler
        // auto response = m_SamplingHandler(request);
        // SendResponse(InRequestID, JSONData(response));

        // TODO: Implement actual sampling handler call

    } catch (const std::exception& e) {
        SendErrorResponse(ErrorResponseBase(InRequest.GetRequestID(),
                                            MCPError(ErrorCodes::INTERNAL_ERROR, e.what())));
    }
}

void MCPServer::HandleCompletionComplete(const CompleteRequest& InRequest,
                                         std::optional<MCPContext*> InContext) {
    (void)InContext;
    try {
        auto Request = GetRequestParams<CompleteRequest::Params>(InRequest);

        if (!m_CompletionHandler) {
            SendErrorResponse(
                ErrorResponseBase(InRequest.GetRequestID(), MCPError(ErrorCodes::METHOD_NOT_FOUND,
                                                                     "Completion not supported")));
            return;
        }

        // Call handler
        // auto response = m_CompletionHandler(request);
        // SendResponse(InRequestID, JSONData(response));

        // TODO: Implement actual completion handler call

    } catch (const std::exception& e) {
        SendErrorResponse(ErrorResponseBase(InRequest.GetRequestID(),
                                            MCPError(ErrorCodes::INTERNAL_ERROR, e.what())));
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
// TODO: @HalcyonOmega - Implement this
std::string MCPServer::GetCurrentClientID() const {
    // For now, return a default client ID
    // In a real implementation, this would extract the client ID from the current transport session
    return "default_client";
}

// Send notification to specific client (simplified implementation)
// TODO: @HalcyonOmega - Implement this
MCPTask_Void
MCPServer::SendNotificationToClient(const std::string& InClientID,
                                    const ResourceUpdatedNotification& InNotification) {
    // For now, send to all clients via the protocol
    // In a real implementation, this would route to the specific client
    co_await SendNotification(InNotification);
    co_return;
}

// Enhanced tool execution with progress reporting
MCPTask<CallToolResponse> MCPServer::ExecuteToolWithProgress(
    const Tool& InTool, const std::optional<std::unordered_map<std::string, JSONData>>& InArguments,
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
        ProgressNotification::Params Params;
        Params.ProgressToken = ProgressToken("current_request");
        Params.Progress = InProgress;
        if (InTotal.has_value()) { Params.Total = *InTotal; }

        co_await SendNotification(ProgressNotification(Params));
    } catch (const std::exception&) {
        // Ignore progress reporting errors to not break main operation
    }

    co_return;
}

MCPTask_Void MCPServer::CompleteProgress() {
    co_await UpdateProgress(1.0);
    co_return;
}

MCPTask<CreateMessageResponse::Result>
MCPServer::RequestSampling(const CreateMessageRequest::Params& InParams) {
    auto Response = co_await SendRequest<CreateMessageResponse>(CreateMessageRequest(InParams));
    co_return Response.Result.value();
}

MCP_NAMESPACE_END