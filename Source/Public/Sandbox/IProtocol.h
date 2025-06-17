#pragma once

#include <chrono>
#include <coroutine>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

#include "Core.h"
#include "Core/Messages/ErrorBase.h"
#include "Core/Messages/MessageBase.h"
#include "Core/Messages/NotificationBase.h"
#include "Core/Messages/RequestBase.h"
#include "Core/Messages/ResponseBase.h"

MCP_NAMESPACE_BEGIN

// Forward declaration
class ITransport;

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
    function<void(const ErrorBase&)> OnProtocolError;
};

// Protocol Base Class - handles MCP specification compliance and typed API
class ProtocolBase {
  public:
    ProtocolBase(const ProtocolConfig& InConfig = {});
    virtual ~ProtocolBase() = default;

    // === Configuration (shared implementation) ===

    const ProtocolConfig& GetConfig() const {
        return m_Config;
    }
    void SetConfig(const ProtocolConfig& InConfig) {
        m_Config = InConfig;
    }

    // === Transport Integration (shared implementation) ===

    void SetTransport(shared_ptr<ITransport> InTransport) {
        m_Transport = InTransport;
    }
    shared_ptr<ITransport> GetTransport() const {
        return m_Transport;
    }

    // === Event Handling (shared implementation) ===

    void SetCallbacks(const ProtocolCallbacks& InCallbacks) {
        m_Callbacks = InCallbacks;
    }

    // === Typed Message API (default implementations) ===

    // Send any typed message
    virtual MCPTask_Void SendMessage(const MessageBase& InMessage);

    // Send typed request and wait for typed response
    virtual MCPTask<ResponseBase> SendRequest(const RequestBase& InRequest);

    // Send typed notification (fire and forget)
    virtual MCPTask_Void SendNotification(const NotificationBase& InNotification);

    // Send typed response
    virtual MCPTask_Void SendResponse(const ResponseBase& InResponse);

    // Send typed error
    virtual MCPTask_Void SendError(const ErrorBase& InError);

    // === Message Processing ===

    // Process incoming typed message from transport
    void HandleIncomingMessage(const MessageBase& InMessage);

    // === Utility Operations ===

    // Send ping and get response time
    virtual MCPTask<chrono::milliseconds> Ping();

    // === Request/Response Tracking ===

    // Generate unique request ID
    RequestID GenerateRequestID();

    // Register pending request
    void RegisterPendingRequest(const string& InRequestID,
                                function<void(const ResponseBase&)> InHandler);

    // Handle incoming response
    void HandleResponse(const ResponseBase& InResponse);

  protected:
    // === Customization Points (pure virtual) ===

    // Handle specific request types - override in derived classes
    virtual MCPTask<ResponseBase> HandleRequest(const RequestBase& InRequest) = 0;

    // Handle specific notification types - override in derived classes
    virtual MCPTask_Void HandleNotification(const NotificationBase& InNotification) = 0;

    // === Shared State ===

    ProtocolConfig m_Config;
    shared_ptr<ITransport> m_Transport;
    ProtocolCallbacks m_Callbacks;

    // Request tracking
    RequestID m_NextRequestID;
    // TODO: @HalcyonOmega Should this be a queue?
    unordered_map<string, function<void(const ResponseBase&)>> m_PendingRequests;
    mutex m_PendingRequestsMutex;

    // === Helper Methods ===

    void OnError(const string& InError);
    void OnProtocolError(const ErrorBase& InError);
};

MCP_NAMESPACE_END
