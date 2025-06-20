#pragma once

#include <coroutine>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

#include "Macros.h"

MCP_NAMESPACE_BEGIN

template <typename T> struct MCPTask;

// Primary template for coroutine tasks that return a value
template <typename T> struct MCPTask {
    struct promise_type {
        T m_Result;
        std::optional<std::string> m_Exception;

        [[nodiscard]] MCPTask get_return_object() {
            return MCPTask{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_never initial_suspend() {
            return {};
        }
        std::suspend_never final_suspend() noexcept {
            return {};
        }

        void return_value(T Value) {
            m_Result = std::move(Value);
        }

        void unhandled_exception() {
            m_Exception = "Coroutine exception occurred";
        }
    };

    std::coroutine_handle<promise_type> m_Handle;

    explicit MCPTask(std::coroutine_handle<promise_type> Handle) : m_Handle{Handle} {}

    ~MCPTask() {
        if (m_Handle) { m_Handle.destroy(); }
    }

    // Non-copyable, movable
    MCPTask(const MCPTask&) = delete;
    MCPTask& operator=(const MCPTask&) = delete;
    MCPTask(MCPTask&& Other) noexcept : m_Handle{std::exchange(Other.m_Handle, {})} {}
    MCPTask& operator=(MCPTask&& Other) noexcept {
        if (this != &Other) {
            if (m_Handle) { m_Handle.destroy(); }
            m_Handle = std::exchange(Other.m_Handle, {});
        }
        return *this;
    }

    // Awaitable interface
    [[nodiscard]] bool await_ready() const {
        return m_Handle.done();
    }
    void await_suspend(std::coroutine_handle<> /*unused*/) const {}

    [[nodiscard]] T await_resume() const {
        if (std::holds_alternative<std::string>(m_Handle.promise().m_Result)) {
            throw std::runtime_error(std::get<std::string>(m_Handle.promise().m_Result));
        }
        return std::get<T>(m_Handle.promise().m_Result);
    }
};

// Specialization for void return type
template <> struct MCPTask<void> {
    struct promise_type {
        std::optional<std::string> m_Exception;

        [[nodiscard]] MCPTask<void> get_return_object() {
            return MCPTask<void>{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_never initial_suspend() {
            return {};
        }
        std::suspend_never final_suspend() noexcept {
            return {};
        }

        void return_void() {}

        void unhandled_exception() {
            m_Exception = "Coroutine exception occurred";
        }
    };

    std::coroutine_handle<promise_type> m_Handle;

    explicit MCPTask(std::coroutine_handle<promise_type> Handle) : m_Handle{Handle} {}

    ~MCPTask() {
        if (m_Handle) { m_Handle.destroy(); }
    }

    // Non-copyable, movable
    MCPTask(const MCPTask&) = delete;
    MCPTask& operator=(const MCPTask&) = delete;
    MCPTask(MCPTask&& Other) noexcept : m_Handle{std::exchange(Other.m_Handle, {})} {}
    MCPTask& operator=(MCPTask&& Other) noexcept {
        if (this != &Other) {
            if (m_Handle) { m_Handle.destroy(); }
            m_Handle = std::exchange(Other.m_Handle, {});
        }
        return *this;
    }

    // Awaitable interface
    [[nodiscard]] bool await_ready() const {
        return m_Handle.done();
    }
    void await_suspend(std::coroutine_handle<> /*unused*/) const {}

    void await_resume() const {
        if (m_Handle.promise().m_Exception) {
            throw std::runtime_error(*m_Handle.promise().m_Exception);
        }
    }
};

// Specialized void task
using MCPTask_Void = MCPTask<void>;

MCP_NAMESPACE_END