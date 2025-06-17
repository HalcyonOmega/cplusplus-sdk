#pragma once

#include <atomic>
#include <chrono>
#include <coroutine>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "Core.h"
#include "Core/Types/Capabilities.h"
#include "Core/Types/Implementation.h"
#include "Core/Types/Initialization.h"
#include "IProtocol.h"
#include "ITransport.h"
#include "MessageBase.h"
#include "RequestBase.h"
#include "ResponseBase.h"

MCP_NAMESPACE_BEGIN

// Session state enumeration following MCP lifecycle
enum class SessionState {
    Disconnected, // Initial state, no connection
    Connecting,   // Establishing transport connection
    Initializing, // MCP initialization phase (initialize request/response)
    Initialized,  // Ready for operation (after initialized notification)
    Operating,    // Normal MCP operations
    ShuttingDown, // Graceful shutdown in progress
    Terminated,   // Connection closed cleanly
    Error         // Error state
};

// Session configuration
struct SessionConfig {
    string ProtocolVersion = LATEST_PROTOCOL_VERSION;
    chrono::milliseconds ConnectionTimeout{30000};
    chrono::milliseconds RequestTimeout{30000};
    chrono::milliseconds InitializationTimeout{10000};
    bool AllowBatchRequests{true};
    size_t MaxConcurrentRequests{100};
    size_t MaxMessageSize{1024 * 1024}; // 1MB

    // Retry configuration
    struct RetryConfig {
        size_t MaxRetries{3};
        chrono::milliseconds InitialDelay{1000};
        chrono::milliseconds MaxDelay{30000};
        double BackoffMultiplier{2.0};
    } Retry;
};

// Session callbacks for events
struct SessionCallbacks {
    function<void(SessionState, SessionState)> OnStateChanged;
    function<void(const string&)> OnError;
    function<void(const NotificationBase&)> OnNotification;
    function<void(const RequestBase&, function<void(const JSON&)>)> OnRequest;
    function<void(const string&)> OnDisconnected;
    function<void()> OnInitialized;
};

// Session statistics
struct SessionStats {
    atomic<size_t> RequestsSent{0};
    atomic<size_t> RequestsReceived{0};
    atomic<size_t> NotificationsSent{0};
    atomic<size_t> NotificationsReceived{0};
    atomic<size_t> ErrorsReceived{0};
    atomic<size_t> BytesSent{0};
    atomic<size_t> BytesReceived{0};
    chrono::steady_clock::time_point ConnectionTime;
    chrono::steady_clock::time_point InitializationTime;
    chrono::steady_clock::time_point LastActivity;

    // Get connection duration
    chrono::milliseconds GetConnectionDuration() const {
        return chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now()
                                                           - ConnectionTime);
    }
};

// Negotiated capabilities after successful initialization
struct NegotiatedCapabilities {
    string ProtocolVersion;
    ClientCapabilities Client;
    ServerCapabilities Server;
    optional<string> ServerInstructions;
    Implementation ClientInfo;
    Implementation ServerInfo;
};

// Session Interface
class ISession {
  public:
    virtual ~ISession() = default;

    // === Lifecycle Management (MCP Spec Compliant) ===

    // Initialize the session with given capabilities
    virtual MCPTask<InitializeResult> Initialize(const ClientCapabilities& InClientCapabilities,
                                                 const Implementation& InClientInfo,
                                                 shared_ptr<ITransport> InTransport) = 0;

    // Graceful shutdown following MCP spec
    virtual MCPTask_Void Shutdown() = 0;

    // === State Management ===

    virtual SessionState GetState() const = 0;
    virtual bool IsConnected() const = 0;
    virtual bool IsInitialized() const = 0;
    virtual bool IsOperational() const = 0;

    // === Session Information ===

    virtual optional<string> GetSessionID() const = 0;
    virtual optional<NegotiatedCapabilities> GetCapabilities() const = 0;
    virtual const SessionConfig& GetConfig() const = 0;
    virtual SessionStats GetStats() const = 0;

    // === Transport Access ===

    virtual shared_ptr<ITransport> GetTransport() const = 0;

    // === Event Handling ===

    virtual void SetCallbacks(const SessionCallbacks& InCallbacks) = 0;

    // === Utility Operations ===

    // Send ping and get response time
    virtual MCPTask<chrono::milliseconds> Ping() = 0;

    // Force disconnect (for error recovery)
    virtual void ForceDisconnect() = 0;
};

// Client Session Interface - specific to client-side MCP sessions
class IClientSession : public ISession {
  public:
    // Send the initialized notification (required after successful initialize)
    virtual MCPTask_Void SendInitializedNotification() = 0;

