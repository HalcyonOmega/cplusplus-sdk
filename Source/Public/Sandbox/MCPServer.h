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
class MCPServerFactory;

// Main MCP Server class - high-level interface for MCP server operations
class Server {
  public:
    ~Server() = default;

    // === Lifecycle Management ===

    // Start the server and begin listening
    MCPTask_Void Start();

    // Stop the server gracefully
    MCPTask_Void Stop();

    // Check if server is running
    bool IsRunning() const;

    // === MCP Server Operations ===

    // Register a tool that clients can call
    void RegisterTool(const Tool& InTool, function<MCPTask<ToolResult>(const JSON&)> InHandler);

    // Register a resource that clients can read
    void RegisterResource(const Resource& InResource,
                          function<MCPTask<ResourceContents>()> InHandler);

    // Register a prompt that clients can get
    void RegisterPrompt(const Prompt& InPrompt,
                        function<MCPTask<GetPromptResult>(const JSON&)> InHandler);

    // Register a root that clients can list
    void RegisterRoot(const Root& InRoot);

    // === Notification Operations ===

    // Send notification to all connected clients
    MCPTask_Void NotifyAllClients(const NotificationBase& InNotification);

    // Send notification to specific client
    MCPTask_Void NotifyClient(const string& InClientID, const NotificationBase& InNotification);

    // Send resource updated notification
    MCPTask_Void NotifyResourceChanged(const string& InURI);

    // Send tool list changed notification
    MCPTask_Void NotifyToolsChanged();

    // Send prompt list changed notification
    MCPTask_Void NotifyPromptsChanged();

    // === Server Information ===

    // Get server capabilities
    const ServerCapabilities& GetCapabilities() const;

    // Get server implementation info
    const Implementation& GetServerInfo() const;

    // Get connected clients count
    size_t GetConnectedClientsCount() const;

    // Get server statistics
    SessionStats GetStats() const;

    // === Event Handling ===

    // Set callbacks for various events
    void OnClientConnected(function<void(const string& InClientID)> InCallback);
    void OnClientDisconnected(function<void(const string& InClientID)> InCallback);
    void OnClientInitialized(
        function<void(const string& InClientID, const ClientCapabilities&)> InCallback);
    void OnError(function<void(const string&)> InCallback);

  private:
    friend class ::MCP::MCPServerFactory;

    // Private constructor - use MCPServer() builder
    Server(unique_ptr<ISession> InSession, ServerCapabilities InCapabilities,
           Implementation InServerInfo, shared_ptr<ITransport> InTransport);

    unique_ptr<ISession> m_Session;
    ServerCapabilities m_Capabilities;
    Implementation m_ServerInfo;
    shared_ptr<ITransport> m_Transport;

    // Internal state
    bool m_IsRunning{false};

    // Registered handlers
    unordered_map<string, pair<Tool, function<MCPTask<ToolResult>(const JSON&)>>> m_Tools;
    unordered_map<string, pair<Resource, function<MCPTask<ResourceContents>()>>> m_Resources;
    unordered_map<string, pair<Prompt, function<MCPTask<GetPromptResult>(const JSON&)>>> m_Prompts;
    vector<Root> m_Roots;

    // Connected clients
    unordered_map<string, ClientCapabilities> m_ConnectedClients;

    // Callbacks
    optional<function<void(const string&)>> m_OnClientConnected;
    optional<function<void(const string&)>> m_OnClientDisconnected;
    optional<function<void(const string&, const ClientCapabilities&)>> m_OnClientInitialized;
    optional<function<void(const string&)>> m_OnError;

    // Internal handlers
    MCPTask<InitializeResult> HandleInitialize(const InitializeRequest& InRequest);
    MCPTask<vector<Tool>> HandleListTools();
    MCPTask<ToolResult> HandleCallTool(const string& InName, const JSON& InArguments);
    MCPTask<vector<Resource>> HandleListResources();
    MCPTask<ResourceContents> HandleReadResource(const string& InURI);
    MCPTask<vector<Prompt>> HandleListPrompts();
    MCPTask<GetPromptResult> HandleGetPrompt(const string& InName, const JSON& InArguments);
    MCPTask<vector<Root>> HandleListRoots();
};

