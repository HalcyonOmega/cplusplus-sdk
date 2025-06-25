#pragma once

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Core/IMCP.h"

MCP_NAMESPACE_BEGIN

// Client protocol handler
class MCPClient : public MCPProtocol {
  public:
    explicit MCPClient(std::unique_ptr<ITransport> InTransport);

    // Client-specific operations
    void Initialize();

    // Tools
    MCPTask<ListToolsResponse::ListToolsResult> ListTools();
    MCPTask<CallToolResponse::CallToolResult> CallTool(const Tool& InTool);

    // Prompts
    MCPTask<ListPromptsResponse::ListPromptsResult> ListPrompts();
    MCPTask<GetPromptResponse::GetPromptResult> GetPrompt(const Prompt& InPrompt);

    // Resources
    MCPTask<ListResourcesResponse::ListResourcesResult> ListResources();
    MCPTask<ReadResourceResponse::ReadResourceResult> ReadResource(const Resource& InResource);
    MCPTask_Void Subscribe(const Resource& InResource);
    MCPTask_Void Unsubscribe(const Resource& InResource);

    // Roots
    MCPTask<ListRootsResponse::ListRootsResult> ListRoots();

    // Logging
    MCPTask_Void SetLoggingLevel(LoggingLevel InLevel);

    // Autocomplete
    MCPTask<CompleteResponse::CompleteResult> Complete(const std::string& InRefType,
                                                       const std::string& InRefURI,
                                                       const std::string& InArgName,
                                                       const std::string& InArgValue);

    // Sampling (for servers that want to sample via client)
    MCPTask<CreateMessageResponse::CreateMessageResult>
    CreateMessage(const std::vector<SamplingMessage>& InMessages, int64_t InMaxTokens,
                  const std::string& InSystemPrompt = "",
                  const std::string& InIncludeContext = "none",
                  double InTemperature = DEFAULT_TEMPERATURE,
                  const std::vector<std::string>& InStopSequences = {},
                  const ModelPreferences& InModelPrefs = {}, const JSONValue& InMetadata = {});

  private:
    ServerCapabilities m_ServerCapabilities;
    Implementation m_ServerInfo;
};

MCP_NAMESPACE_END