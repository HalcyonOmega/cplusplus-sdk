#pragma once

#include <coroutine>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <variant>

#include "Core.h"

MCP_NAMESPACE_BEGIN

// Forward declarations
class MessageBase;
class ErrorBase;

// Coroutine task for clean async operations (C++20 built-in, no third-party needed)
template <typename T> struct MCP_Task {
    struct promise_type {
        std::variant<T, std::string> m_Result;

        MCP_Task get_return_object() {
            return MCP_Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_never initial_suspend() {
            return {};
        }
        std::suspend_never final_suspend() noexcept {
            return {};
        }

        void return_value(T value)
            requires(!std::is_void_v<T>)
        {
            m_Result = std::move(value);
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

    MCP_Task(std::coroutine_handle<promise_type> handle) : m_Handle(handle) {}

    ~MCP_Task() {
        if (m_Handle) { m_Handle.destroy(); }
    }

    // Non-copyable, movable
    MCP_Task(const MCP_Task&) = delete;
    MCP_Task& operator=(const MCP_Task&) = delete;
    MCP_Task(MCP_Task&& other) noexcept : m_Handle(std::exchange(other.m_Handle, {})) {}
    MCP_Task& operator=(MCP_Task&& other) noexcept {
        if (this != &other) {
            if (m_Handle) { m_Handle.destroy(); }
            m_Handle = std::exchange(other.m_Handle, {});
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
using MCP_VoidTask = MCP_Task<void>;

// Enhanced Transport Interface following strict naming conventions
class ITransport {
  public:
    virtual ~ITransport() = default;

    // --- Core Transport Methods (Coroutines as default, no suffix) ---

    /**
     * @brief Starts the transport, establishing connections and beginning message processing.
     * @return Coroutine task that completes when transport is started
     */
    virtual MCP_VoidTask Start() = 0;

    /**
     * @brief Sends a JSON_RPC message over the transport.
     * @param InMessage Reference to the JSON_RPC message to send
     * @return Coroutine task that completes when message is sent
     */
    virtual MCP_VoidTask Send(const MessageBase& InMessage) = 0;

    /**
     * @brief Closes the transport connection and cleans up resources.
     * @return Coroutine task that completes when transport is closed
     */
    virtual MCP_VoidTask Close() = 0;

    /**
     * @brief Sends raw JSON string for protocol-level operations.
     * @param JSON_Message Raw JSON string to send
     * @return Coroutine task that completes when message is sent
     */
    virtual MCP_VoidTask SendRaw(const std::string& JSON_Message) = 0;

    // --- Synchronous Methods (with Sync suffix) ---

    /**
     * @brief Synchronous version of Start() - blocks until complete
     */
    virtual void StartSync() = 0;

    /**
     * @brief Synchronous version of Send() - blocks until complete
     */
    virtual void SendSync(const MessageBase& InMessage) = 0;

    /**
     * @brief Synchronous version of Close() - blocks until complete
     */
    virtual void CloseSync() = 0;

    /**
     * @brief Synchronous version of SendRaw() - blocks until complete
     */
    virtual void SendRawSync(const std::string& JSON_Message) = 0;

    // --- Connection State Management ---

    /**
     * @brief Check if transport is currently connected and ready
     */
    virtual bool IsConnected() const = 0;

    /**
     * @brief Check if transport is currently starting up
     */
    virtual bool IsStarting() const = 0;

    /**
     * @brief Get current connection state as string for debugging
     */
    virtual std::string GetConnectionState() const = 0;

    // --- Transport Metadata ---

    /**
     * @brief Get transport type (e.g., "stdio", "HTTP", "websocket")
     */
    virtual std::string GetTransportType() const = 0;

    /**
     * @brief Get optional connection information for debugging/logging
     */
    virtual std::optional<std::string> GetConnectionInfo() const = 0;

  protected:
    // --- Callback Types (following naming conventions) ---
    using OnCloseDelegate = std::function<void()>;
    using OnErrorDelegate = std::function<void(const ErrorBase& InError)>;
    using OnMessageDelegate = std::function<void(const MessageBase& InMessage)>;
    using OnRawMessageDelegate = std::function<void(const std::string& RawMessage)>;
    using OnStateChangeDelegate =
        std::function<void(const std::string& OldState, const std::string& NewState)>;

    /**
     * @brief Callback invoked when transport connection is closed
     */
    OnCloseDelegate m_OnClose;

    /**
     * @brief Callback invoked when transport error occurs
     */
    OnErrorDelegate m_OnError;

    /**
     * @brief Callback invoked when JSON_RPC message is received
     */
    OnMessageDelegate m_OnMessage;

    /**
     * @brief Callback invoked when raw JSON message is received (before parsing)
     */
    OnRawMessageDelegate m_OnRawMessage;

    /**
     * @brief Callback invoked when transport state changes
     */
    OnStateChangeDelegate m_OnStateChange;

    /**
     * @brief Default constructor for derived classes
     */
    ITransport() = default;

    /**
     * @brief Basic constructor for derived classes to initialize core callbacks
     */
    ITransport(OnCloseDelegate InOnClose, OnErrorDelegate InOnError, OnMessageDelegate InOnMessage)
        : m_OnClose(std::move(InOnClose)), m_OnError(std::move(InOnError)),
          m_OnMessage(std::move(InOnMessage)) {}

    /**
     * @brief Enhanced constructor with all callback support
     */
    ITransport(OnCloseDelegate InOnClose, OnErrorDelegate InOnError, OnMessageDelegate InOnMessage,
               OnRawMessageDelegate InOnRawMessage, OnStateChangeDelegate InOnStateChange)
        : m_OnClose(std::move(InOnClose)), m_OnError(std::move(InOnError)),
          m_OnMessage(std::move(InOnMessage)), m_OnRawMessage(std::move(InOnRawMessage)),
          m_OnStateChange(std::move(InOnStateChange)) {}

  public:
    // Disallow copy and move operations to prevent slicing
    ITransport(const ITransport&) = delete;
    ITransport& operator=(const ITransport&) = delete;
    ITransport(ITransport&&) = delete;
    ITransport& operator=(ITransport&&) = delete;
};

// Result wrapper for operations that may fail (following naming conventions)
template <typename T> struct MCP_Result {
    std::variant<T, std::string> Value;

    MCP_Result(T&& value) : Value(std::forward<T>(value)) {}
    MCP_Result(const T& value) : Value(value) {}
    MCP_Result(const std::string& error) : Value(error) {}
    MCP_Result(std::string&& error) : Value(std::move(error)) {}

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

MCP_NAMESPACE_END
