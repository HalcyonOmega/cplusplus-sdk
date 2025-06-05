#pragma once

#include <coroutine>

#include "Auth/Types/Auth.h"
#include "Auth/Types/AuthErrors.h"
#include "Core.h"

MCP_NAMESPACE_BEGIN

// Modern C++20 callback types
using OnClientRetrievedCallback = function<void(shared_ptr<OAuthClientInformationFull>)>;
using OnMiddlewareCompleteCallback = function<void(bool)>;
using NextFunction = function<void()>;

// HTTP Request/Response abstraction for middleware pattern
struct HTTPRequest {
    JSON Body;
    shared_ptr<OAuthClientInformationFull> Client = nullptr;
};

struct HTTPResponse {
    int StatusCode = 200;
    JSON Body;

    void SetStatus(int Status);
    void SetJSON(const JSON& Data);
};

// Modern C++20 middleware function type
using RequestHandler = function<future<void>(HTTPRequest&, HTTPResponse&, NextFunction)>;

struct ClientAuthenticationMiddlewareOptions {
    /**
     * A store used to read information about registered OAuth clients.
     */
    shared_ptr<OAuthRegisteredClientsStore> ClientsStore;
};

// Zod-like validation for ClientAuthenticatedRequest
struct ClientAuthenticatedRequestValidation {
    string ClientID;
    optional<string> ClientSecret;

    static optional<ClientAuthenticatedRequestValidation> Validate(const JSON& Body);
};

// Main middleware function (matches original TypeScript authenticateClient)
RequestHandler AuthenticateClient(const ClientAuthenticationMiddlewareOptions& Options);

struct AuthenticationTask {
    struct promise_type {
        bool Result = false;

        AuthenticationTask get_return_object() {
            return AuthenticationTask{coroutine_handle<promise_type>::from_promise(*this)};
        }

        suspend_never initial_suspend() {
            return {};
        }
        suspend_never final_suspend() noexcept {
            return {};
        }

        void return_value(bool Value) {
            Result = Value;
        }
        void unhandled_exception() {
            Result = false;
        }
    };

    coroutine_handle<promise_type> Handle;

    explicit AuthenticationTask(coroutine_handle<promise_type> H) : Handle(H) {}
    ~AuthenticationTask() {
        if (Handle) Handle.destroy();
    }

    bool GetResult() const {
        return Handle.promise().Result;
    }
    bool IsReady() const {
        return Handle.done();
    }
};

AuthenticationTask AuthenticateClientCoroutine(const ClientAuthenticationMiddlewareOptions& Options,
                                               HTTPRequest& Request, HTTPResponse& Response);

MCP_NAMESPACE_END