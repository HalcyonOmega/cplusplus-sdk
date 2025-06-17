#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Core.h"
#include "IProtocol.h"
#include "ISession.h"
#include "ITransport.h"

MCP_NAMESPACE_BEGIN

// Forward declarations
class ConnectionPoolFactory;
class SessionManagementFactory;
class SecurityFactory;
class MultiClientServerImpl;

// Transport types for server creation
enum class ServerTransportType {
    Stdio,    // Standard input/output (for process-based servers)
    HTTP,     // HTTP server
    WebSocket // WebSocket server
};

// Load balancing strategies
enum class LoadBalanceStrategy { RoundRobin, LeastConnections, Random, HealthBased };

// Client information passed to callbacks
struct ClientInfo {
    string ClientId;
    string RemoteEndpoint;
    chrono::steady_clock::time_point ConnectedAt;
    unordered_map<string, string> Headers; // For HTTP clients

    string GetId() const {
        return ClientId;
    }
    string GetEndpoint() const {
        return RemoteEndpoint;
    }
    chrono::milliseconds GetConnectionDuration() const {
        return chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now()
                                                           - ConnectedAt);
    }
};

// Retry configuration
struct RetryConfig {
    size_t MaxRetries{3};
    chrono::milliseconds InitialDelay{1000};
    chrono::milliseconds MaxDelay{30000};
    double BackoffMultiplier{2.0};
};

// Main Server class with progressive disclosure API
class Server {
  public:
    explicit Server(ServerTransportType InType);
    ~Server() = default;

    // === Core API (Simple Usage) ===

    void AddTool(const string& InName, const string& InDescription,
                 function<MCPTask<JSON>(const JSON&)> InHandler);
    void AddResource(const string& InURI, const string& InName,
                     function<MCPTask<string>()> InHandler);
    void AddPrompt(const string& InName, const string& InDescription,
                   function<MCPTask<string>(const JSON&)> InHandler);

    MCPTask_Void Start();
    MCPTask_Void Stop();
    bool IsRunning() const;

    // === Advanced Configuration (Method Chaining) ===

    // Multi-client settings
    Server& MaxClients(size_t InMax);
    Server& ConnectionTimeout(chrono::milliseconds InTimeout);
    Server& RequestTimeout(chrono::milliseconds InTimeout);

    // HTTP-specific settings
    Server& HTTPPort(int InPort);
    Server& HTTPHost(const string& InHost);
    Server& EnableCORS(bool InEnable = true);

    // WebSocket-specific settings
    Server& WebSocketPort(int InPort);
    Server& WebSocketHost(const string& InHost);
    Server& PingInterval(chrono::milliseconds InInterval);

    // Client lifecycle callbacks
    Server& OnClientConnected(function<void(const ClientInfo&)> InCallback);
    Server& OnClientDisconnected(function<void(const string&)> InCallback);
    Server& OnError(function<void(const string&)> InCallback);

    // Advanced feature builders
    ConnectionPoolFactory& ConnectionPool();
    SessionManagementFactory& SessionManagement();
    SecurityFactory& Security();

    // Server information
    size_t GetConnectedClientsCount() const;
    vector<ClientInfo> GetConnectedClients() const;

  private:
    friend class ConnectionPoolFactory;
    friend class SessionManagementFactory;
    friend class SecurityFactory;

    ServerTransportType m_TransportType;
    bool m_IsRunning{false};

    // Basic configuration
    optional<size_t> m_MaxClients;
    optional<chrono::milliseconds> m_ConnectionTimeout;
    optional<chrono::milliseconds> m_RequestTimeout;

    // Transport-specific configuration
    optional<int> m_HTTPPort;
    optional<string> m_HTTPHost;
    optional<bool> m_EnableCORS;
    optional<int> m_WebSocketPort;
    optional<string> m_WebSocketHost;
    optional<chrono::milliseconds> m_PingInterval;

    // Callbacks
    optional<function<void(const ClientInfo&)>> m_OnClientConnected;
    optional<function<void(const string&)>> m_OnClientDisconnected;
    optional<function<void(const string&)>> m_OnError;

    // Tool/Resource/Prompt handlers
    unordered_map<string, function<MCPTask<JSON>(const JSON&)>> m_ToolHandlers;
    unordered_map<string, function<MCPTask<string>()>> m_ResourceHandlers;
    unordered_map<string, function<MCPTask<string>(const JSON&)>> m_PromptHandlers;

    // Sub-builders
    unique_ptr<ConnectionPoolFactory> m_PoolFactory;
    unique_ptr<SessionManagementFactory> m_SessionFactory;
    unique_ptr<SecurityFactory> m_SecurityFactory;

    // Internal implementation
    unique_ptr<MultiClientServerImpl> m_Impl;

    void InitializeFactorys();
    void ApplyConfiguration();
};

// Connection pool configuration builder
class ConnectionPoolFactory {
  public:
    explicit ConnectionPoolFactory(Server& InServer);