    // Client-specific operations
    virtual MCPTask_Void RequestRoots() = 0;
};

// Server Session Interface - specific to server-side MCP sessions
class IServerSession : public ISession {
  public:
    // Server-specific operations
    virtual void SetInstructions(const string& InInstructions) = 0;
    virtual optional<string> GetInstructions() const = 0;

    // Handle initialize request from client
    virtual MCPTask<InitializeResult>
    HandleInitializeRequest(const InitializeRequest& InRequest) = 0;
};

// Session Factory
class SessionFactory {
  public:
    // Create client session
    static unique_ptr<IClientSession> CreateClientSession(const SessionConfig& InConfig = {});

    // Create server session
    static unique_ptr<IServerSession>
    CreateServerSession(const ServerCapabilities& InServerCapabilities,
                        const Implementation& InServerInfo, const SessionConfig& InConfig = {});

    // Create session with custom transport and protocol
    static unique_ptr<IClientSession>
    CreateClientSessionWithTransport(shared_ptr<ITransport> InTransport,
                                     const SessionConfig& InConfig = {});

    static unique_ptr<IServerSession> CreateServerSessionWithTransport(
        shared_ptr<ITransport> InTransport, const ServerCapabilities& InServerCapabilities,
        const Implementation& InServerInfo, const SessionConfig& InConfig = {});
};

// Session Manager for handling multiple sessions
class SessionManager {
  public:
    SessionManager() = default;
    ~SessionManager();

    // === Session Management ===

    void AddSession(const string& InSessionID, unique_ptr<ISession> InSession);
    void RemoveSession(const string& InSessionID);
    ISession* GetSession(const string& InSessionID);
    vector<string> GetSessionIds() const;

    // === Batch Operations ===

    MCPTask_Void ShutdownAllSessions();
    void CleanupTerminatedSessions();

    // === Statistics and Monitoring ===

    size_t GetActiveSessionCount() const;
    size_t GetSessionCount(SessionState InState) const;
    SessionStats GetAggregateStats() const;
    vector<pair<string, SessionState>> GetSessionStates() const;

    // === Event Handling ===

    using SessionEventCallback = function<void(const string&, SessionState, SessionState)>;
    void SetSessionEventCallback(SessionEventCallback InCallback);

  private:
    unordered_map<string, unique_ptr<ISession>> m_Sessions;
    mutable shared_mutex m_SessionsMutex;
    optional<SessionEventCallback> m_EventCallback;

    void OnSessionStateChanged(const string& InSessionID, SessionState InOldState,
                               SessionState InNewState);
};

// === Utility Functions ===

string ToString(SessionState InState);
bool IsTerminalState(SessionState InState);
bool IsActiveState(SessionState InState);
bool IsErrorState(SessionState InState);

// Session builder for fluent API
class SessionBuilder {
  public:
    SessionBuilder& WithConfig(const SessionConfig& InConfig);
    SessionBuilder& WithTransport(shared_ptr<ITransport> InTransport);
    SessionBuilder& WithCallbacks(const SessionCallbacks& InCallbacks);
    SessionBuilder& WithTimeout(chrono::milliseconds InTimeout);
    SessionBuilder& WithRetryPolicy(const SessionConfig::RetryConfig& InRetry);

    // Build client session
    unique_ptr<IClientSession> BuildClient();

    // Build server session
    unique_ptr<IServerSession> BuildServer(const ServerCapabilities& InCapabilities,
                                           const Implementation& InServerInfo);

  private:
    SessionConfig m_Config;
    shared_ptr<ITransport> m_Transport;
    optional<SessionCallbacks> m_Callbacks;
};

// Example usage with coroutines:
/*
ProtocolTask<void> ExampleClientSession() {
    auto session = SessionFactory::CreateClientSession();

    ClientCapabilities capabilities;
    capabilities.Roots = ClientCapabilities::RootsCapability{true};

    Implementation clientInfo{"MyClient", "1.0.0"};
    auto transport = TransportFactory::CreateStdioTransport();
    auto protocol = ProtocolFactory::CreateClientProtocol(capabilities, clientInfo);

    try {
        // Initialize with coroutines - much cleaner than callbacks!
        auto result = co_await session->Initialize(capabilities, clientInfo, transport, protocol);
        if (result.HasError()) {
            co_return; // Handle error
        }

        // Send initialized notification
        co_await session->SendInitializedNotification();

        // Now we can do normal operations
        auto pingTime = co_await session->Ping();

        // Graceful shutdown
        co_await session->Shutdown();

    } catch (...) {
        // Handle exceptions
        session->ForceDisconnect();
    }
}
*/

MCP_NAMESPACE_END
