#pragma once

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Core/IMCP.h"
#include "CoreSDK/Features/FeatureSignatures.h"
#include "CoreSDK/Features/PromptManager.h"
#include "CoreSDK/Features/ResourceManager.h"
#include "CoreSDK/Features/ToolManager.h"

MCP_NAMESPACE_BEGIN

// Server protocol handler
class MCPServer : public MCPProtocol {
  public:
    MCPServer(TransportType InTransportType,
              std::optional<std::unique_ptr<TransportOptions>> InOptions,
              const Implementation& InServerInfo, const ServerCapabilities& InCapabilities);

    void HandleInitializeRequest(const InitializeRequest& InRequest);
    void NotifyInitialized();

    // Tool management
    void AddTool(const Tool& InTool, ToolHandler InHandler);
    void RemoveTool(const Tool& InTool);
    MCPTask_Void NotifyToolListChanged();

    // Prompt management
    void AddPrompt(const Prompt& InPrompt, PromptHandler InHandler);
    void RemovePrompt(const Prompt& InPrompt);
    MCPTask_Void NotifyPromptListChanged();

    // Resource management
    void AddResource(const Resource& InResource, ResourceHandler InHandler);
    void AddResourceTemplate(const ResourceTemplate& InTemplate, ResourceTemplateHandler InHandler);
    void RemoveResource(const Resource& InResource);
    void RemoveResourceTemplate(const ResourceTemplate& InTemplate);
    MCPTask_Void NotifyResourceListChanged();
    MCPTask_Void NotifyResourceUpdated(const Resource& InResource);

    // Root management
    void AddRoot(const Root& InRoot);
    void RemoveRoot(const Root& InRoot);
    MCPTask_Void NotifyRootsListChanged();

    // Logging
    MCPTask_Void LogMessage(LoggingLevel InLevel, const std::string& InLogger,
                            const JSONValue& InData);

    // Sampling requests (servers can request sampling from clients)
    MCPTask<CreateMessageResponse::CreateMessageResult>
    RequestSampling(const std::vector<SamplingMessage>& InMessages, int64_t InMaxTokens,
                    const std::string& InSystemPrompt = "",
                    const std::string& InIncludeContext = "none",
                    double InTemperature = DEFAULT_TEMPERATURE,
                    const std::vector<std::string>& InStopSequences = {},
                    const ModelPreferences& InModelPrefs = {}, const JSONValue& InMetadata = {});

    // Progress reporting
    MCPTask_Void ReportProgress(const RequestID& InRequestID, double InProgress,
                                int64_t InTotal = -1);
    MCPTask_Void CancelRequest(const RequestID& InRequestID, const std::string& InReason = "");

  private:
    // Server state
    bool m_IsRunning{false};

    // Feature managers (thread-safe)
    PromptManager m_PromptManager;
    ResourceManager m_ResourceManager;
    ToolManager m_ToolManager;

    // Handler mappings for legacy compatibility
    std::unordered_map<std::string, ToolHandler> m_ToolHandlers;
    std::unordered_map<std::string, PromptHandler> m_PromptHandlers;
    std::unordered_map<std::string, ResourceHandler> m_ResourceHandlers;
    std::unordered_map<std::string, ResourceTemplateHandler> m_ResourceTemplateHandlers;
    mutable std::mutex m_HandlerMappingMutex;

    // Root management
    std::unordered_map<Root, RootHandler> m_Roots;
    mutable std::mutex m_RootsMutex;

    // Feature handlers
    // TODO: @HalcyonOmega Cleanup, fix, or remove these handlers
    std::function<void()> m_SamplingHandler;
    std::function<void()> m_CompletionHandler;

    // Handler management
    mutable std::mutex m_HandlersMutex;

    // Pagination helper methods
    std::string EncodeCursor(size_t InIndex) const;
    size_t DecodeCursor(const std::string& InCursor) const;
    static constexpr size_t DEFAULT_PAGE_SIZE = 100;

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

    // Updated subscription management with client tracking
    std::unordered_map<Resource, std::vector<std::string> /* Client IDs */> m_ResourceSubscriptions;
    mutable std::mutex m_ResourceSubscriptionsMutex;

    MCPTask_Void UpdateProgress(double InProgress, std::optional<int64_t> InTotal = {});
    MCPTask_Void CompleteProgress();
};

MCP_NAMESPACE_END