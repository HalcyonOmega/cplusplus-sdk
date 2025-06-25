#pragma once

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Core/IMCP.h"

MCP_NAMESPACE_BEGIN

// Server protocol handler
class MCPServer : public MCPProtocol {
  protected:
    void OnInitializeRequest(const InitializeRequest& InRequest);
    void OnInitializedNotification();

    void HandleRequest(const RequestBase& InRequest) override;

  public:
    explicit MCPServer(std::unique_ptr<ITransport> InTransport, const Implementation& InServerInfo,
                       const ServerCapabilities& InCapabilities);

    // Tool management
    void RegisterTool(const Tool& InTool, std::function<MCPTask<CallToolResponse::CallToolResult>(
                                              const std::unordered_map<std::string, JSONValue>&)>
                                              InHandler);
    void UnregisterTool(const Tool& InTool);
    MCPTask_Void NotifyToolListChanged();

    // Prompt management
    void RegisterPrompt(
        const Prompt& InPrompt,
        std::function<MCPTask<GetPromptResult>(const std::unordered_map<std::string, std::string>&)>
            InHandler);
    void UnregisterPrompt(const std::string& InName);
    MCPTask_Void NotifyPromptListChanged();

    // Resource management
    void
    RegisterResource(const Resource& InResource,
                     std::function<MCPTask<ReadResourceResponse::ReadResourceResult>()> InHandler);
    void RegisterResourceTemplate(
        const ResourceTemplate& InTemplate,
        std::function<MCPTask<ReadResourceResponse::ReadResourceResult>(const std::string&)>
            InHandler);
    void UnregisterResource(const Resource& InResource);
    MCPTask_Void NotifyResourceListChanged();
    MCPTask_Void NotifyResourceUpdated(const std::string& InURI);

    // Root management
    void RegisterRoot(const Root& InRoot);
    void UnregisterRoot(const std::string& InURI);
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
    Implementation m_ServerInfo;
    ServerCapabilities m_ServerCapabilities;
    ClientCapabilities m_ClientCapabilities;

    // Tool handlers
    struct ToolHandler {
        Tool ToolInfo;
        std::function<MCPTask<CallToolResult>(const std::unordered_map<std::string, JSONValue>&)>
            Handler;
    };
    std::unordered_map<std::string, ToolHandler> m_Tools;

    // Prompt handlers
    struct PromptHandler {
        Prompt PromptInfo;
        std::function<MCPTask<GetPromptResult>(const std::unordered_map<std::string, std::string>&)>
            Handler;
    };
    std::unordered_map<std::string, PromptHandler> m_Prompts;

    // Resource handlers
    struct ResourceHandler {
        Resource ResourceInfo;
        std::function<MCPTask<ReadResourceResult>()> Handler;
    };
    std::unordered_map<std::string, ResourceHandler> m_Resources;

    struct ResourceTemplateHandler {
        ResourceTemplate TemplateInfo;
        std::function<MCPTask<ReadResourceResult>(const std::string&)> Handler;
    };
    std::unordered_map<std::string, ResourceTemplateHandler> m_ResourceTemplates;

    // Root storage
    std::unordered_map<std::string, Root> m_Roots;

    // Subscription management
    std::unordered_map<std::string, std::unordered_set<std::string>>
        m_Subscriptions; // URI -> Set of client IDs

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
    MCPTask_Void NotifyResourceSubscribers(const std::string& InURI);
    std::string_view GetCurrentClientID() const;
    MCPTask_Void SendNotificationToClient(const std::string& InClientID,
                                          const ResourceUpdatedNotification& InNotification);

    // Updated subscription management with client tracking
    std::unordered_map<std::string, std::set<std::string>>
        m_ResourceSubscriptions; // URI -> Set of client IDs
    mutable std::mutex m_ResourceSubscriptionsMutex;
};

MCP_NAMESPACE_END