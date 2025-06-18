#pragma once

#include "Core.h"
#include "Sandbox/IMCP.h"
#include "StreamableHTTPBase.h"

// Poco Net includes
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Thread.h>

MCP_NAMESPACE_BEGIN

// Forward declarations
class StreamableHTTPRequestHandler;
class StreamableHTTPRequestHandlerFactory;

// Configuration options for HTTP server transport
struct HTTPServerConfig {
    string Host{"localhost"};
    int Port{8080};
    string BasePath{"/"};
    bool EnableResumability{false};
    bool EnableStatefulMode{true};
    std::chrono::milliseconds SessionTimeout{300000}; // 5 minutes
    int MaxConcurrentConnections{100};
    int ThreadPoolSize{10};
    bool EnableCORS{false};
    vector<string> AllowedOrigins;
    shared_ptr<EventStore> EventStore;
    bool EnableHTTPS{false};
    string CertificatePath;
    string PrivateKeyPath;
};

// Server implementation of Streamable HTTP transport
// Handles server-specific transport behavior only
class StreamableHTTPServer : public StreamableHTTPBase {
  public:
    explicit StreamableHTTPServer(const HTTPServerConfig& InConfig);
    virtual ~StreamableHTTPServer() = default;

    // === Server-Specific Operations ===

    /**
     * Starts HTTP server to accept connections
     * Initializes server socket and begins listening for requests
     */
    virtual MCPTask_Void StartHTTPServer();

    /**
     * Stops HTTP server
     * Gracefully shuts down server and closes all client connections
     */
    virtual MCPTask_Void StopHTTPServer();

    /**
     * Server-specific connection handling
     * Manages incoming HTTP connections and creates request handlers
     */
    virtual MCPTask_Void HandleIncomingConnection();

    // === Server Connection Management ===

    /**
     * Handles multiple client sessions
     * Manages session lifecycle for all connected clients
     */
    virtual void ManageServerSessions();

    /**
     * Server-side cleanup when client disconnects
     * Removes session data and releases resources
     */
    virtual MCPTask_Void HandleClientDisconnection(const string& InSessionID);

    /**
     * Sends message to connected clients
     * Supports both unicast (specific session) and broadcast scenarios
     */
    virtual MCPTask_Void BroadcastToClients(const MessageBase& InMessage,
                                            const optional<string>& InTargetSessionID = nullopt);

    // === Server Request Routing ===

    /**
     * Routes requests to base class handlers
     * Determines appropriate handler based on HTTP method and path
     */
    virtual MCPTask_Void RouteIncomingRequest(const Poco::Net::HTTPRequest& InRequest,
                                              Poco::Net::HTTPResponse& OutResponse);

    /**
     * Decides between JSON response or SSE stream
     * Based on message type and client capabilities
     */
    virtual string DetermineResponseType(const MessageBase& InMessage);

    // === Configuration Access ===

    /**
     * Gets the current server configuration
     */
    const HTTPServerConfig& GetConfig() const {
        return m_Config;
    }

    /**
     * Updates server configuration (requires restart)
     */
    void UpdateConfig(const HTTPServerConfig& InNewConfig);

    /**
     * Gets current connection statistics
     */
    struct ConnectionStats {
        int ActiveConnections{0};
        int TotalSessions{0};
        std::chrono::steady_clock::time_point ServerStartTime;
        map<string, std::chrono::steady_clock::time_point> SessionLastActivity;
    };
    ConnectionStats GetConnectionStats() const;

  protected:
    // === Protected Server Methods ===

    /**
     * Creates and configures server socket
     */
    virtual unique_ptr<Poco::Net::ServerSocket> CreateServerSocket();

    /**
     * Creates request handler factory for incoming requests
     */
    virtual unique_ptr<Poco::Net::HTTPRequestHandlerFactory> CreateRequestHandlerFactory();

    /**
     * Manages session expiration and cleanup
     */
    virtual void CleanupExpiredSessions();

    /**
     * Validates incoming request for server requirements
     */
    virtual bool ValidateIncomingRequest(const Poco::Net::HTTPRequest& InRequest);

    /**
     * Creates new session for connecting client
     */
    virtual string CreateClientSession(const Poco::Net::HTTPRequest& InRequest);

    /**
     * Removes client session and associated resources
     */
    virtual void RemoveClientSession(const string& InSessionID);

  private:
    // === Member Variables ===

    // Server configuration
    HTTPServerConfig m_Config;

    // HTTP server components
    unique_ptr<Poco::Net::HTTPServer> m_HTTPServer;
    unique_ptr<Poco::Net::ServerSocket> m_ServerSocket;
    unique_ptr<Poco::Net::HTTPRequestHandlerFactory> m_HandlerFactory;

    // Session management
    map<string, std::chrono::steady_clock::time_point> m_ClientSessions;
    map<string, unique_ptr<std::ostream>> m_SSEStreams; // Session ID -> SSE stream

    // Server state
    bool m_IsRunning{false};
    std::chrono::steady_clock::time_point m_ServerStartTime;
    std::atomic<int> m_ActiveConnections{0};

    // Session cleanup
    std::thread m_SessionCleanupThread;
    std::atomic<bool> m_ShouldStopCleanup{false};

    // Thread synchronization
    mutable std::mutex m_SessionsMutex;
    mutable std::mutex m_StreamsMutex;
    mutable std::mutex m_ConfigMutex;
    mutable std::mutex m_StatsMutex;
};

// HTTP Request Handler for MCP messages
class StreamableHTTPRequestHandler : public Poco::Net::HTTPRequestHandler {
  public:
    explicit StreamableHTTPRequestHandler(StreamableHTTPServer& InServer);

    void handleRequest(Poco::Net::HTTPServerRequest& InRequest,
                       Poco::Net::HTTPServerResponse& OutResponse) override;

  private:
    StreamableHTTPServer& m_Server;
};

// Request Handler Factory for creating request handlers
class StreamableHTTPRequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory {
  public:
    explicit StreamableHTTPRequestHandlerFactory(StreamableHTTPServer& InServer);

    Poco::Net::HTTPRequestHandler*
    createRequestHandler(const Poco::Net::HTTPServerRequest& InRequest) override;

  private:
    StreamableHTTPServer& m_Server;
};

MCP_NAMESPACE_END