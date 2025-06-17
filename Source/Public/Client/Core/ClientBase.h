#pragma once

#include "IMCP.h"

MCP_NAMESPACE_BEGIN

// TODO: @HalcyonOmega Fix naming conventions

// Client Interface
class IClientAPI {
  public:
    // Tool Operations
    virtual MCPTask<ListToolsResult> ListTools(const optional<ListToolsRequest>& Params = nullopt);
    virtual MCPTask<ToolResult> CallTool(const ToolCall& InCall) = 0;

    // Resource Operations
    virtual MCPTask<ListResourcesResult>
    ListResources(const optional<ListResourcesRequest>& Params = nullopt);
    virtual MCPTask<vector<ResourceContent>> ReadResource(const string& InURI) = 0;
    virtual MCPTask<vector<ResourceTemplate>>
    ListResourceTemplates(const optional<string>& InCursor = {}) = 0;

    // Resource Subscription
    virtual MCPTask_Void SubscribeToResource(const string& InURI) = 0;
    virtual MCPTask_Void UnsubscribeFromResource(const string& InURI) = 0;

    // Prompt Operations
    virtual MCPTask<ListPromptsResult>
    ListPrompts(const optional<ListPromptsRequest>& Params = nullopt);
    virtual MCPTask<GetPromptResult>
    GetPrompt(const GetPromptRequest::GetPromptRequestParams& Params) = 0;

    // Sampling (for servers to request LLM operations from clients)
    virtual MCPTask<SamplingResult> CreateMessage(const SamplingRequest& InRequest) = 0;

    // Sampling Registration (client provides sampling capability)
    virtual void RegisterSamplingHandler(
        function<MCPTask<SamplingResult>(const SamplingRequest&)> InHandler) = 0;

    // Root Directory Management
    virtual MCPTask<ListRootsResult> ListRoots() = 0;
    virtual MCPTask_Void SetRoots(const vector<string>& InRoots) = 0;
    virtual MCPTask_Void SendRootsListChanged() = 0;

    // Notification Handlers
    virtual void OnToolListChanged(function<void()> InCallback) = 0;
    virtual void OnResourceListChanged(function<void()> InCallback) = 0;
    virtual void OnPromptListChanged(function<void()> InCallback) = 0;
    virtual void OnResourceUpdated(function<void(const string& InURI)> InCallback) = 0;
    virtual void OnRootsListChanged(function<void()> InCallback) = 0;

    // Capabilities
    virtual void SetCapabilities(const ClientCapabilities& InCapabilities) = 0;
    virtual ClientCapabilities GetCapabilities() const = 0;

    // TODO: Determine if below is right or above is right. This was used in Direct Translation
    // Registers new capabilities. This can only be called before connecting to a transport.
    //
    // The new capabilities will be merged with any existing capabilities previously given (e.g., at
    // initialization).
    void RegisterCapabilities(const ClientCapabilities& Capabilities);
};

/**
 * An MCP client on top of a pluggable transport.
 *
 * The client will automatically begin the initialization flow with the server when Connect() is
 * called.
 */
class ClientBase : public IClientAPI, public IMCP {
  public:
    struct ClientOptions : public ProtocolOptions {
        optional<ClientCapabilities> Capabilities;
    };
    // Initializes this client with the given name and version information.
    ClientBase(const Implementation& ClientInfo, const optional<ClientOptions>& Options = nullopt);

    ClientBase(const ClientOptions& InOptions);
    ~ClientBase() override;

    // Client Methods
    // TODO: Ping?
    // TODO: MCPTask<CompleteResult> Complete(const CompleteRequest& Params);
    // TODO: MCPTask_Void SetLoggingLevel(LoggingLevelRequest& InRequest);
    MCPTask_Void Connect(optional<shared_ptr<Transport>> InTransport);

    // TODO: This was used in Direct Translation - Needed?
  protected:
    void AssertCapability(const string& Capability, const string& Method);
    void AssertCapabilityForMethod(const string& Method);
    void AssertNotificationCapability(const string& Method);
    void AssertRequestHandlerCapability(const string& Method);

    // TODO: This was used in Direct Translation - Needed?
    // private:
    //   void CacheToolOutputSchemas(const vector<Tool>& Tools);
    //   optional<ValidateFunction> GetToolOutputValidator(const string& ToolName);

  private:
    Implementation m_ClientInfo;
    ClientCapabilities m_Capabilities;
    // unordered_map<string, ValidateFunction> m_CachedToolOutputValidators; TODO: Needed?
};

MCP_NAMESPACE_END