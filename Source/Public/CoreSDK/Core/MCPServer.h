#pragma once

#include <memory>
#include <unordered_map>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Core/IMCP.h"
#include "CoreSDK/Features/FeatureSignatures.h"
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

    void HandleInitializeRequest(const InitializeRequest& InRequest);
    void NotifyInitialized();

    // Tool management
    void AddTool(const Tool& InTool);
    void RemoveTool(const Tool& InTool);
    MCPTask_Void NotifyToolListChanged();

    // Prompt management
    void AddPrompt(const Prompt& InPrompt);
    void RemovePrompt(const Prompt& InPrompt);
    MCPTask_Void NotifyPromptListChanged();

    // Resource management
    void AddResource(const Resource& InResource);
    void AddResourceTemplate(const ResourceTemplate& InTemplate);
    void RemoveResource(const Resource& InResource);
    void RemoveResourceTemplate(const ResourceTemplate& InTemplate);
    MCPTask_Void NotifyResourceListChanged();
    MCPTask_Void NotifyResourceUpdated(const ResourceUpdatedNotification::Params& InParams);

    // Root management
    void AddRoot(const Root& InRoot);
    void RemoveRoot(const Root& InRoot);
    MCPTask_Void NotifyRootsListChanged();

    // Logging
    MCPTask_Void LogMessage(const LoggingMessageNotification::Params& InParams);

    // Sampling requests (servers can request sampling from clients)
    MCPTask<CreateMessageResponse::Result>
    RequestSampling(const CreateMessageRequest::Params& InParams);

    // Progress reporting
    MCPTask_Void ReportProgress(const ProgressNotification::Params& InParams);

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
    void HandleInitialize(const InitializeRequest& InRequest, MCPContext* InContext);
    void HandleToolsList(const ListToolsRequest& InRequest, MCPContext* InContext);
    void HandleToolCall(const CallToolRequest& InRequest, MCPContext* InContext);
    void HandlePromptsList(const ListPromptsRequest& InRequest, MCPContext* InContext);
    void HandlePromptGet(const GetPromptRequest& InRequest, MCPContext* InContext);
    void HandleResourcesList(const ListResourcesRequest& InRequest, MCPContext* InContext);
    void HandleResourceRead(const ReadResourceRequest& InRequest, MCPContext* InContext);
    void HandleResourceSubscribe(const SubscribeRequest& InRequest, MCPContext* InContext);
    void HandleResourceUnsubscribe(const UnsubscribeRequest& InRequest, MCPContext* InContext);
    void HandleSamplingCreateMessage(const CreateMessageRequest& InRequest, MCPContext* InContext);
    void HandleCompletionComplete(const CompleteRequest& InRequest, MCPContext* InContext);

    // Progress tracking and tool execution
    MCPTask<CallToolResponse> ExecuteToolWithProgress(
        const Tool& InTool,
        const std::optional<std::unordered_map<std::string, JSONValue>>& InArguments,
        const RequestID& InRequestID);

    // Resource subscription management
    MCPTask_Void NotifyResourceSubscribers(const Resource& InResource);
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