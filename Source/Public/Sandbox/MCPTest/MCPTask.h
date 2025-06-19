#pragma once

#include <concepts>
#include <coroutine>
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

// Template coroutine type for MCP SDK
template <typename T = void> class MCPTask {
  public:
    struct promise_type {
        T m_Value{};
        std::exception_ptr m_Exception{};

        MCPTask get_return_object() {
            return MCPTask{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_never initial_suspend() {
            return {};
        }
        std::suspend_always final_suspend() noexcept {
            return {};
        }

        void unhandled_exception() {
            m_Exception = std::current_exception();
        }

        template <typename U>
        void return_value(U&& InValue)
            requires std::convertible_to<U, T>
        {
            m_Value = std::forward<U>(InValue);
        }

        void return_void()
            requires std::same_as<T, void>
        {}
    };

    using HandleType = std::coroutine_handle<promise_type>;

    MCPTask(HandleType InHandle) : m_Handle(InHandle) {}

    MCPTask(const MCPTask&) = delete;
    MCPTask& operator=(const MCPTask&) = delete;

    MCPTask(MCPTask&& InOther) noexcept : m_Handle(std::exchange(InOther.m_Handle, {})) {}
    MCPTask& operator=(MCPTask&& InOther) noexcept {
        if (this != &InOther) {
            if (m_Handle) m_Handle.destroy();
            m_Handle = std::exchange(InOther.m_Handle, {});
        }
        return *this;
    }

    ~MCPTask() {
        if (m_Handle) m_Handle.destroy();
    }

    bool IsReady() const {
        return m_Handle && m_Handle.done();
    }

    T GetResult() {
        if (!m_Handle) throw std::runtime_error("Invalid task handle");

        if (m_Handle.promise().m_Exception) std::rethrow_exception(m_Handle.promise().m_Exception);

        if constexpr (!std::same_as<T, void>) { return std::move(m_Handle.promise().m_Value); }
    }

    auto operator co_await() {
        struct Awaiter {
            HandleType m_Handle;

            bool await_ready() {
                return m_Handle.done();
            }
            void await_suspend(std::coroutine_handle<>) {}
            T await_resume() {
                if (m_Handle.promise().m_Exception)
                    std::rethrow_exception(m_Handle.promise().m_Exception);

                if constexpr (!std::same_as<T, void>) {
                    return std::move(m_Handle.promise().m_Value);
                }
            }
        };

        return Awaiter{m_Handle};
    }

  private:
    HandleType m_Handle;
};

// Specialization for void
template <> class MCPTask<void> {
  public:
    struct promise_type {
        std::exception_ptr m_Exception{};

        MCPTask get_return_object() {
            return MCPTask{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_never initial_suspend() {
            return {};
        }
        std::suspend_always final_suspend() noexcept {
            return {};
        }

        void unhandled_exception() {
            m_Exception = std::current_exception();
        }

        void return_void() {}
    };

    using HandleType = std::coroutine_handle<promise_type>;

    MCPTask(HandleType InHandle) : m_Handle(InHandle) {}

    MCPTask(const MCPTask&) = delete;
    MCPTask& operator=(const MCPTask&) = delete;

    MCPTask(MCPTask&& InOther) noexcept : m_Handle(std::exchange(InOther.m_Handle, {})) {}
    MCPTask& operator=(MCPTask&& InOther) noexcept {
        if (this != &InOther) {
            if (m_Handle) m_Handle.destroy();
            m_Handle = std::exchange(InOther.m_Handle, {});
        }
        return *this;
    }

    ~MCPTask() {
        if (m_Handle) m_Handle.destroy();
    }

    bool IsReady() const {
        return m_Handle && m_Handle.done();
    }

    void GetResult() {
        if (!m_Handle) throw std::runtime_error("Invalid task handle");

        if (m_Handle.promise().m_Exception) std::rethrow_exception(m_Handle.promise().m_Exception);
    }

    auto operator co_await() {
        struct Awaiter {
            HandleType m_Handle;

            bool await_ready() {
                return m_Handle.done();
            }
            void await_suspend(std::coroutine_handle<>) {}
            void await_resume() {
                if (m_Handle.promise().m_Exception)
                    std::rethrow_exception(m_Handle.promise().m_Exception);
            }
        };

        return Awaiter{m_Handle};
    }

  private:
    HandleType m_Handle;
};

using MCPTaskVoid = MCPTask<void>;