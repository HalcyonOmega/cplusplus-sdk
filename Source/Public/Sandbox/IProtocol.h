#pragma once

#include <coroutine>
#include <functional>
#include <optional>
#include <string>
#include <variant>

#include "Core.h"
#include "ErrorBase.h"
#include "MessageBase.h"

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

// Result wrapper for operations that may fail (following naming conventions)
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

MCP_NAMESPACE_END
