#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Core.h"
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
    friend class ::MCP::MCPClientFactory;

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

// Beautiful, human-readable builder pattern
class MCPClientFactory {
  public:
    MCPClientFactory();

    // Transport configuration
    MCPClientFactory& WithTransport(TransportType InType);
    MCPClientFactory& WithTransport(shared_ptr<ITransport> InTransport);

    // Stdio-specific options
    MCPClientFactory& WithStdioCommand(const string& InCommand);
    MCPClientFactory& WithStdioArgs(const vector<string>& InArgs);

    // HTTP-specific options
    MCPClientFactory& WithHTTPEndpoint(const string& InURL);
    MCPClientFactory& WithHTTPHeaders(const JSON& InHeaders);

    // WebSocket-specific options
    MCPClientFactory& WithWebSocketURL(const string& InURL);

    // Client capabilities
    MCPClientFactory& WithCapabilities(const ClientCapabilities& InCapabilities);
    MCPClientFactory& WithToolsCapability(bool InEnabled = true);
    MCPClientFactory& WithResourcesCapability(bool InEnabled = true);
    MCPClientFactory& WithPromptsCapability(bool InEnabled = true);
    MCPClientFactory& WithRootsCapability(bool InEnabled = true);
    MCPClientFactory& WithSamplingCapability(bool InEnabled = true);

    // Client information
    MCPClientFactory& WithClientInfo(const Implementation& InClientInfo);
    MCPClientFactory& WithClientInfo(const string& InName, const string& InVersion);

    // Session configuration
    MCPClientFactory& WithTimeout(chrono::milliseconds InTimeout);
    MCPClientFactory& WithRetries(size_t InMaxRetries);
    MCPClientFactory& WithSessionConfig(const SessionConfig& InConfig);

    // Build the client
    MCP::Client Build();

  private:
    // Transport settings
    TransportType m_TransportType{TransportType::Stdio};
    shared_ptr<ITransport> m_CustomTransport;

    // Stdio settings
    optional<string> m_StdioCommand;
    optional<vector<string>> m_StdioArgs;

    // HTTP settings
    optional<string> m_HTTPEndpoint;
    optional<JSON> m_HTTPHeaders;

    // WebSocket settings
    optional<string> m_WebSocketURL;

    // Capabilities
    ClientCapabilities m_Capabilities;

    // Client info
    Implementation m_ClientInfo{"MCPClient", "1.0.0"};

    // Session config
    SessionConfig m_SessionConfig;

    shared_ptr<ITransport> CreateTransport();
};

// Global factory function for the beautiful API
inline MCPClientFactory MCPClient() {
    return MCPClientFactory();
}

// Example usage:
/*
auto client = MCPClient()
    .WithTransport(TransportType::Stdio)
    .WithStdioCommand("my-mcp-server")
    .WithToolsCapability(true)
    .WithResourcesCapability(true)
    .WithClientInfo("MyApp", "1.0.0")
    .Build();

// Connect and use
co_await client.Connect();
co_await client.Initialize();

auto tools = co_await client.ListTools();
auto result = co_await client.CallTool("my_tool", {{"param", "value"}});

co_await client.Disconnect();
*/

MCP_NAMESPACE_END
