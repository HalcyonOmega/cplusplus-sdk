#pragma once

#include <memory>
#include <unordered_map>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Core/IMCP.h"
#include "CoreSDK/Features/PromptManager.h"
#include "CoreSDK/Features/ResourceManager.h"
#include "CoreSDK/Features/ToolManager.h"
#include "CoreSDK/Messages/MCPMessages.h"

MCP_NAMESPACE_BEGIN

// Server protocol handler
class MCPServer : public MCPProtocol {
  public:
    MCPServer(TransportType InTransportType,
              std::optional<std::unique_ptr<TransportOptions>> InOptions,
              const Implementation& InServerInfo, const ServerCapabilities& InCapabilities);

    // Lifecycle methods
    MCPTask_Void Start() override;
    MCPTask_Void Stop() override;
    bool IsRunning() const;

    // Initialization
    void OnRequest_Initialize(const InitializeRequest& InRequest,
                              std::optional<MCPContext*> InContext);
    void Notify_Initialized();

    // Tool management
    void AddTool(const Tool& InTool);
    void RemoveTool(const Tool& InTool);
    MCPTask_Void Notify_ToolListChanged();
    void OnRequest_ListTools(const ListToolsRequest& InRequest,
                             std::optional<MCPContext*> InContext);
    void OnRequest_CallTool(const CallToolRequest& InRequest, std::optional<MCPContext*> InContext);

    // Prompt management
    void AddPrompt(const Prompt& InPrompt);
    void RemovePrompt(const Prompt& InPrompt);
    MCPTask_Void Notify_PromptListChanged();
    void OnRequest_ListPrompts(const ListPromptsRequest& InRequest,
                               std::optional<MCPContext*> InContext);
    void OnRequest_GetPrompt(const GetPromptRequest& InRequest,
                             std::optional<MCPContext*> InContext);

    // Resource management
    void AddResource(const Resource& InResource);
    void AddResourceTemplate(const ResourceTemplate& InTemplate);
    void RemoveResource(const Resource& InResource);
    void RemoveResourceTemplate(const ResourceTemplate& InTemplate);
    MCPTask_Void Notify_ResourceListChanged();
    MCPTask_Void Notify_ResourceUpdated(const ResourceUpdatedNotification::Params& InParams);
    void OnRequest_ListResources(const ListResourcesRequest& InRequest,
                                 std::optional<MCPContext*> InContext);
    void OnRequest_ReadResource(const ReadResourceRequest& InRequest,
                                std::optional<MCPContext*> InContext);
    void OnRequest_SubscribeResource(const SubscribeRequest& InRequest,
                                     std::optional<MCPContext*> InContext);
    void OnRequest_UnsubscribeResource(const UnsubscribeRequest& InRequest,
                                       std::optional<MCPContext*> InContext);

    // Root management
    void AddRoot(const Root& InRoot);
    void RemoveRoot(const Root& InRoot);
    MCPTask_Void Notify_RootsListChanged();

    // Logging
    MCPTask_Void Notify_LogMessage(const LoggingMessageNotification::Params& InParams);

    // Sampling requests (servers can request sampling from clients)
    MCPTask<CreateMessageResponse::Result>
    Request_CreateMessage(const CreateMessageRequest::Params& InParams);
    void OnRequest_CreateMessage(const CreateMessageRequest& InRequest,
                                 std::optional<MCPContext*> InContext);

    MCPTask_Void Request_Complete(const CompleteRequest& InRequest);
    void OnRequest_Complete(const CompleteRequest& InRequest, std::optional<MCPContext*> InContext);

    // Progress reporting
    MCPTask_Void Notify_Progress(const ProgressNotification::Params& InParams);

    MCPTask_Void CancelRequest(const RequestID& InRequestID, const std::string& InReason = "");

    // Access to managers for MCPContext
    std::weak_ptr<ToolManager> GetToolManager() {
        return m_ToolManager;
    }
    std::weak_ptr<ResourceManager> GetResourceManager() {
        return m_ResourceManager;
    }
    std::weak_ptr<PromptManager> GetPromptManager() {
        return m_PromptManager;
    }

  private:
    // Server state
    bool m_IsRunning{false};

    // Managers for handling MCP features
    std::shared_ptr<ToolManager> m_ToolManager;
    std::shared_ptr<PromptManager> m_PromptManager;
    std::shared_ptr<ResourceManager> m_ResourceManager;

    // Root management
    std::vector<Root> m_Roots;
    mutable std::mutex m_RootsMutex;

    // Feature handlers
    // TODO: @HalcyonOmega Cleanup, fix, or remove these handlers
    std::function<void()> m_SamplingHandler;
    std::function<void()> m_CompletionHandler;

    // Handler management
    mutable std::mutex m_HandlersMutex;

    // Setup methods
    void SetupDefaultHandlers();

    // Request handlers that delegate to managers

    // Progress tracking and tool execution
    MCPTask<CallToolResponse> ExecuteToolWithProgress(
        const Tool& InTool,
        const std::optional<std::unordered_map<std::string, JSONData>>& InArguments,
        const RequestID& InRequestID);

    // Resource subscription management
    MCPTask_Void Notify_ResourceSubscribers(const Resource& InResource);
    std::string_view GetCurrentClientID() const;
    MCPTask_Void SendNotificationToClient(std::string_view InClientID,
                                          const ResourceUpdatedNotification& InNotification);

    std::unordered_map<std::string /* Resource */, std::vector<std::string> /* Client IDs */>
        m_ResourceSubscriptions;
    mutable std::mutex m_ResourceSubscriptionsMutex;

    MCPTask_Void UpdateProgress(double InProgress, std::optional<int64_t> InTotal = {});
    MCPTask_Void CompleteProgress();
};

MCP_NAMESPACE_END