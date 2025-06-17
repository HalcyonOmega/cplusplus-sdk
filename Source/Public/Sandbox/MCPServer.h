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
    friend class MCPServerFactory;

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

// Clean factory API - builds automatically when no more chaining
class MCPServerFactory {
  public:
    // Required transport configuration in constructor - auto-builds if no chaining

    // Stdio transport constructor (for process-based servers)
    MCPServerFactory(TransportType InType);

    // HTTP server constructor
    MCPServerFactory(TransportType InType, const string& InHost = "localhost", int InPort = 8080);

    // WebSocket server constructor
    MCPServerFactory(TransportType InType, const string& InHost, int InPort);

    // Custom transport constructor
    MCPServerFactory(shared_ptr<ITransport> InTransport);

    // === Grouped Configuration (chainable) ===

    // HTTP-specific options
    struct HTTPOptions {
        optional<string> Host;
        optional<int> Port;
        optional<bool> CORS;
        optional<chrono::milliseconds> Timeout;
        optional<size_t> MaxConnections;
    };
    MCPServerFactory& HTTPOptions(const HTTPOptions& InOptions);

    // WebSocket-specific options
    struct WebSocketOptions {
        optional<string> Host;
        optional<int> Port;
        optional<chrono::milliseconds> PingInterval;
        optional<size_t> MaxFrameSize;
        optional<size_t> MaxConnections;
    };
    MCPServerFactory& WebSocketOptions(const WebSocketOptions& InOptions);

    // All capabilities grouped (defaults to all false)
    struct CapabilityOptions {
        bool Tools{false};
        bool Resources{false};
        bool Prompts{false};
        bool Roots{false};
        bool Sampling{false};
        bool Logging{false};
    };
    MCPServerFactory& Capabilities(const CapabilityOptions& InCapabilities);

    // Individual capabilities (auto-enables when called)
    MCPServerFactory& Tools();
    MCPServerFactory& Resources();
    MCPServerFactory& Prompts();
    MCPServerFactory& Roots();
    MCPServerFactory& Sampling();
    MCPServerFactory& Logging();

    // Server information
    MCPServerFactory& ServerInfo(const string& InName, const string& InVersion);
    MCPServerFactory& Instructions(const string& InInstructions);

    // Session settings
    MCPServerFactory& SessionOptions(chrono::milliseconds InTimeout, size_t InMaxClients = 100);

    // Auto-build when destroyed or explicitly converted
    operator Server();

  private:
    // Transport settings
    TransportType m_TransportType;
    shared_ptr<ITransport> m_CustomTransport;

    // Transport-specific config defaults
    string m_DefaultHost{"localhost"};
    int m_DefaultPort{8080};

    // Grouped options
    optional<HTTPOptions> m_HTTPOptions;
    optional<WebSocketOptions> m_WebSocketOptions;

    // Capabilities (defaults to all false)
    CapabilityOptions m_Capabilities;

    // Server info
    Implementation m_ServerInfo{"MCPServer", "1.0.0"};

    // Instructions
    optional<string> m_Instructions;

    // Session config
    SessionConfig m_SessionConfig;

    // Additional config
    optional<size_t> m_MaxClients;

    shared_ptr<ITransport> CreateTransport();
    ServerCapabilities BuildCapabilities();
};

// Global factory functions for clean API
inline MCPServerFactory MCPServer(TransportType InType) {
    return MCPServerFactory(InType);
}

inline MCPServerFactory MCPServer(TransportType InType, const string& InHost = "localhost",
                                  int InPort = 8080) {
    return MCPServerFactory(InType, InHost, InPort);
}

inline MCPServerFactory MCPServer(shared_ptr<ITransport> InTransport) {
    return MCPServerFactory(InTransport);
}

// Example usage - much cleaner and auto-builds!
/*
// Stdio server - auto-builds after constructor
auto server1 = MCPServer(TransportType::Stdio);

// HTTP server with grouped options - auto-builds after last chain
auto server2 = MCPServer(TransportType::HTTP)
    .HTTPOptions({.Port = 9001, .CORS = true}) // Keep default host, change port
    .Tools()
    .Resources()
    .ServerInfo("MyMCPServer", "2.0.0");

// With capabilities grouped
auto server3 = MCPServer(TransportType::WebSocket, "0.0.0.0", 8080)
    .Capabilities({.Tools = true, .Resources = true, .Prompts = false})
    .Instructions("A helpful MCP server")
    .SessionOptions(chrono::seconds(30), 10);

// Register and start (server built automatically)
server1.RegisterTool(myTool, myToolHandler);
co_await server1.Start();
*/

MCP_NAMESPACE_END
