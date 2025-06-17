#pragma once

#include <concepts>
#include <coroutine>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "Core.h"

MCP_NAMESPACE_BEGIN

// Core types
struct JSON_RPCRequest;
struct JSON_RPCResponse;
struct JSON_RPCNotification;
struct JSON_RPCError;

// Content types
struct TextContent;
struct ImageContent;
struct AudioContent;
struct ResourceContent;

// MCP types
struct Capability;
struct ClientInfo;
struct ServerInfo;
struct InitializeParams;
struct InitializeResult;

// Tool types
struct Tool;
struct ToolCall;
struct ToolResult;

// Resource types
struct Resource;
struct ResourceTemplate;

// Prompt types
struct Prompt;
struct PromptMessage;

// Sampling types
struct SamplingRequest;
struct SamplingResult;
struct ModelPreferences;

// Transport
class ITransport;

} // namespace MCP

namespace MCP {

// Core Protocol Interface
class IMCP {
  public:
    virtual ~IMCP() = default;

    // Lifecycle Management
    virtual future<InitializeResult> Initialize(const InitializeParams& params) = 0;
    virtual future<void> Initialized() = 0;
    virtual future<void> Shutdown() = 0;

    // Connection Management
    virtual bool IsConnected() const = 0;
    virtual bool IsInitialized() const = 0;
    virtual string GetProtocolVersion() const = 0;

    // Transport
    virtual void SetTransport(unique_ptr<ITransport> transport) = 0;
    virtual ITransport* GetTransport() const = 0;

    // Error Handling
    virtual void OnError(function<void(const JSON_RPCError&)> callback) = 0;
    virtual void OnDisconnected(function<void()> callback) = 0;

    // Ping/Utility
    virtual future<void> Ping() = 0;
};

// Server Interface
class IMCP_Server : public virtual IMCP {
  public:
    virtual ~IMCP_Server() = default;

    // Tool Management
    virtual future<vector<Tool>> ListTools(const optional<string>& cursor = {}) = 0;
    virtual future<ToolResult> CallTool(const ToolCall& call) = 0;

    // Tool Registration
    virtual void RegisterTool(const Tool& tool,
                              function<future<ToolResult>(const ToolCall&)> handler) = 0;
    virtual void UnregisterTool(const string& toolName) = 0;

    // Resource Management
    virtual future<vector<Resource>> ListResources(const optional<string>& cursor = {}) = 0;
    virtual future<vector<ResourceContent>> ReadResource(const string& uri) = 0;
    virtual future<vector<ResourceTemplate>>
    ListResourceTemplates(const optional<string>& cursor = {}) = 0;

    // Resource Registration
    virtual void RegisterResource(const Resource& resource,
                                  function<future<vector<ResourceContent>>()> provider) = 0;
    virtual void UnregisterResource(const string& uri) = 0;

    // Resource Subscription
    virtual future<void> SubscribeToResource(const string& uri) = 0;
    virtual future<void> UnsubscribeFromResource(const string& uri) = 0;

    // Prompt Management
    virtual future<vector<Prompt>> ListPrompts(const optional<string>& cursor = {}) = 0;
    virtual future<PromptMessage>
    GetPrompt(const string& name, const unordered_map<string, string>& arguments = {}) = 0;

    // Prompt Registration
    virtual void RegisterPrompt(
        const Prompt& prompt,
        function<future<PromptMessage>(const unordered_map<string, string>&)> handler) = 0;
    virtual void UnregisterPrompt(const string& name) = 0;

    // Logging
    virtual future<void> LogMessage(const string& level, const string& message,
                                    const optional<string>& logger = {}) = 0;

    // Progress Tracking
    virtual future<void> ReportProgress(const string& progressToken, double progress,
                                        const optional<string>& total = {}) = 0;

