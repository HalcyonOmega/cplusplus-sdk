#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Core.h"
#include "Core/Features/Prompt/Prompts.h"
#include "Core/Features/Resource/Resources.h"
#include "Core/Features/Root/Roots.h"
#include "Core/Features/Sampling/Sampling.h"
#include "Core/Features/Tool/Tools.h"
#include "Core/Types/Capabilities.h"
#include "Core/Types/Implementation.h"
#include "Core/Types/Initialization.h"
#include "ISession.h"
#include "ITransport.h"

MCP_NAMESPACE_BEGIN

// Forward declarations
class MCPClientFactory;

// Main MCP Client class - high-level interface for MCP client operations
class Client {
  public:
    ~Client() = default;

    // === Lifecycle Management ===

    // Connect to MCP server
    MCPTask_Void Connect();

    // Disconnect from server
    MCPTask_Void Disconnect();

    // Check if connected and ready
    bool IsConnected() const;

    // === MCP Operations ===

    // Initialize MCP session with server
    MCPTask<InitializeResult> Initialize();

    // Request available tools from server
    MCPTask<vector<Tool>> ListTools();

    // Call a specific tool
    MCPTask<ToolResult> CallTool(const string& InName, const JSON& InArguments = {});

    // Request available resources from server
    MCPTask<vector<Resource>> ListResources();

    // Read a specific resource
    MCPTask<ResourceContents> ReadResource(const string& InURI);

    // Request available prompts from server
    MCPTask<vector<Prompt>> ListPrompts();

    // Get a specific prompt
    MCPTask<GetPromptResult> GetPrompt(const string& InName, const JSON& InArguments = {});

    // Request available roots from server
    MCPTask<vector<Root>> ListRoots();

    // === Sampling Operations (if supported) ===

    // Create sampling request
    MCPTask<SamplingResult> CreateSamplingRequest(const SamplingRequest& InRequest);

    // === Utility Operations ===

    // Send ping to server
    MCPTask<chrono::milliseconds> Ping();

    // Get client capabilities
    const ClientCapabilities& GetCapabilities() const;

    // Get server capabilities (after initialization)
    optional<ServerCapabilities> GetServerCapabilities() const;

    // Get session statistics
    SessionStats GetStats() const;

    // === Event Handling ===

    // Set callbacks for various events
    void OnTool(function<void(const Tool&)> InCallback);
    void OnResource(function<void(const Resource&)> InCallback);
    void OnPrompt(function<void(const Prompt&)> InCallback);
    void OnNotification(function<void(const NotificationBase&)> InCallback);
    void OnError(function<void(const string&)> InCallback);
    void OnDisconnected(function<void()> InCallback);

  private:
    friend class MCPClientFactory;

    // Private constructor - use MCPClient() builder
    Client(unique_ptr<ISession> InSession, ClientCapabilities InCapabilities,
           Implementation InClientInfo, shared_ptr<ITransport> InTransport);

    unique_ptr<ISession> m_Session;
    ClientCapabilities m_Capabilities;
    Implementation m_ClientInfo;
    shared_ptr<ITransport> m_Transport;

    // Internal state
    bool m_IsInitialized{false};
    optional<ServerCapabilities> m_ServerCapabilities;

    // Callbacks
    optional<function<void(const Tool&)>> m_OnTool;
    optional<function<void(const Resource&)>> m_OnResource;
    optional<function<void(const Prompt&)>> m_OnPrompt;
    optional<function<void(const NotificationBase&)>> m_OnNotification;
    optional<function<void(const string&)>> m_OnError;
    optional<function<void()>> m_OnDisconnected;
};

// Clean factory API - builds automatically when no more chaining
class MCPClientFactory {
  public:
    // Required transport configuration in constructor - auto-builds if no chaining

    // Stdio transport constructor
    MCPClientFactory(TransportType InType, const string& InCommand,
                     const vector<string>& InArgs = {});

    // HTTP transport constructor
    MCPClientFactory(TransportType InType, const string& InEndpoint);

    // WebSocket transport constructor
    MCPClientFactory(TransportType InType, const string& InURL);

