#pragma once

#include "Core.h"
#include "IMCP.h"

MCP_NAMESPACE_BEGIN

// Server Interface
class IServerAPI {
  public:
    virtual ~IServerAPI() = default;

    // Tool Management
    virtual MCPTask<vector<Tool>> ListTools(const optional<string>& InCursor = {}) = 0;
    virtual MCPTask<ToolResult> CallTool(const ToolCall& InCall) = 0;

    // Tool Registration
    virtual void RegisterTool(const Tool& InTool,
                              function<MCPTask<ToolResult>(const ToolCall&)> InHandler) = 0;
    virtual void UnregisterTool(const string& InToolName) = 0;

    // Resource Management
    virtual MCPTask<vector<Resource>> ListResources(const optional<string>& InCursor = {}) = 0;
    virtual MCPTask<vector<ResourceContent>> ReadResource(const string& InURI) = 0;
    virtual MCPTask<vector<ResourceTemplate>>
    ListResourceTemplates(const optional<string>& InCursor = {}) = 0;

    // Resource Registration
    virtual void RegisterResource(const Resource& InResource,
                                  function<MCPTask<vector<ResourceContent>>()> InProvider) = 0;
    virtual void UnregisterResource(const string& InURI) = 0;

    // Resource Subscription
    virtual MCPTask_Void SubscribeToResource(const string& InURI) = 0;
    virtual MCPTask_Void UnsubscribeFromResource(const string& InURI) = 0;

    // Prompt Management
    virtual MCPTask<vector<Prompt>> ListPrompts(const optional<string>& InCursor = {}) = 0;
    virtual MCPTask<PromptMessage>
    GetPrompt(const string& InName, const unordered_map<string, string>& InArguments = {}) = 0;

    // Prompt Registration
    virtual void RegisterPrompt(
        const Prompt& InPrompt,
        function<MCPTask<PromptMessage>(const unordered_map<string, string>&)> InHandler) = 0;
    virtual void UnregisterPrompt(const string& InPromptName) = 0;

    // Logging
    virtual MCPTask_Void LogMessage(const string& InLevel, const string& InMessage,
                                    const optional<string>& InLogger = {}) = 0;

    // Progress Tracking
    virtual MCPTask_Void ReportProgress(const string& InProgressToken, double InProgress,
                                        const optional<string>& InTotal = {}) = 0;

    // Notifications
    virtual void OnToolListChanged(function<void()> InCallback) = 0;
    virtual void OnResourceListChanged(function<void()> InCallback) = 0;
    virtual void OnPromptListChanged(function<void()> InCallback) = 0;
    virtual void OnResourceUpdated(function<void(const string& InURI)> InCallback) = 0;

    // Capabilities
    virtual void SetCapabilities(const Capability& InCapabilities) = 0;
    virtual Capability GetCapabilities() const = 0;
};

class ServerBase : public IServerAPI, public IMCP {
  public:
    ServerBase(const ServerOptions& InOptions);
    ~ServerBase() override;
};

MCP_NAMESPACE_END