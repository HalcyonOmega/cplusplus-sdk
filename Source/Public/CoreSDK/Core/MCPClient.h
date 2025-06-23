#pragma once

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Core/IMCP.h"

MCP_NAMESPACE_BEGIN

// Client protocol handler
class MCPClient : public MCPProtocol {
  public:
    explicit MCPClient(std::unique_ptr<ITransport> InTransport);

    // Client-specific operations
    MCPTask<ListToolsResponse::ListToolsResult> ListTools();
    MCPTask<CallToolResponse::CallToolResult>
    CallTool(const std::string& InName,
             const std::unordered_map<std::string, JSONValue>& InArguments = {});

    MCPTask<ListPromptsResponse::ListPromptsResult> ListPrompts();
    MCPTask<GetPromptResponse::GetPromptResult>
    GetPrompt(const std::string& InName,
              const std::unordered_map<std::string, std::string>& InArguments = {});

    MCPTask<ListResourcesResponse::ListResourcesResult> ListResources();
    MCPTask<ReadResourceResponse::ReadResourceResult> ReadResource(const std::string& InURI);
    MCPTask_Void Subscribe(const std::string& InURI);
    MCPTask_Void Unsubscribe(const std::string& InURI);

    MCPTask<ListRootsResponse::ListRootsResult> ListRoots();
    MCPTask_Void SetLoggingLevel(LoggingLevel InLevel);

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

  protected:
    void OnInitializeRequest(const InitializeRequest& InRequest,
                             const std::string& InRequestID) override;
    void OnInitializedNotification() override;

  private:
    ServerCapabilities m_ServerCapabilities;
    Implementation m_ServerInfo;
};

MCP_NAMESPACE_END