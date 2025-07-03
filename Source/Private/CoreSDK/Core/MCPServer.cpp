#include "CoreSDK/Core/MCPServer.h"

#include "CoreSDK/Common/Content.h"
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

void MCPServer::OnRequest_Initialize(const InitializeRequest& InRequest) {
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
                if (i > 0) { supportedVersions += ", "; }
                supportedVersions += SUPPORTED_PROTOCOL_VERSIONS[i];
            }

            SendErrorResponse(ErrorResponseBase(
                InRequest.GetRequestID(),
                MCPError(ErrorCodes::INVALID_REQUEST,
                         "Unsupported protocol version: " + Request.ProtocolVersion
                             + ". Supported versions: " + supportedVersions)));
            return;
        }

        InitializeResponse::Result Result;
        Result.ProtocolVersion = Request.ProtocolVersion; // Use negotiated version
        Result.ServerInfo = m_ServerInfo;

        // Set up capabilities based on what we support
        ServerCapabilities capabilities;

        // Tools capability
        if (m_ToolManager->ListTools().size() > 0) {
            capabilities.Tools = ToolsCapability{};
            capabilities.Tools->ListChanged = true;
        }

        // Prompts capability
        if (m_PromptManager->ListPrompts().size() > 0) {
            capabilities.Prompts = PromptsCapability{};
            capabilities.Prompts->ListChanged = true;
        }

        // Resources capability
        if (m_ResourceManager->ListResources().size() > 0) {
            capabilities.Resources = ResourcesCapability{};
            capabilities.Resources->Subscribe = true;
            capabilities.Resources->ListChanged = true;
        }

        // Logging capability
        capabilities.Logging = LoggingCapability{};

        Result.Capabilities = capabilities;

        SendResponse(ResponseBase(InRequest.GetRequestID(), Result));

    } catch (const std::exception& e) {
        SendErrorResponse(ErrorResponseBase(InRequest.GetRequestID(),
                                            MCPError(ErrorCodes::INTERNAL_ERROR, e.what())));
    }
}

void MCPServer::Notify_Initialized() {
    SendNotification(InitializedNotification());
}

bool MCPServer::AddTool(const Tool& InTool, const ToolManager::ToolFunction& InFunction) {
    if (!m_ToolManager->AddTool(InTool, InFunction)) {
        HandleRuntimeError("Failed to add tool: " + InTool.Name);
        return false;
    }

    Notify_ToolListChanged();
    return true;
}

bool MCPServer::RemoveTool(const Tool& InTool) {
    if (!m_ToolManager->RemoveTool(InTool)) {
        HandleRuntimeError("Failed to remove tool: " + InTool.Name);
        return false;
    }

    Notify_ToolListChanged();
    return true;
}

MCPTask_Void MCPServer::Notify_ToolListChanged() {
    co_await SendNotification(ToolListChangedNotification());
}