    // Custom transport constructor
    MCPClientFactory(shared_ptr<ITransport> InTransport);

    // === Grouped Configuration (chainable) ===

    // HTTP-specific options
    struct HTTPOptions {
        optional<JSON> Headers;
        optional<chrono::milliseconds> Timeout;
        optional<size_t> MaxRetries;
    };
    MCPClientFactory& HTTPOptions(const HTTPOptions& InOptions);

    // Stdio-specific options
    struct StdioOptions {
        optional<chrono::milliseconds> ProcessTimeout;
        optional<string> WorkingDirectory;
        optional<map<string, string>> Environment;
    };
    MCPClientFactory& StdioOptions(const StdioOptions& InOptions);

    // WebSocket-specific options
    struct WebSocketOptions {
        optional<JSON> Headers;
        optional<chrono::milliseconds> PingInterval;
        optional<size_t> MaxFrameSize;
    };
    MCPClientFactory& WebSocketOptions(const WebSocketOptions& InOptions);

    // All capabilities grouped (defaults to all false)
    struct CapabilityOptions {
        bool Tools{false};
        bool Resources{false};
        bool Prompts{false};
        bool Roots{false};
        bool Sampling{false};
    };
    MCPClientFactory& Capabilities(const CapabilityOptions& InCapabilities);

    // Individual capabilities (auto-enables when called)
    MCPClientFactory& Tools();
    MCPClientFactory& Resources();
    MCPClientFactory& Prompts();
    MCPClientFactory& Roots();
    MCPClientFactory& Sampling();

    // Client information
    MCPClientFactory& ClientInfo(const string& InName, const string& InVersion);

    // Session settings
    MCPClientFactory& SessionOptions(chrono::milliseconds InTimeout, size_t InMaxRetries = 3);

    // Auto-build when destroyed or explicitly converted
    operator Client();

  private:
    // Transport settings
    TransportType m_TransportType;
    shared_ptr<ITransport> m_CustomTransport;

    // Transport-specific config
    string m_StdioCommand;
    vector<string> m_StdioArgs;
    string m_HTTPEndpoint;
    string m_WebSocketURL;

    // Grouped options
    optional<HTTPOptions> m_HTTPOptions;
    optional<StdioOptions> m_StdioOptions;
    optional<WebSocketOptions> m_WebSocketOptions;

    // Capabilities (defaults to all false)
    CapabilityOptions m_Capabilities;

    // Client info
    Implementation m_ClientInfo{.Name = "MCPClient", .Version = "1.0.0"};

    // Session config
    SessionConfig m_SessionConfig;

    shared_ptr<ITransport> CreateTransport();
    ClientCapabilities BuildCapabilities();
};

// Global factory functions for clean API
inline MCPClientFactory MCPClient(TransportType InType, const string& InCommand,
                                  const vector<string>& InArgs = {}) {
    return MCPClientFactory(InType, InCommand, InArgs);
}

inline MCPClientFactory MCPClient(TransportType InType, const string& InEndpoint) {
    return MCPClientFactory(InType, InEndpoint);
}

inline MCPClientFactory MCPClient(shared_ptr<ITransport> InTransport) {
    return MCPClientFactory(InTransport);
}

// Example usage - much cleaner and auto-builds!
/*
// Stdio client - auto-builds after constructor
auto client1 = MCPClient(TransportType::Stdio, "my-mcp-server");

// HTTP client with grouped options - auto-builds after last chain
auto client2 = MCPClient(TransportType::HTTP, "https://api.example.com/mcp")
    .HTTPOptions({.Headers = {{"Authorization", "Bearer token"}}, .Timeout = chrono::seconds(30)})
    .Tools()
    .Resources();

// With capabilities grouped
auto client3 = MCPClient(TransportType::WebSocket, "wss://server.com/mcp")
    .Capabilities({.Tools = true, .Resources = true, .Prompts = false})
    .ClientInfo("MyApp", "2.0.0");

// Connect and use (client built automatically)
co_await client1.Connect();
co_await client1.Initialize();
auto tools = co_await client1.ListTools();
*/

MCP_NAMESPACE_END