// Beautiful, human-readable builder pattern
class MCPServerFactory {
  public:
    MCPServerFactory();

    // Transport configuration
    MCPServerFactory& WithTransport(TransportType InType);
    MCPServerFactory& WithTransport(shared_ptr<ITransport> InTransport);

    // Stdio-specific options (for stdio transport)
    MCPServerFactory& WithStdio();

    // HTTP-specific options
    MCPServerFactory& WithHTTPServer(const string& InHost = "localhost", int InPort = 8080,
                                     bool InCORS = true);

    // WebSocket-specific options
    MCPServerFactory& WithWebSocketServer(const string& InHost = "localhost", int InPort = 8080);

    // Server capabilities
    MCPServerFactory& WithCapabilities(const ServerCapabilities& InCapabilities);
    MCPServerFactory& WithToolsCapability(bool InEnabled = true);
    MCPServerFactory& WithResourcesCapability(bool InEnabled = true);
    MCPServerFactory& WithPromptsCapability(bool InEnabled = true);
    MCPServerFactory& WithRootsCapability(bool InEnabled = true);
    MCPServerFactory& WithSamplingCapability(bool InEnabled = true);
    MCPServerFactory& WithLoggingCapability(bool InEnabled = true);

    // Server information
    MCPServerFactory& WithServerInfo(const Implementation& InServerInfo);
    MCPServerFactory& WithServerInfo(const string& InName, const string& InVersion);

    // Server instructions
    MCPServerFactory& WithInstructions(const string& InInstructions);

    // Session configuration
    MCPServerFactory& WithTimeout(chrono::milliseconds InTimeout);
    MCPServerFactory& WithMaxClients(size_t InMaxClients);
    MCPServerFactory& WithSessionConfig(const SessionConfig& InConfig);

    // Build the server
    MCP::Server Build();

  private:
    // Transport settings
    TransportType m_TransportType{TransportType::Stdio};
    shared_ptr<ITransport> m_CustomTransport;

    // HTTP settings
    optional<string> m_HTTPHost;
    optional<int> m_HTTPPort;
    optional<bool> m_HTTPCors;

    // WebSocket settings
    optional<string> m_WebSocketHost;
    optional<int> m_WebSocketPort;

    // Capabilities
    ServerCapabilities m_Capabilities;

    // Server info
    Implementation m_ServerInfo{"MCPServer", "1.0.0"};

    // Instructions
    optional<string> m_Instructions;

    // Session config
    SessionConfig m_SessionConfig;

    // Additional config
    optional<size_t> m_MaxClients;

    shared_ptr<ITransport> CreateTransport();
};

// Global factory function
inline MCPServerFactory MCPServer() {
    return MCPServerFactory();
}

// Example usage:
/*
auto server = MCPServer()
    .WithTransport(TransportType::HTTP)
    .WithHTTPServer("localhost", 8080)
    .WithToolsCapability(true)
    .WithResourcesCapability(true)
    .WithServerInfo("MyMCPServer", "1.0.0")
    .WithInstructions("A helpful MCP server")
    .Build();

// Register tools
server.RegisterTool(
    Tool{"calculate", "Performs mathematical calculations", ...},
    [](const JSON& args) -> MCPTask<ToolResult> {
        // Tool implementation
        co_return ToolResult{...};
    }
);

// Register resources
server.RegisterResource(
    Resource{"file://data.txt", "Data file", ...},
    []() -> MCPTask<ResourceContents> {
        // Resource implementation
        co_return ResourceContents{...};
    }
);

// Start server
co_await server.Start();
*/

MCP_NAMESPACE_END
