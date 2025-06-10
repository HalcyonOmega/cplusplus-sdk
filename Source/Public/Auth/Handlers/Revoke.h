#pragma once

#include <chrono>
#include <coroutine>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "Constants.h"
#include "Core.h"

MCP_NAMESPACE_BEGIN

// TODO: Fix External Ref: OAuthServerProvider
// TODO: Fix External Ref: CorsMiddleware
// TODO: Fix External Ref: RateLimitMiddleware
// TODO: Fix External Ref: ClientAuthMiddleware
// TODO: Fix External Ref: OAuthTokenRevocationRequestSchema

// Configuration for the revocation handler
struct RevocationHandlerOptions {
    shared_ptr<OAuthServerProvider> Provider;
    // Rate limiting configuration for the token revocation endpoint.
    // Set to nullopt to disable rate limiting for this endpoint.
    optional<RateLimitOptions> RateLimit;
};

// OAuth Token Revocation Request validation
struct OAuthTokenRevocationRequest {
    string Token;
    optional<string> TokenTypeHint;

    static bool Validate(const JSON& InRequestBody);
    static OAuthTokenRevocationRequest FromJSON(const JSON& InRequestBody);
};

// Async request handler coroutine
struct AsyncRequestHandler {
    struct promise_type {
        AsyncRequestHandler get_return_object() {
            return AsyncRequestHandler{handle_type::from_promise(*this)};
        }

        suspend_never initial_suspend() {
            return {};
        }
        suspend_never final_suspend() noexcept {
            return {};
        }

        void return_void() {}
        void unhandled_exception() {
            exception_ptr = current_exception();
        }

        exception_ptr exception_ptr;
    };

    using handle_type = coroutine_handle<promise_type>;
    handle_type coro;

    AsyncRequestHandler(handle_type h) : coro(h) {}
    ~AsyncRequestHandler() {
        if (coro) { coro.destroy(); }
    }

    AsyncRequestHandler(const AsyncRequestHandler&) = delete;
    AsyncRequestHandler& operator=(const AsyncRequestHandler&) = delete;

    AsyncRequestHandler(AsyncRequestHandler&& other) noexcept : coro(other.coro) {
        other.coro = {};
    }

    AsyncRequestHandler& operator=(AsyncRequestHandler&& other) noexcept {
        if (this != &other) {
            if (coro) { coro.destroy(); }
            coro = other.coro;
            other.coro = {};
        }
        return *this;
    }
};

// Main revocation handler function
RequestHandler RevocationHandler(const RevocationHandlerOptions& InOptions);

// Factory function to create a configured router with middleware
shared_ptr<HTTP_Router> CreateRevocationRouter(const RevocationHandlerOptions& InOptions);

MCP_NAMESPACE_END