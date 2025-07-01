#pragma once

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Core/IMCP.h"

MCP_NAMESPACE_BEGIN

// Client protocol handler
class MCPClient : public MCPProtocol {
  public:
    MCPClient(TransportType InTransportType,
              std::optional<std::unique_ptr<TransportOptions>> InOptions,
              const Implementation& InClientInfo, const ClientCapabilities& InCapabilities);

    MCPTask_Void Start() override;
    MCPTask_Void Stop() override;

    // Requests
    MCPTask<InitializeResponse::Result> Request_Initialize();
    void OnNotified_Initialized();

    // Tools
    MCPTask<ListToolsResponse::Result> Request_ListTools();
    MCPTask<CallToolResponse::Result> Request_CallTool(const Tool& InTool);
    void OnNotified_ToolListChanged();

    // Prompts
    MCPTask<ListPromptsResponse::Result> Request_ListPrompts();
    MCPTask<GetPromptResponse::Result> Request_GetPrompt(const Prompt& InPrompt);
    void OnNotified_PromptListChanged();

    // Resources
    MCPTask<ListResourcesResponse::Result> Request_ListResources();
    MCPTask<ReadResourceResponse::Result> Request_ReadResource(const Resource& InResource);
    MCPTask_Void Request_Subscribe(const Resource& InResource);
    MCPTask_Void Request_Unsubscribe(const Resource& InResource);
    void OnNotified_ResourceListChanged();
    void OnNotified_ResourceUpdated();

    // Roots
    MCPTask<ListRootsResponse::Result> Request_ListRoots();
    void OnNotified_RootsListChanged();

    // Logging
    MCPTask_Void Request_SetLoggingLevel(LoggingLevel InLevel);
    void OnNotified_LogMessage();

    // Sampling (for servers that want to sample via client)
    MCPTask<CreateMessageResponse::Result>
    OnRequest_CreateMessage(const CreateMessageRequest& InRequest);

    // Progress
    void OnNotified_Progress();

    // Autocomplete
    MCPTask<CompleteResponse::Result> OnRequest_Complete(const CompleteRequest& InRequest);
};

MCP_NAMESPACE_END