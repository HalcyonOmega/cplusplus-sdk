#pragma once

#include <concepts>
#include <coroutine>
#include <functional>
#include <memory>
#include <string>
#include <variant>

#include "Core.h"
#include "Core/Types/Initialization.h"
#include "ITransport.h"
#include "NotificationBase.h"
#include "RequestBase.h"
#include "ResponseBase.h"

MCP_NAMESPACE_BEGIN

// Coroutine task for clean async operations
template <typename T> struct MCPTask {
    struct promise_type {
        std::variant<T, std::string> m_Result;

        MCPTask get_return_object() {
            return MCPTask{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_never initial_suspend() {
            return {};
        }
        std::suspend_never final_suspend() noexcept {
            return {};
        }

        void return_value(T Value)
            requires(!std::is_void_v<T>)
        {
            m_Result = std::move(Value);
        }

        void return_void()
            requires std::is_void_v<T>
        {
            m_Result = std::monostate{};
        }

        void unhandled_exception() {
            m_Result = "Coroutine exception occurred";
        }
    };

    std::coroutine_handle<promise_type> m_Handle;

    MCPTask(std::coroutine_handle<promise_type> Handle) : m_Handle(Handle) {}

    ~MCPTask() {
        if (m_Handle) { m_Handle.destroy(); }
    }

    // Non-copyable, movable
    MCPTask(const MCPTask&) = delete;
    MCPTask& operator=(const MCPTask&) = delete;
    MCPTask(MCPTask&& Other) noexcept : m_Handle(std::exchange(Other.m_Handle, {})) {}
    MCPTask& operator=(MCPTask&& Other) noexcept {
        if (this != &Other) {
            if (m_Handle) { m_Handle.destroy(); }
            m_Handle = std::exchange(Other.m_Handle, {});
        }
        return *this;
    }

    // Awaitable interface
    bool await_ready() const {
        return m_Handle.done();
    }
    void await_suspend(std::coroutine_handle<>) const {}

    T await_resume() const
        requires(!std::is_void_v<T>)
    {
        if (std::holds_alternative<std::string>(m_Handle.promise().m_Result)) {
            throw std::runtime_error(std::get<std::string>(m_Handle.promise().m_Result));
        }
        return std::get<T>(m_Handle.promise().m_Result);
    }

    void await_resume() const
        requires std::is_void_v<T>
    {
        if (std::holds_alternative<std::string>(m_Handle.promise().m_Result)) {
            throw std::runtime_error(std::get<std::string>(m_Handle.promise().m_Result));
        }
    }
};

// Specialized void task
using MCPTask_Void = MCPTask<void>;

// Result wrapper for operations that may fail
template <typename T> struct MCPResult {
    std::variant<T, std::string> Value;

    MCPResult(T&& Value) : Value(std::forward<T>(Value)) {}
    MCPResult(const T& Value) : Value(Value) {}
    MCPResult(const std::string& Error) : Value(Error) {}
    MCPResult(std::string&& Error) : Value(std::move(Error)) {}

    bool HasValue() const {
        return std::holds_alternative<T>(Value);
    }
    bool HasError() const {
        return std::holds_alternative<std::string>(Value);
    }

    const T& GetValue() const {
        return std::get<T>(Value);
    }
    T& GetValue() {
        return std::get<T>(Value);
    }
    const std::string& GetError() const {
        return std::get<std::string>(Value);
    }
};

// Protocol configuration with sensible defaults
struct ProtocolConfig {
    string ProtocolVersion = LATEST_PROTOCOL_VERSION;
    chrono::milliseconds RequestTimeout{30000};
    chrono::milliseconds DefaultTimeout{10000};
    bool AllowBatchRequests{true};
    size_t MaxConcurrentRequests{100};
    bool EnforceStrictCapabilities{false};
};

// Protocol callbacks for message handling
struct ProtocolCallbacks {
    function<void(const string&)> OnError;
    function<void(const NotificationBase&)> OnNotification;
    function<void(const RequestBase&, function<void(const ResponseBase&)>)> OnRequest;
    function<void(const ResponseBase&)> OnResponse;
    function<void()> OnInitialized;
    function<void(const ErrorBase&)> OnProtocolError;
};

// Core Protocol Interface
class IMCP {
  public:
    virtual ~IMCP() = default;

    /* Core Protocol Interface */
    // === Lifecycle Management ===
    virtual MCPTask<InitializeResult> Initialize(const InitializeRequest& InRequest) = 0;
    virtual MCPTask_Void Initialized() = 0;
    virtual MCPTask_Void Shutdown() = 0;

    // === Connection Management ===
    virtual bool IsConnected() const = 0;
    virtual bool IsInitialized() const = 0;
    virtual string GetProtocolVersion() const = 0;

    // === Transport ===
    void SetTransport(shared_ptr<ITransport> Transport) {
        m_Transport = Transport;
    }
    shared_ptr<ITransport> GetTransport() const {
        return m_Transport;
    }

    // === Error Handling ===
    virtual void OnError(function<void(const ErrorBase&)> Callback) = 0;
    virtual void OnDisconnected(function<void()> Callback) = 0;

    // === Ping/Utility ===
    virtual MCPTask_Void Ping() = 0;

    /* Message Interface*/

    // === Senders ===

    virtual MCPTask_Void SendMessage(const MessageBase& InMessage);
    virtual MCPTask<ResponseBase> SendRequest(const RequestBase& InRequest);
    virtual MCPTask_Void SendResponse(const ResponseBase& InResponse);
    virtual MCPTask_Void SendNotification(const NotificationBase& InNotification);
    virtual MCPTask_Void SendError(const ErrorBase& InError);

    // === Handlers ===

    void SetCallbacks(const ProtocolCallbacks& InCallbacks) {
        m_Callbacks = InCallbacks;
    }
    const ProtocolCallbacks& GetCallbacks() const {
        return m_Callbacks;
    }

    void HandleIncomingMessage(const MessageBase& InMessage);
    virtual MCPTask<ResponseBase> HandleRequest(const RequestBase& InRequest) = 0;
    void HandleResponse(const ResponseBase& InResponse);
    virtual MCPTask_Void HandleNotification(const NotificationBase& InNotification) = 0;
    void RegisterPendingRequest(const string& InRequestID,
                                function<void(const ResponseBase&)> InHandler);

    /* Configuration */

    const ProtocolConfig& GetConfig() const {
        return m_Config;
    }
    void SetConfig(const ProtocolConfig& InConfig) {
        m_Config = InConfig;
    }

  private:
    ProtocolConfig m_Config;
    ProtocolCallbacks m_Callbacks;
    shared_ptr<ITransport> m_Transport;

    RequestID m_NextRequestID;
    queue<function<void(const ResponseBase&)>> m_PendingRequests;
    mutex m_PendingRequestsMutex;
};

// Content Type Concepts
template <typename T>
concept MCPContent = requires(T t) {
    { t.GetType() } -> convertible_to<string>;
    { t.GetData() } -> convertible_to<string>;
};

template <typename T>
concept MCPHandler = requires(T t) { is_invocable_v<T>; };

// Practical MCP Implementation Helper Class
// This class provides implementation-specific functionality not defined in the MCP spec
// but commonly needed for real-world usage
class PracticalMCP {
  public:
    virtual ~PracticalMCP() = default;

    // Connection Management (implementation details)
    virtual void SetConnectionTimeout(chrono::milliseconds timeout) = 0;
    virtual chrono::milliseconds GetConnectionTimeout() const = 0;
    virtual void SetRetryPolicy(int maxRetries, chrono::milliseconds retryDelay) = 0;

    // Session State Management (practical concerns)
    virtual void SaveState(const string& statePath) = 0;
    virtual bool LoadState(const string& statePath) = 0;
    virtual void ClearState() = 0;

    // Performance & Monitoring
    virtual void EnableMetrics(bool enable) = 0;
    virtual unordered_map<string, double> GetMetrics() const = 0;
    virtual void SetMaxConcurrentRequests(size_t maxRequests) = 0;
    virtual size_t GetActiveRequestCount() const = 0;

    // Security & Validation
    virtual void SetRequestValidator(function<bool(const RequestBase&)> validator) = 0;
    virtual void SetResponseValidator(function<bool(const ResponseBase&)> validator) = 0;
    virtual void EnableRequestLogging(bool enable, const string& logPath = "") = 0;

    // Advanced Transport Configuration
    virtual void SetCustomHeaders(const unordered_map<string, string>& headers) = 0;
    virtual void SetCompressionEnabled(bool enable) = 0;
    virtual void SetKeepAliveSettings(bool enable, chrono::seconds interval) = 0;

    // Event System (for advanced monitoring)
    virtual void OnRequestSent(function<void(const RequestBase&)> callback) = 0;
    virtual void OnResponseReceived(function<void(const ResponseBase&)> callback) = 0;
    virtual void OnNotificationReceived(function<void(const NotificationBase&)> callback) = 0;
    virtual void OnConnectionStateChanged(function<void(bool connected)> callback) = 0;

    // Batching Support (JSON-RPC batch operations)
    virtual void EnableBatching(bool enable, size_t maxBatchSize = 10) = 0;
    virtual void FlushBatch() = 0;

    // Resource Management
    virtual void SetResourceCacheSize(size_t maxCacheSize) = 0;
    virtual void ClearResourceCache() = 0;

    // Development/Debug Features
    virtual void SetDebugMode(bool enable) = 0;
    virtual string DumpInternalState() const = 0;
    virtual void InjectTestFailure(const string& methodName, const string& errorType) = 0;
};

// Utility Functions
namespace Utilities {
// Protocol version comparison
int CompareProtocolVersions(const string& version1, const string& version2);
bool IsVersionCompatible(const string& clientVersion, const string& serverVersion);
} // namespace Utilities

MCP_NAMESPACE_END