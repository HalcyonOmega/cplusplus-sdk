#pragma once

#include <coroutine>
#include <exception>
#include <optional>
#include <utility>

#include "CoreSDK/Common/Macros.h"

MCP_NAMESPACE_BEGIN

template <typename T> struct MCPTask;

// Primary template for coroutine tasks that return a value
template <typename T> struct MCPTask {
    struct promise_type {
        std::optional<T> m_Result;
        std::exception_ptr m_Exception;
        std::coroutine_handle<> m_Awaiter;

        [[nodiscard]] MCPTask get_return_object() {
            return MCPTask{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() noexcept {
            return {};
        }

        std::suspend_always final_suspend() noexcept {
            // Resume awaiter when this task completes
            if (m_Awaiter) { m_Awaiter.resume(); }
            return {};
        }

        void return_value(T Value) {
            m_Result = std::move(Value);
        }

        void unhandled_exception() {
            m_Exception = std::current_exception();
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
        return m_Handle && m_Handle.done();
    }

    void await_suspend(std::coroutine_handle<> InAwaiter) {
        // Store awaiter to resume when this task completes
        m_Handle.promise().m_Awaiter = InAwaiter;
        // Resume this task if not started
        if (m_Handle && !m_Handle.done()) { m_Handle.resume(); }
    }

    [[nodiscard]] T await_resume() const {
        if (m_Handle.promise().m_Exception) {
            std::rethrow_exception(m_Handle.promise().m_Exception);
        }
        return std::move(*m_Handle.promise().m_Result);
    }
};

// Specialization for void return type
template <> struct MCPTask<void> {
    struct promise_type {
        std::exception_ptr m_Exception;
        std::coroutine_handle<> m_Awaiter;

        [[nodiscard]] MCPTask<void> get_return_object() {
            return MCPTask<void>{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        static std::suspend_always initial_suspend() noexcept {
            return {};
        }

        std::suspend_always final_suspend() const noexcept {
            // Resume awaiter when this task completes
            if (m_Awaiter) { m_Awaiter.resume(); }
            return {};
        }

        void return_void() {}

        void unhandled_exception() {
            m_Exception = std::current_exception();
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
        return m_Handle && m_Handle.done();
    }

    void await_suspend(std::coroutine_handle<> InAwaiter) const {
        // Store awaiter to resume when this task completes
        m_Handle.promise().m_Awaiter = InAwaiter;
        // Resume this task if not started
        if (m_Handle && !m_Handle.done()) { m_Handle.resume(); }
    }

    void await_resume() const {
        if (m_Handle.promise().m_Exception) {
            std::rethrow_exception(m_Handle.promise().m_Exception);
        }
    }
};

// Specialized void task
using MCPTask_Void = MCPTask<void>;

MCP_NAMESPACE_END