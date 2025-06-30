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

    // Client-specific operations
    MCPTask<InitializeResponse::Result> Initialize();

    // Tools
    MCPTask<ListToolsResponse::Result> ListTools();
    MCPTask<CallToolResponse::Result> CallTool(const Tool& InTool);

    // Prompts
    MCPTask<ListPromptsResponse::Result> ListPrompts();
    MCPTask<GetPromptResponse::Result> GetPrompt(const Prompt& InPrompt);

    // Resources
    MCPTask<ListResourcesResponse::Result> ListResources();
    MCPTask<ReadResourceResponse::Result> ReadResource(const Resource& InResource);
    MCPTask_Void Subscribe(const Resource& InResource);
    MCPTask_Void Unsubscribe(const Resource& InResource);

    // Roots
    MCPTask<ListRootsResponse::Result> ListRoots();

    // Logging
    MCPTask_Void SetLoggingLevel(LoggingLevel InLevel);

    // Autocomplete
    MCPTask<CompleteResponse::Result> Complete(const CompleteRequest& InRequest);

    // Sampling (for servers that want to sample via client)
    MCPTask<CreateMessageResponse::Result> CreateMessage(const CreateMessageRequest& InRequest);
};

MCP_NAMESPACE_END