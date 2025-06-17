#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "Core.h"
#include "Core/Features/Prompt/Prompts.h"
#include "Core/Features/Resource/Resources.h"
#include "Core/Features/Tool/Tools.h"
#include "IProtocol.h"
#include "ISession.h"
#include "ITransport.h"

MCP_NAMESPACE_BEGIN

// Transport types for easy server creation
enum class ServerTransportType {
    Stdio,    // Standard input/output (for process-based servers)
    HTTP,     // HTTP server
    WebSocket // WebSocket server
};

// Simple configuration structs
struct HTTPServerOptions {
    string Host = "localhost";
    int Port = 8080;
    bool EnableCORS = true;
    chrono::milliseconds Timeout = chrono::seconds(30);
    size_t MaxConnections = 100;
};

struct WebSocketServerOptions {
    string Host = "localhost";
    int Port = 8080;
    chrono::milliseconds PingInterval = chrono::seconds(30);
    size_t MaxConnections = 100;
};

// Main MCP Server class - encapsulates all complexity
class Server {
  public:
    ~Server() = default;

    // === Simple Registration API ===

    // Add tool with handler function
    void AddTool(const string& InName, const string& InDescription,
                 function<MCPTask<JSON>(const JSON&)> InHandler);

    // Add resource with handler function
    void AddResource(const string& InURI, const string& InName,
                     function<MCPTask<string>()> InHandler);

    // Add prompt with handler function
    void AddPrompt(const string& InName, const string& InDescription,
                   function<MCPTask<string>(const JSON&)> InHandler);

    // === Server Lifecycle ===

    // Start the server - handles all transport setup internally
    MCPTask_Void Start();

    // Stop the server gracefully
    MCPTask_Void Stop();

    // Check if running
    bool IsRunning() const;

    // === Server Information ===

    size_t GetConnectedClientsCount() const;

    // === Event Callbacks (optional) ===

    void OnClientConnected(function<void(const string&)> InCallback);
    void OnClientDisconnected(function<void(const string&)> InCallback);
    void OnError(function<void(const string&)> InCallback);

    // TODO: @HalcyonOmega Simple Server API
    MCPTask<PingResponse> RequestPing(const PingRequest& InRequest = {});

    MCPTask<CreateMessageResponse> RequestCreateMessage(const CreateMessageRequest& InRequest = {});

    MCPTask<ListRootsResponse> RequestListRoots(const ListRootsRequest& InRequest = {});

    MCPTask_Void NotifyLoggingMessage(const LoggingMessageNotification& InMessage);

    MCPTask_Void NotifyResourceUpdated(const ResourceUpdatedNotification& InMessage);

    MCPTask_Void NotifyResourceListChanged(const ResourceListChangedNotification& InMessage);

    MCPTask_Void NotifyToolListChanged(const ToolListChangedNotification& InMessage);

    MCPTask_Void NotifyPromptListChanged(const PromptListChangedNotification& InMessage);
    // TODO: @HalcyonOmega End Simple Server API

  private:
    friend class MCPServerBuilder;

    // Private constructor - use MCPServer() factory
    Server(ServerTransportType InType, HTTPServerOptions InHTTPOptions,
           WebSocketServerOptions InWSOptions);

    // Internal components (hidden from user)
    shared_ptr<ITransport> m_Transport;

    // Configuration
    ServerTransportType m_TransportType;
    HTTPServerOptions m_HTTPOptions;
    WebSocketServerOptions m_WSOptions;

    // State
    bool m_IsRunning{false};

    // Registered handlers
    unordered_map<string, function<MCPTask<JSON>(const JSON&)>> m_ToolHandlers;
    unordered_map<string, function<MCPTask<string>()>> m_ResourceHandlers;
    unordered_map<string, function<MCPTask<string>(const JSON&)>> m_PromptHandlers;

    // Internal setup
    void SetupTransport();
    void SetupProtocol();
    void SetupSession();
    void RegisterInternalHandlers();
};

// Simple builder for clean API
class MCPServerBuilder {
  public:
    explicit MCPServerBuilder(ServerTransportType InType);

    // HTTP configuration
    MCPServerBuilder& HTTPOptions(const HTTPServerOptions& InOptions);

    // WebSocket configuration
    MCPServerBuilder& WebSocketOptions(const WebSocketServerOptions& InOptions);

    // Build the server
    Server Build();

    // Auto-conversion to Server
    operator Server();

  private:
    ServerTransportType m_TransportType;
    HTTPServerOptions m_HTTPOptions;
    WebSocketServerOptions m_WSOptions;
};

// Clean factory function
inline MCPServerBuilder MCPServer(ServerTransportType InType) {
    return MCPServerBuilder(InType);
}

MCP_NAMESPACE_END

// === Usage Examples ===
//
// // Simple stdio server
// auto Server = MCPServer(ServerTransportType::Stdio);
// Server.AddTool("greet", "Greet someone", [](const JSON& args) -> MCPTask<JSON> {
//     co_return JSON{{"message", "Hello " + args["name"].get<string>()}};
// });
// co_await Server.Start();
//
// // HTTP server with custom port
// auto HTTPServer = MCPServer(ServerTransportType::HTTP)
//     .HTTPOptions({.Port = 9001});
// HTTPServer.AddTool("greet", "Say hello", myHandler);
// co_await HTTPServer.Start();
//
