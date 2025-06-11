#pragma once

#include <chrono>
#include <concepts>
#include <coroutine>
#include <functional>
#include <future>
#include <ranges>
#include <regex>
#include <thread>

#include "Auth/Types/Auth.h"
#include "Core.h"
#include "Core/Constants/TransportConstants.h"


// TODO: Fix External Ref: express framework integration
// TODO: Fix External Ref: cors library integration

MCP_NAMESPACE_BEGIN

// C++20 concept for validatable schemas
template <typename T>
concept ValidatableSchema = requires(const JSON& body, T& out, string& error) {
    { T::Validate(body, out, error) } -> std::convertible_to<bool>;
};

// Proper C++20 coroutine task type with real async scheduling
template <typename T = void> struct Task {
    struct promise_type {
        T Value;
        exception_ptr Exception;
        std::function<void()> Continuation;
        bool Ready = false;

        Task get_return_object() {
            return Task{coroutine_handle<promise_type>::from_promise(*this)};
        }

        suspend_never initial_suspend() {
            return {};
        }
        suspend_always final_suspend() noexcept {
            return {};
        }

        void return_value(T value)
            requires(!std::same_as<T, void>)
        {
            Value = move(value);
            Ready = true;
            if (Continuation) Continuation();
        }

        void return_void()
            requires std::same_as<T, void>
        {
            Ready = true;
            if (Continuation) Continuation();
        }

        void unhandled_exception() {
            Exception = current_exception();
            Ready = true;
            if (Continuation) Continuation();
        }
    };

    coroutine_handle<promise_type> Handle;

    Task(coroutine_handle<promise_type> handle) : Handle(handle) {}

    ~Task() {
        if (Handle) Handle.destroy();
    }

    Task(Task&& other) noexcept : Handle(exchange(other.Handle, {})) {}
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (Handle) Handle.destroy();
            Handle = exchange(other.Handle, {});
        }
        return *this;
    }

    // Proper awaitable interface with real async scheduling
    bool await_ready() const {
        return Handle && Handle.promise().Ready;
    }

    void await_suspend(coroutine_handle<> continuation) const {
        if (Handle) {
            Handle.promise().Continuation = [continuation]() {
                // Schedule continuation on thread pool
                thread([continuation]() { continuation.resume(); }).detach();
            };
        }
    }

    T await_resume() const
        requires(!std::same_as<T, void>)
    {
        if (Handle.promise().Exception) { rethrow_exception(Handle.promise().Exception); }
        return Handle.promise().Value;
    }

    void await_resume() const
        requires(std::same_as<T, void>)
    {
        if (Handle.promise().Exception) { rethrow_exception(Handle.promise().Exception); }
    }

    // For blocking wait (when needed)
    T Get()
        requires(!std::same_as<T, void>)
    {
        while (!Handle.promise().Ready) { this_thread::sleep_for(chrono::milliseconds(1)); }
        return await_resume();
    }

    void Get()
        requires(std::same_as<T, void>)
    {
        while (!Handle.promise().Ready) { this_thread::sleep_for(chrono::milliseconds(1)); }
        await_resume();
    };
};

// Rate limiting implementation
struct RateLimitState {
    map<string, vector<chrono::steady_clock::time_point>> RequestTimes;
    mutex RateLimitMutex;

    bool CheckRateLimit(const string& identifier, int maxRequests, int windowMs);
};

struct TokenRequestSchema {
    string GrantType;

    static bool Validate(const JSON& Body, TokenRequestSchema& Out, string& ErrorMessage);
};

struct AuthorizationCodeGrantSchema {
    string Code;
    string CodeVerifier;
    optional<string> RedirectURI;

    static bool Validate(const JSON& Body, AuthorizationCodeGrantSchema& Out, string& ErrorMessage);
};

struct RefreshTokenGrantSchema {
    string RefreshToken;
    optional<string> Scope;

    static bool Validate(const JSON& Body, RefreshTokenGrantSchema& Out, string& ErrorMessage);
};

struct TokenHandlerOptions {
    shared_ptr<OAuthServerProvider> Provider;
    optional<RateLimitOptions> RateLimit;
};

// PKCE implementation matching pkce-challenge library
class PKCEVerifier {
  public:
    static Task<bool> VerifyChallenge(const string& codeVerifier, const string& codeChallenge);

  private:
    static bool VerifyChallengeSync(const string& codeVerifier, const string& codeChallenge);
};

class TokenHandler {
  public:
    TokenHandlerOptions Options;
    shared_ptr<RateLimitState> m_RateLimitState;

    TokenHandler(const TokenHandlerOptions& InOptions)
        : Options(InOptions), m_RateLimitState(make_shared<RateLimitState>()) {}

    // Middleware application equivalent to Express router setup
    struct MiddlewareResult {
        bool ShouldContinue = true;
        string ErrorResponse;
        HTTPStatus StatusCode = HTTPStatus::Ok;
        JSON Headers;
    };

    MiddlewareResult ApplyMiddleware(const JSON& RequestBody, const JSON& Headers,
                                     const string& Method, const string& ClientIP);

    // Client authentication middleware equivalent
    shared_ptr<Client> AuthenticateClient(const JSON& Headers, const JSON& RequestBody);

    // Main async request handler with full middleware pipeline
    Task<JSON> HandleRequestAsync(const JSON& RequestBody, const JSON& Headers,
                                  const string& Method, const string& ClientIP,
                                  string& ResponseStatus, JSON& ResponseHeaders);

  private:
    Task<JSON> HandleAuthorizationCodeGrantAsync(const JSON& RequestBody,
                                                 shared_ptr<Client> client);

    Task<JSON> HandleRefreshTokenGrantAsync(const JSON& RequestBody, shared_ptr<Client> client);

    // Fixed C++20 ranges-based string splitting
    vector<string> SplitStringWithRanges(const string& Str, const string& Delimiter);
};

// Factory function equivalent to the original tokenHandler function
shared_ptr<TokenHandler> CreateTokenHandler(const TokenHandlerOptions& Options);

MCP_NAMESPACE_END