    // Notifications
    virtual void OnToolListChanged(function<void()> callback) = 0;
    virtual void OnResourceListChanged(function<void()> callback) = 0;
    virtual void OnPromptListChanged(function<void()> callback) = 0;
    virtual void OnResourceUpdated(function<void(const string& uri)> callback) = 0;

    // Capabilities
    virtual void SetCapabilities(const Capability& capabilities) = 0;
    virtual Capability GetCapabilities() const = 0;
};

// Client Interface
class IMCP_Client : public virtual IMCP {
  public:
    virtual ~IMCP_Client() = default;

    // Tool Operations
    virtual future<vector<Tool>> ListTools(const optional<string>& cursor = {}) = 0;
    virtual future<ToolResult> CallTool(const ToolCall& call) = 0;

    // Resource Operations
    virtual future<vector<Resource>> ListResources(const optional<string>& cursor = {}) = 0;
    virtual future<vector<ResourceContent>> ReadResource(const string& uri) = 0;
    virtual future<vector<ResourceTemplate>>
    ListResourceTemplates(const optional<string>& cursor = {}) = 0;

    // Resource Subscription
    virtual future<void> SubscribeToResource(const string& uri) = 0;
    virtual future<void> UnsubscribeFromResource(const string& uri) = 0;

    // Prompt Operations
    virtual future<vector<Prompt>> ListPrompts(const optional<string>& cursor = {}) = 0;
    virtual future<PromptMessage>
    GetPrompt(const string& name, const unordered_map<string, string>& arguments = {}) = 0;

    // Sampling (for servers to request LLM operations from clients)
    virtual future<SamplingResult> CreateMessage(const SamplingRequest& request) = 0;

    // Sampling Registration (client provides sampling capability)
    virtual void
    RegisterSamplingHandler(function<future<SamplingResult>(const SamplingRequest&)> handler) = 0;

    // Root Directory Management
    virtual future<vector<string>> ListRoots() = 0;
    virtual void SetRoots(const vector<string>& roots) = 0;

    // Notification Handlers
    virtual void OnToolListChanged(function<void()> callback) = 0;
    virtual void OnResourceListChanged(function<void()> callback) = 0;
    virtual void OnPromptListChanged(function<void()> callback) = 0;
    virtual void OnResourceUpdated(function<void(const string& uri)> callback) = 0;
    virtual void OnRootsListChanged(function<void()> callback) = 0;

    // Capabilities
    virtual void SetCapabilities(const Capability& capabilities) = 0;
    virtual Capability GetCapabilities() const = 0;
};

// Unified Interface (can act as both client and server)
class IMCP_Host : public IMCP_Client, public IMCP_Server {
  public:
    virtual ~IMCP_Host() = default;

    // Mode Management
    enum class Mode { Client, Server, Bidirectional };

    virtual void SetMode(Mode mode) = 0;
    virtual Mode GetMode() const = 0;

    // Connection Management for bidirectional scenarios
    virtual future<void> ConnectAsClient(const string& serverEndpoint) = 0;
    virtual future<void> StartAsServer(const string& bindAddress, int port) = 0;
};

// Factory Interface
class IMCP_Factory {
  public:
    virtual ~IMCP_Factory() = default;

    // Factory Methods
    virtual unique_ptr<IMCP_Client> CreateClient() = 0;
    virtual unique_ptr<IMCP_Server> CreateServer() = 0;
    virtual unique_ptr<IMCP_Host> CreateHost() = 0;

    // Transport Creation
    virtual unique_ptr<ITransport> CreateStdioTransport() = 0;
    virtual unique_ptr<ITransport> CreateHTTPTransport(const string& endpoint) = 0;
    virtual unique_ptr<ITransport> CreateWebSocketTransport(const string& endpoint) = 0;
    virtual unique_ptr<ITransport> CreateInMemoryTransport() = 0;

