#pragma once

#include <chrono>
#include <coroutine>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Core.h"

MCP_NAMESPACE_BEGIN

// TODO: Fix External Ref: Express RequestHandler equivalent
// TODO: Fix External Ref: RateLimit functionality

// Rate limit options structure
struct RateLimitOptions {
    chrono::milliseconds WindowMs = chrono::minutes(15);
    int Max = 100;
    bool StandardHeaders = true;
    bool LegacyHeaders = false;
    JSON Message;
};

// Authorization handler options
struct AuthorizationHandlerOptions {
    shared_ptr<OAuthServerProvider> Provider;
    optional<RateLimitOptions> RateLimit; // nullopt means disabled
};

// Client authorization parameters validation
struct ClientAuthorizationParams {
    string ClientId;
    optional<string> RedirectUri;

    static bool Validate(const map<string, string>& Params, ClientAuthorizationParams& Output,
                         string& ErrorMessage);
};

// Request authorization parameters validation
struct RequestAuthorizationParams {
    string ResponseType;
    string CodeChallenge;
    string CodeChallengeMethod;
    optional<string> Scope;
    optional<string> State;

    static bool Validate(const map<string, string>& Params, RequestAuthorizationParams& Output,
                         string& ErrorMessage);
};

// Client information structure
struct OAuthClient {
    string ClientId;
    vector<string> RedirectUris;
    optional<string> Scope;
};

// Authorization request structure
struct AuthorizationRequest {
    optional<string> State;
    vector<string> Scopes;
    string RedirectUri;
    string CodeChallenge;
};

// Coroutine return type for async operations
template <typename T> struct Task {
    struct promise_type {
        T Value;
        exception_ptr Exception = nullptr;

        Task<T> get_return_object() {
            return Task<T>{coroutine_handle<promise_type>::from_promise(*this)};
        }

        suspend_never initial_suspend() {
            return {};
        }
        suspend_never final_suspend() noexcept {
            return {};
        }

        void return_value(T Value) {
            this->Value = move(Value);
        }

        void unhandled_exception() {
            Exception = current_exception();
        }
    };

    coroutine_handle<promise_type> Handle;

    Task(coroutine_handle<promise_type> Handle) : Handle(Handle) {}

    ~Task() {
        if (Handle) { Handle.destroy(); }
    }

    T Get() {
        if (Handle.promise().Exception) { rethrow_exception(Handle.promise().Exception); }
        return Handle.promise().Value;
    }
};

// Rate limiter class
class RateLimiter {
  private:
    RateLimitOptions Options;
    map<string, pair<chrono::steady_clock::time_point, int>> ClientRequests;

  public:
    RateLimiter(const RateLimitOptions& Options) : Options(Options) {}

    bool CheckRateLimit(const string& ClientIp);
};

// Authorization handler class
class AuthorizationHandler {
  private:
    shared_ptr<OAuthServerProvider> Provider;
    unique_ptr<RateLimiter> Limiter;

    string CreateErrorRedirect(const string& RedirectUri, const OAuthError& Error,
                               const optional<string>& State);

    vector<string> SplitString(const string& Str, char Delimiter);

  public:
    AuthorizationHandler(const AuthorizationHandlerOptions& Options) : Provider(Options.Provider) {
        if (Options.RateLimit.has_value()) {
            Limiter = make_unique<RateLimiter>(Options.RateLimit.value());
        }
    }

    Task<void> HandleRequest(const HTTP_Request& Request, HTTP_Response& Response,
                             const string& ClientIp);
};

// Factory function to create authorization handler
unique_ptr<AuthorizationHandler>
CreateAuthorizationHandler(const AuthorizationHandlerOptions& Options);

MCP_NAMESPACE_END