#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "Core.h"
#include "Core/Types/Capabilities.h"
#include "Core/Types/Implementation.h"
#include "ITransport.h"
#include "MessageBase.h"
#include "NotificationBase.h"
#include "RequestBase.h"

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

// Pure Session Interface - handles connection, messaging, and state management
class ISession {
  public:
    virtual ~ISession() = default;

    // === Lifecycle Management ===

    // Connect with transport and start session
    virtual MCPTask_Void Connect(shared_ptr<ITransport> InTransport) = 0;

    // Graceful shutdown
    virtual MCPTask_Void Shutdown() = 0;

    // Force disconnect (for error recovery)
    virtual void ForceDisconnect() = 0;

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

    // === Message Handling ===

    // Send any message
    virtual MCPTask_Void SendMessage(const MessageBase& InMessage) = 0;

    // Send request and wait for response
    virtual MCPTask<JSON> SendRequest(const RequestBase& InRequest) = 0;

    // Send notification (fire and forget)
    virtual MCPTask_Void SendNotification(const NotificationBase& InNotification) = 0;

    // === Event Handling ===

    virtual void SetCallbacks(const SessionCallbacks& InCallbacks) = 0;

    // === Utility Operations ===

    // Send ping and get response time
    virtual MCPTask<chrono::milliseconds> Ping() = 0;
};

// Session Factory - creates sessions with different configurations
class SessionFactory {
  public:
    // Create session with configuration
    static unique_ptr<ISession> Create(const SessionConfig& InConfig = {});

    // Create session with custom transport
    static unique_ptr<ISession> CreateWithTransport(shared_ptr<ITransport> InTransport,
                                                    const SessionConfig& InConfig = {});
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

MCP_NAMESPACE_END