    // Protocol Version Support
    virtual vector<string> GetSupportedProtocolVersions() const = 0;
    virtual string GetLatestProtocolVersion() const = 0;
    virtual bool IsProtocolVersionSupported(const string& version) const = 0;
};

// Content Type Concepts
template <typename T>
concept MCPContent = requires(T t) {
    { t.GetType() } -> convertible_to<string>;
    { t.GetData() } -> convertible_to<string>;
};

template <typename T>
concept MCPHandler = requires(T t) { is_invocable_v<T>; };

// Practical MCP Implementation Helper Class
// This class provides implementation-specific functionality not defined in the MCP spec
// but commonly needed for real-world usage
class PracticalMCP {
  public:
    virtual ~PracticalMCP() = default;

    // Connection Management (implementation details)
    virtual void SetConnectionTimeout(chrono::milliseconds timeout) = 0;
    virtual chrono::milliseconds GetConnectionTimeout() const = 0;
    virtual void SetRetryPolicy(int maxRetries, chrono::milliseconds retryDelay) = 0;

    // Session State Management (practical concerns)
    virtual void SaveState(const string& statePath) = 0;
    virtual bool LoadState(const string& statePath) = 0;
    virtual void ClearState() = 0;

    // Performance & Monitoring
    virtual void EnableMetrics(bool enable) = 0;
    virtual unordered_map<string, double> GetMetrics() const = 0;
    virtual void SetMaxConcurrentRequests(size_t maxRequests) = 0;
    virtual size_t GetActiveRequestCount() const = 0;

    // Security & Validation
    virtual void SetRequestValidator(function<bool(const JSON_RPCRequest&)> validator) = 0;
    virtual void SetResponseValidator(function<bool(const JSON_RPCResponse&)> validator) = 0;
    virtual void EnableRequestLogging(bool enable, const string& logPath = "") = 0;

    // Advanced Transport Configuration
    virtual void SetCustomHeaders(const unordered_map<string, string>& headers) = 0;
    virtual void SetCompressionEnabled(bool enable) = 0;
    virtual void SetKeepAliveSettings(bool enable, chrono::seconds interval) = 0;

    // Event System (for advanced monitoring)
    virtual void OnRequestSent(function<void(const JSON_RPCRequest&)> callback) = 0;
    virtual void OnResponseReceived(function<void(const JSON_RPCResponse&)> callback) = 0;
    virtual void OnNotificationReceived(function<void(const JSON_RPCNotification&)> callback) = 0;
    virtual void OnConnectionStateChanged(function<void(bool connected)> callback) = 0;

    // Batching Support (JSON-RPC batch operations)
    virtual void EnableBatching(bool enable, size_t maxBatchSize = 10) = 0;
    virtual void FlushBatch() = 0;

    // Thread Safety Options
    enum class ThreadingModel { SingleThreaded, ThreadSafe, Async };
    virtual void SetThreadingModel(ThreadingModel model) = 0;

    // Resource Management
    virtual void SetResourceCacheSize(size_t maxCacheSize) = 0;
    virtual void ClearResourceCache() = 0;

    // Development/Debug Features
    virtual void SetDebugMode(bool enable) = 0;
    virtual string DumpInternalState() const = 0;
    virtual void InjectTestFailure(const string& methodName, const string& errorType) = 0;
};

// Utility Functions
namespace Utilities {
// JSON-RPC ID generation
string GenerateRequestID();

// Content validation
bool ValidateJSON_Schema(const string& schema, const string& data);

// Error creation helpers
JSON_RPCError CreateInvalidRequestError(const string& message);
JSON_RPCError CreateMethodNotFoundError(const string& method);
JSON_RPCError CreateInvalidParamsError(const string& message);
JSON_RPCError CreateInternalError(const string& message);

// Protocol version comparison
int CompareProtocolVersions(const string& version1, const string& version2);
bool IsVersionCompatible(const string& clientVersion, const string& serverVersion);
} // namespace Utilities

MCP_NAMESPACE_END