    ConnectionPoolFactory& AddServer(const string& InName, const string& InEndpoint);
    ConnectionPoolFactory& RemoveServer(const string& InName);
    ConnectionPoolFactory& SetIdleTimeout(chrono::milliseconds InTimeout);
    ConnectionPoolFactory& SetMaxConnectionsPerServer(size_t InMax);
    ConnectionPoolFactory& LoadBalancing(LoadBalanceStrategy InStrategy);
    ConnectionPoolFactory& EnableHealthCheck(bool InEnable = true);
    ConnectionPoolFactory& HealthCheckInterval(chrono::milliseconds InInterval);

    // Return to main builder
    Server& Done();
    operator Server&();

  private:
    Server& m_Server;
    unordered_map<string, string> m_Servers;
    optional<chrono::milliseconds> m_IdleTimeout;
    optional<size_t> m_MaxConnectionsPerServer;
    optional<LoadBalanceStrategy> m_LoadBalanceStrategy;
    optional<bool> m_EnableHealthCheck;
    optional<chrono::milliseconds> m_HealthCheckInterval;
};

// Session management configuration builder
class SessionManagementFactory {
  public:
    explicit SessionManagementFactory(Server& InServer);

    SessionManagementFactory& SetSessionFactory(function<unique_ptr<SessionBase>()> InFactory);
    SessionManagementFactory& EnableAutoReconnect(bool InEnable = true);
    SessionManagementFactory& SetRetryPolicy(const RetryConfig& InConfig);
    SessionManagementFactory& SessionTimeout(chrono::milliseconds InTimeout);
    SessionManagementFactory& MaxSessionsPerClient(size_t InMax);
    SessionManagementFactory& EnableSessionPersistence(bool InEnable = true);

    // Session lifecycle callbacks
    SessionManagementFactory& OnSessionCreated(function<void(const string&)> InCallback);
    SessionManagementFactory& OnSessionDestroyed(function<void(const string&)> InCallback);

    Server& Done();
    operator Server&();

  private:
    Server& m_Server;
    optional<function<unique_ptr<SessionBase>()>> m_SessionFactory;
    optional<bool> m_AutoReconnect;
    optional<RetryConfig> m_RetryConfig;
    optional<chrono::milliseconds> m_SessionTimeout;
    optional<size_t> m_MaxSessionsPerClient;
    optional<bool> m_EnableSessionPersistence;
    optional<function<void(const string&)>> m_OnSessionCreated;
    optional<function<void(const string&)>> m_OnSessionDestroyed;
};

// Security configuration builder
class SecurityFactory {
  public:
    explicit SecurityFactory(Server& InServer);

    SecurityFactory& EnableTLS(bool InEnable = true);
    SecurityFactory& SetCertificate(const string& InCertPath);
    SecurityFactory& SetPrivateKey(const string& InKeyPath);
    SecurityFactory& EnableAuthentication(bool InEnable = true);
    SecurityFactory& SetAuthHandler(function<bool(const string&)> InHandler);
    SecurityFactory& EnableRateLimiting(bool InEnable = true);
    SecurityFactory& SetRateLimit(size_t InRequestsPerSecond);
    SecurityFactory& SetAllowedOrigins(const vector<string>& InOrigins);

    Server& Done();
    operator Server&();

  private:
    Server& m_Server;
    optional<bool> m_EnableTLS;
    optional<string> m_CertPath;
    optional<string> m_KeyPath;
    optional<bool> m_EnableAuth;
    optional<function<bool(const string&)>> m_AuthHandler;
    optional<bool> m_EnableRateLimit;
    optional<size_t> m_RateLimit;
    optional<vector<string>> m_AllowedOrigins;
};

// Clean factory function
inline Server MCPServer(ServerTransportType InType) {
    return Server(InType);
}

MCP_NAMESPACE_END

// === Usage Examples ===
//
// // Simple case (unchanged)
// auto server = MCPServer(ServerTransportType::HTTP);
// server.AddTool("greet", "Say hello", handler);
// co_await server.Start();
//
// // Moderate complexity
// auto server = MCPServer(ServerTransportType::HTTP)
//     .HTTPPort(9001)
//     .MaxClients(100)
//     .OnClientConnected([](const ClientInfo& client) {
//         cout << "Client " << client.GetId() << " connected\n";
//     });
//
// // Advanced features
// auto server = MCPServer(ServerTransportType::HTTP)
//     .MaxClients(1000)
//     .ConnectionTimeout(30s)
//     .ConnectionPool()
//         .AddServer("backup", "http://backup:8080")
//         .SetIdleTimeout(60s)
//         .LoadBalancing(LoadBalanceStrategy::RoundRobin)
//     .SessionManagement()
//         .EnableAutoReconnect(true)
//         .SetRetryPolicy({.MaxRetries = 5})
//     .Security()
//         .EnableTLS(true)
//         .SetCertificate("cert.pem");
//
