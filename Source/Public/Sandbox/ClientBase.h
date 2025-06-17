#pragma once

#include "IMCP.h"

MCP_NAMESPACE_BEGIN

// TODO: @HalcyonOmega Fix naming conventions

// Client Interface
class IClientAPI {
  public:
    // Tool Operations
    virtual MCPTask<vector<Tool>> ListTools(const optional<string>& InCursor = {}) = 0;
    virtual MCPTask<ToolResult> CallTool(const ToolCall& InCall) = 0;

    // Resource Operations
    virtual MCPTask<vector<Resource>> ListResources(const optional<string>& InCursor = {}) = 0;
    virtual MCPTask<vector<ResourceContent>> ReadResource(const string& InURI) = 0;
    virtual MCPTask<vector<ResourceTemplate>>
    ListResourceTemplates(const optional<string>& InCursor = {}) = 0;

    // Resource Subscription
    virtual MCPTask_Void SubscribeToResource(const string& InURI) = 0;
    virtual MCPTask_Void UnsubscribeFromResource(const string& InURI) = 0;

    // Prompt Operations
    virtual MCPTask<vector<Prompt>> ListPrompts(const optional<string>& InCursor = {}) = 0;
    virtual MCPTask<PromptMessage>
    GetPrompt(const string& InName, const unordered_map<string, string>& InArguments = {}) = 0;

    // Sampling (for servers to request LLM operations from clients)
    virtual MCPTask<SamplingResult> CreateMessage(const SamplingRequest& InRequest) = 0;

    // Sampling Registration (client provides sampling capability)
    virtual void RegisterSamplingHandler(
        function<MCPTask<SamplingResult>(const SamplingRequest&)> InHandler) = 0;

    // Root Directory Management
    virtual MCPTask<vector<string>> ListRoots() = 0;
    virtual void SetRoots(const vector<string>& InRoots) = 0;

    // Notification Handlers
    virtual void OnToolListChanged(function<void()> InCallback) = 0;
    virtual void OnResourceListChanged(function<void()> InCallback) = 0;
    virtual void OnPromptListChanged(function<void()> InCallback) = 0;
    virtual void OnResourceUpdated(function<void(const string& InURI)> InCallback) = 0;
    virtual void OnRootsListChanged(function<void()> InCallback) = 0;

    // Capabilities
    virtual void SetCapabilities(const Capability& InCapabilities) = 0;
    virtual Capability GetCapabilities() const = 0;
};

class ClientBase : public IClientAPI, public IMCP {
  public:
    ClientBase(const ClientOptions& InOptions);
    ~ClientBase() override;
};

MCP_NAMESPACE_END