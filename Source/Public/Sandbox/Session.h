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

#include "../Core/Includes/Core.h"
#include "Protocol.h"

MCP_NAMESPACE_BEGIN

// Forward declarations
class ITransport;
class IProtocol;

// Session state enumeration following MCP lifecycle
enum class SessionState {
    Disconnected,  // Initial state, no connection
    Connecting,    // Establishing transport connection
    Initializing,  // MCP initialization phase (initialize request/response)
    Initialized,   // Ready for operation (after initialized notification)
    Operating,     // Normal MCP operations
    Shutting_Down, // Graceful shutdown in progress
    Terminated,    // Connection closed cleanly
    Error          // Error state
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
    function<void(const JSON_RPCNotification&)> OnNotification;
    function<void(const JSON_RPCRequest&, function<void(const JSON&)>)> OnRequest;
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

// Session Interface - combines Gemini's clean interface with coroutine support
class ISession {
  public:
    virtual ~ISession() = default;

    // === Lifecycle Management (MCP Spec Compliant) ===

    // Initialize the session with given capabilities
    virtual future<InitializeResult> Initialize(const ClientCapabilities& clientCapabilities,
                                                const Implementation& clientInfo,
                                                shared_ptr<ITransport> transport,
                                                shared_ptr<IProtocol> protocol) = 0;

    // Coroutine version for better async experience
    virtual ProtocolTask<InitializeResult>
    InitializeAsync(const ClientCapabilities& clientCapabilities, const Implementation& clientInfo,
                    shared_ptr<ITransport> transport, shared_ptr<IProtocol> protocol) = 0;

    // Graceful shutdown following MCP spec
    virtual future<void> Shutdown() = 0;
    virtual ProtocolTask<void> ShutdownAsync() = 0;

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

    // === Protocol and Transport Access ===

    virtual shared_ptr<IProtocol> GetProtocol() const = 0;
    virtual shared_ptr<ITransport> GetTransport() const = 0;

    // === Event Handling ===

    virtual void SetCallbacks(const SessionCallbacks& callbacks) = 0;

    // === Utility Operations ===

    // Send ping and get response time
    virtual future<chrono::milliseconds> Ping() = 0;
    virtual ProtocolTask<chrono::milliseconds> PingAsync() = 0;

    // Force disconnect (for error recovery)
    virtual void ForceDisconnect() = 0;
};

// Client Session Interface - specific to client-side MCP sessions
class IClientSession : public ISession {
  public:
    // Send the initialized notification (required after successful initialize)
    virtual future<void> SendInitializedNotification() = 0;
    virtual ProtocolTask<void> SendInitializedNotificationAsync() = 0;

    // Client-specific operations
    virtual future<void> RequestRoots() = 0;
    virtual ProtocolTask<void> RequestRootsAsync() = 0;
};

// Server Session Interface - specific to server-side MCP sessions
class IServerSession : public ISession {
  public:
    // Server-specific operations
    virtual void SetInstructions(const string& instructions) = 0;
    virtual optional<string> GetInstructions() const = 0;

    // Handle initialize request from client
    virtual future<InitializeResult> HandleInitializeRequest(const InitializeRequest& request) = 0;
    virtual ProtocolTask<InitializeResult>
    HandleInitializeRequestAsync(const InitializeRequest& request) = 0;
};

// Session Factory
class SessionFactory {
  public:
    // Create client session
    static unique_ptr<IClientSession> CreateClientSession(const SessionConfig& config = {});

    // Create server session
    static unique_ptr<IServerSession>
    CreateServerSession(const ServerCapabilities& serverCapabilities,
                        const Implementation& serverInfo, const SessionConfig& config = {});

    // Create session with custom transport and protocol
    static unique_ptr<IClientSession>
    CreateClientSessionWithTransport(shared_ptr<ITransport> transport,
                                     shared_ptr<IProtocol> protocol,
                                     const SessionConfig& config = {});

    static unique_ptr<IServerSession> CreateServerSessionWithTransport(
        shared_ptr<ITransport> transport, shared_ptr<IProtocol> protocol,
        const ServerCapabilities& serverCapabilities, const Implementation& serverInfo,
        const SessionConfig& config = {});
};

// Session Manager for handling multiple sessions
class SessionManager {
  public:
    SessionManager() = default;
    ~SessionManager();

    // === Session Management ===

    void AddSession(const string& sessionId, unique_ptr<ISession> session);
    void RemoveSession(const string& sessionId);
    ISession* GetSession(const string& sessionId);
    vector<string> GetSessionIds() const;

    // === Batch Operations ===

    future<void> ShutdownAllSessions();
    ProtocolTask<void> ShutdownAllSessionsAsync();
    void CleanupTerminatedSessions();

    // === Statistics and Monitoring ===

    size_t GetActiveSessionCount() const;
    size_t GetSessionCount(SessionState state) const;
    SessionStats GetAggregateStats() const;
    vector<pair<string, SessionState>> GetSessionStates() const;

    // === Event Handling ===

    using SessionEventCallback = function<void(const string&, SessionState, SessionState)>;
    void SetSessionEventCallback(SessionEventCallback callback);

  private:
    unordered_map<string, unique_ptr<ISession>> m_Sessions;
    mutable shared_mutex m_SessionsMutex;
    optional<SessionEventCallback> m_EventCallback;

    void OnSessionStateChanged(const string& sessionId, SessionState oldState,
                               SessionState newState);
};

// === Utility Functions ===

string ToString(SessionState state);
bool IsTerminalState(SessionState state);
bool IsActiveState(SessionState state);
bool IsErrorState(SessionState state);

// Session builder for fluent API
class SessionBuilder {
  public:
    SessionBuilder& WithConfig(const SessionConfig& config);
    SessionBuilder& WithTransport(shared_ptr<ITransport> transport);
    SessionBuilder& WithProtocol(shared_ptr<IProtocol> protocol);
    SessionBuilder& WithCallbacks(const SessionCallbacks& callbacks);
    SessionBuilder& WithTimeout(chrono::milliseconds timeout);
    SessionBuilder& WithRetryPolicy(const SessionConfig::RetryConfig& retry);

    // Build client session
    unique_ptr<IClientSession> BuildClient();

    // Build server session
    unique_ptr<IServerSession> BuildServer(const ServerCapabilities& capabilities,
                                           const Implementation& serverInfo);

  private:
    SessionConfig m_Config;
    shared_ptr<ITransport> m_Transport;
    shared_ptr<IProtocol> m_Protocol;
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
        auto result = co_await session->InitializeAsync(capabilities, clientInfo, transport,
protocol); if (result.HasError()) { co_return; // Handle error
        }

        // Send initialized notification
        co_await session->SendInitializedNotificationAsync();

        // Now we can do normal operations
        auto pingTime = co_await session->PingAsync();

        // Graceful shutdown
        co_await session->ShutdownAsync();

    } catch (...) {
        // Handle exceptions
        session->ForceDisconnect();
    }
}
*/

MCP_NAMESPACE_END