void MCPServer::OnRequest_ListTools(const ListToolsRequest& InRequest) {
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

void MCPServer::OnRequest_CallTool(const CallToolRequest& InRequest) {
    try {
        CallToolRequest::Params Request =
            GetRequestParams<CallToolRequest::Params>(InRequest).value();
        Tool Tool = m_ToolManager->GetTool(Request.Name).value();

        // Use ToolManager to call the tool
        auto Result = m_ToolManager->CallTool(Tool, Request.Arguments);

        CallToolResponse::Result ResponseResult;
        ResponseResult.Content = Result.Content;
        ResponseResult.IsError = Result.IsError;

        SendResponse(ResponseBase(InRequest.GetRequestID(), ResponseResult));

    } catch (const std::exception& e) {
        SendErrorResponse(ErrorResponseBase(InRequest.GetRequestID(),
                                            MCPError(ErrorCodes::INTERNAL_ERROR, e.what())));
    }
}

bool MCPServer::AddPrompt(const Prompt& InPrompt) {
    if (!m_PromptManager->AddPrompt(InPrompt)) {
        HandleRuntimeError("Failed to add prompt: " + InPrompt.Name);
        return false;
    }

    Notify_PromptListChanged();
    return true;
}

bool MCPServer::RemovePrompt(const Prompt& InPrompt) {
    if (!m_PromptManager->RemovePrompt(InPrompt)) {
        HandleRuntimeError("Failed to remove prompt: " + InPrompt.Name);
        return false;
    }

    Notify_PromptListChanged();
    return true;
}

MCPTask_Void MCPServer::Notify_PromptListChanged() {
    co_await SendNotification(PromptListChangedNotification());
}

void MCPServer::OnRequest_ListPrompts(const ListPromptsRequest& InRequest) {
    try {
        PaginatedRequestParams RequestParams =
            GetRequestParams<PaginatedRequestParams>(InRequest).value();

        ListPromptsResponse::Result Result;
        Result.Prompts = m_PromptManager->ListPrompts();
        Result.Cursor = RequestParams.Cursor;

        SendResponse(ResponseBase(InRequest.GetRequestID(), Result));

    } catch (const std::exception& e) {
        SendErrorResponse(ErrorResponseBase(InRequest.GetRequestID(),
                                            MCPError(ErrorCodes::INTERNAL_ERROR, e.what())));
    }
}

void MCPServer::OnRequest_GetPrompt(const GetPromptRequest& InRequest) {
    try {
        GetPromptRequest::Params Request =
            GetRequestParams<GetPromptRequest::Params>(InRequest).value();

        // Use PromptManager to get the prompt
        auto Result = m_PromptManager->GetPrompt(Request.Name, Request.Arguments);

        GetPromptResponse::Result ResponseResult;
        ResponseResult.Description = Result.Description;
        ResponseResult.Messages = Result.Messages;

        SendResponse(ResponseBase(InRequest.GetRequestID(), ResponseResult));

    } catch (const std::exception& e) {
        SendErrorResponse(ErrorResponseBase(InRequest.GetRequestID(),
                                            MCPError(ErrorCodes::INTERNAL_ERROR, e.what())));
    }
}

bool MCPServer::AddResource(const Resource& InResource) {
    if (!m_ResourceManager->AddResource(InResource)) {
        HandleRuntimeError("Failed to add resource: " + InResource.URI.toString());
        return false;
    }

    Notify_ResourceListChanged();
    return true;
}

bool MCPServer::AddResourceTemplate(const ResourceTemplate& InTemplate,
                                    const ResourceManager::ResourceFunction& InFunction) {
    if (!m_ResourceManager->AddTemplate(InTemplate, InFunction)) {
        HandleRuntimeError("Failed to add resource template: " + InTemplate.Name);
        return false;
    }

    Notify_ResourceListChanged();
    return true;
}

bool MCPServer::RemoveResource(const Resource& InResource) {
    if (!m_ResourceManager->RemoveResource(InResource)) {
        HandleRuntimeError("Failed to remove resource: " + InResource.URI.toString());
        return false;
    }

    Notify_ResourceListChanged();
    return true;
}

bool MCPServer::RemoveResourceTemplate(const ResourceTemplate& InTemplate) {
    if (!m_ResourceManager->RemoveTemplate(InTemplate)) {
        HandleRuntimeError("Failed to remove resource template: " + InTemplate.Name);
        return false;
    }

    Notify_ResourceListChanged();
    return true;
}

MCPTask_Void MCPServer::Notify_ResourceListChanged() {
    co_await SendNotification(ResourceListChangedNotification());
}

MCPTask_Void
MCPServer::Notify_ResourceUpdated(const ResourceUpdatedNotification::Params& InParams) {
    co_await SendNotification(ResourceUpdatedNotification(InParams));
}

void MCPServer::OnRequest_ListResources(const ListResourcesRequest& InRequest) {
    try {
        auto Request = GetRequestParams<ListResourcesRequest::Params>(InRequest);

        ListResourcesResponse::Result Result;
        Result.Resources = m_ResourceManager->ListResources();

        SendResponse(ResponseBase(InRequest.GetRequestID(), Result));

    } catch (const std::exception& e) {
        SendErrorResponse(ErrorResponseBase(InRequest.GetRequestID(),
                                            MCPError(ErrorCodes::INTERNAL_ERROR, e.what())));
    }
}

void MCPServer::OnRequest_ReadResource(const ReadResourceRequest& InRequest) {
    try {
        ReadResourceRequest::Params Request =
            GetRequestParams<ReadResourceRequest::Params>(InRequest).value();

        // Use ResourceManager to read the resource content
        std::variant<TextResourceContents, BlobResourceContents> ResourceContent =
            m_ResourceManager->GetResource(Request.URI).value();

        ReadResourceResponse::Result ResponseResult;
        ResponseResult.Contents.emplace_back(ResourceContent);

        SendResponse(ResponseBase(InRequest.GetRequestID(), ResponseResult));

    } catch (const std::exception& e) {
        SendErrorResponse(ErrorResponseBase(InRequest.GetRequestID(),
                                            MCPError(ErrorCodes::INTERNAL_ERROR, e.what())));
    }
}

void MCPServer::OnRequest_SubscribeResource(const SubscribeRequest& InRequest) {
    try {
        SubscribeRequest::Params Request =
            GetRequestParams<SubscribeRequest::Params>(InRequest).value();

        std::string_view clientID = GetCurrentClientID();

        // Validate resource exists
        if (!m_ResourceManager->HasResource(Request.URI)) {
            SendErrorResponse(
                ErrorResponseBase(InRequest.GetRequestID(),
                                  MCPError(ErrorCodes::INVALID_REQUEST, "Resource not found",
                                           JSONData::object({{"uri", Request.URI.toString()}}))));
            return;
        }

        // Add subscription with proper client tracking
        {
            std::lock_guard<std::mutex> lock(m_ResourceSubscriptionsMutex);
            m_ResourceSubscriptions[Request.URI.toString()].emplace_back(clientID);
        }

        // Send empty response to indicate success
        SendResponse(ResponseBase(InRequest.GetRequestID(), JSONData::object()));

    } catch (const std::exception& e) {
        SendErrorResponse(ErrorResponseBase(InRequest.GetRequestID(),
                                            MCPError(ErrorCodes::INTERNAL_ERROR, e.what())));
    }
}

void MCPServer::OnRequest_UnsubscribeResource(const UnsubscribeRequest& InRequest) {
    try {
        UnsubscribeRequest::Params Request =
            GetRequestParams<UnsubscribeRequest::Params>(InRequest).value();

        std::string_view clientID = GetCurrentClientID();

        // Remove from subscriptions
        {
            std::lock_guard<std::mutex> lock(m_ResourceSubscriptionsMutex);
            auto iter = m_ResourceSubscriptions.find(Request.URI.toString());
            if (iter != m_ResourceSubscriptions.end()) {
                auto& Subscribers = iter->second;
                std::erase(Subscribers, std::string(clientID));
                // Clean up empty subscription sets
                if (Subscribers.empty()) { m_ResourceSubscriptions.erase(iter); }
            }
        }

        // Send empty response
        SendResponse(ResponseBase(InRequest.GetRequestID(), JSONData::object()));

    } catch (const std::exception& e) {
        SendErrorResponse(ErrorResponseBase(InRequest.GetRequestID(),
                                            MCPError(ErrorCodes::INTERNAL_ERROR, e.what())));
    }
}

bool MCPServer::AddRoot(const Root& InRoot) {
    if (!m_RootManager->AddRoot(InRoot)) {
        HandleRuntimeError("Failed to add root: " + InRoot.Name);
        return false;
    }

    Notify_RootsListChanged();
    return true;
}

bool MCPServer::RemoveRoot(const Root& InRoot) {
    if (!m_RootManager->RemoveRoot(InRoot)) {
        HandleRuntimeError("Failed to remove root: " + InRoot.Name);
        return false;
    }

    Notify_RootsListChanged();
    return true;
}

MCPTask_Void MCPServer::Notify_RootsListChanged() {
    co_await SendNotification(RootsListChangedNotification());
}

MCPTask_Void MCPServer::Notify_LogMessage(const LoggingMessageNotification::Params& InParams) {
    co_await SendNotification(LoggingMessageNotification(InParams));
}

MCPTask<CreateMessageResponse::Result>
MCPServer::Request_CreateMessage(const CreateMessageRequest::Params& InParams) {
    CreateMessageResponse Response = co_await SendRequest(CreateMessageRequest(InParams));
    co_return GetResponseResult<CreateMessageResponse::Result>(Response);
}

void MCPServer::OnRequest_Complete(const CompleteRequest& InRequest) {
    try {
        CompleteRequest::Params Request =
            GetRequestParams<CompleteRequest::Params>(InRequest).value();

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

MCPTask_Void MCPServer::Notify_Progress(const ProgressNotification::Params& InParams) {
    co_await SendNotification(ProgressNotification(InParams));
}

MCPTask_Void MCPServer::Notify_CancelRequest(const CancelledNotification::Params& InParams) {
    co_await SendNotification(CancelledNotification(InParams));
}

void MCPServer::OnNotified_Progress(const ProgressNotification& InNotification) {
    // Ensure this method requires instance access
    if (!IsInitialized()) { return; }

    ProgressNotification::Params Params =
        GetNotificationParams<ProgressNotification::Params>(InNotification).value();

    // TODO: Implement progress notification handling
    (void)Params;
}

void MCPServer::OnNotified_CancelRequest(const CancelledNotification& InNotification) {
    // Ensure this method requires instance access
    if (!IsInitialized()) { return; }

    (void)InNotification;
    // TODO: Implement cancel request handling
}

void MCPServer::SetupDefaultHandlers() {
    // Register request handlers using the RegisterRequestHandler method
    m_MessageManager->RegisterRequestHandler<InitializeRequest>(
        [this](const auto& request) { return OnRequest_Initialize(request); });
    m_MessageManager->RegisterRequestHandler<ListToolsRequest>(
        [this](const auto& request) { return OnRequest_ListTools(request); });
    m_MessageManager->RegisterRequestHandler<CallToolRequest>(
        [this](const auto& request) { return OnRequest_CallTool(request); });
    m_MessageManager->RegisterRequestHandler<ListPromptsRequest>(
        [this](const auto& request) { return OnRequest_ListPrompts(request); });
    m_MessageManager->RegisterRequestHandler<GetPromptRequest>(
        [this](const auto& request) { return OnRequest_GetPrompt(request); });
    m_MessageManager->RegisterRequestHandler<ListResourcesRequest>(
        [this](const auto& request) { return OnRequest_ListResources(request); });
    m_MessageManager->RegisterRequestHandler<ReadResourceRequest>(
        [this](const auto& request) { return OnRequest_ReadResource(request); });
    m_MessageManager->RegisterRequestHandler<SubscribeRequest>(
        [this](const auto& request) { return OnRequest_SubscribeResource(request); });
    m_MessageManager->RegisterRequestHandler<UnsubscribeRequest>(
        [this](const auto& request) { return OnRequest_UnsubscribeResource(request); });
    m_MessageManager->RegisterRequestHandler<CompleteRequest>(
        [this](const auto& request) { return OnRequest_Complete(request); });
}

// Enhanced resource change notification
MCPTask_Void MCPServer::Notify_ResourceSubscribers(const Resource& InResource) {
    std::vector<std::string> subscribers;

    {
        std::lock_guard<std::mutex> lock(m_ResourceSubscriptionsMutex);
        auto iter = m_ResourceSubscriptions.find(InResource.URI.toString());
        if (iter != m_ResourceSubscriptions.end()) { subscribers = iter->second; }
    }

    if (!subscribers.empty()) {
        ResourceUpdatedNotification notification;
        notification.Params.URI = InResource.URI;

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
std::string_view MCPServer::GetCurrentClientID() const {
    // For now, return a default client ID
    // In a real implementation, this would extract the client ID from the current transport session
    return "default_client";
}

// Send notification to specific client (simplified implementation)
// TODO: @HalcyonOmega - Implement this
MCPTask_Void
MCPServer::SendNotificationToClient(std::string_view InClientID,
                                    const ResourceUpdatedNotification& InNotification) {
    (void)InClientID;
    // For now, send to all clients via the protocol
    // In a real implementation, this would route to the specific client
    co_return co_await SendNotification(InNotification);
}

// Enhanced tool execution with progress reporting
MCPTask<CallToolResponse> MCPServer::ExecuteToolWithProgress(
    const Tool& InTool, const std::optional<std::unordered_map<std::string, JSONData>>& InArguments,
    const RequestID& InRequestID) {
    // Update progress at 0%
    co_await UpdateProgress(0.0);

    // Use ToolManager to execute tool
    auto result = co_await m_ToolManager->CallTool(InTool, InArguments);

    // Complete progress
    co_await UpdateProgress(1.0);

    CallToolResponse response(InRequestID, result);
    co_return response;
}

MCPTask_Void MCPServer::UpdateProgress(double InProgress, std::optional<int64_t> InTotal) {
    try {
        ProgressNotification::Params Params;
        Params.ProgressToken = ProgressToken(std::string("current_request"));
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

MCP_NAMESPACE_END