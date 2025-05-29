#pragma once

#include "../Core/Common.hpp"

namespace MCP::Auth {

// TODO: Fix External Ref: OAuthServerProvider
// TODO: Fix External Ref: HttpRouter (Express equivalent)
// TODO: Fix External Ref: HttpRequest/HttpResponse
// TODO: Fix External Ref: CorsMiddleware
// TODO: Fix External Ref: RateLimitMiddleware
// TODO: Fix External Ref: ClientAuthMiddleware
// TODO: Fix External Ref: OAuthTokenRevocationRequestSchema
// TODO: Fix External Ref: HttpMethod restrictions

#include <coroutine>
#include <memory>
#include <optional>
#include <functional>
#include <chrono>
#include <string>
#include <unordered_map>

// Forward declarations for external dependencies
class OAuthServerProvider;
class HttpRequest;
class HttpResponse;
class HttpRouter;
struct ClientInfo;

// Rate limit configuration options
struct RateLimitOptions {
    chrono::milliseconds window_ms = chrono::minutes(15);
    int max_requests = 50;
    bool standard_headers = true;
    bool legacy_headers = false;
    string message;
};

// Configuration for the revocation handler
struct RevocationHandlerOptions {
    shared_ptr<OAuthServerProvider> provider;
    // Rate limiting configuration for the token revocation endpoint.
    // Set to nullopt to disable rate limiting for this endpoint.
    optional<RateLimitOptions> rate_limit;
};

// OAuth Error classes
class OAuthError : public exception {
public:
    virtual ~OAuthError() = default;
    virtual JSON to_response_object() const = 0;
    virtual const char* what() const noexcept override = 0;
};

class InvalidRequestError : public OAuthError {
private:
    string message_;

public:
    explicit InvalidRequestError(const string& message) : message_(message) {}

    JSON to_response_object() const override {
        return JSON{
            {"error", "invalid_request"},
            {"error_description", message_}
        };
    }

    const char* what() const noexcept override {
        return message_.c_str();
    }
};

class ServerError : public OAuthError {
private:
    string message_;

public:
    explicit ServerError(const string& message) : message_(message) {}

    JSON to_response_object() const override {
        return JSON{
            {"error", "server_error"},
            {"error_description", message_}
        };
    }

    const char* what() const noexcept override {
        return message_.c_str();
    }
};

class TooManyRequestsError : public OAuthError {
private:
    string message_;

public:
    explicit TooManyRequestsError(const string& message) : message_(message) {}

    JSON to_response_object() const override {
        return JSON{
            {"error", "too_many_requests"},
            {"error_description", message_}
        };
    }

    const char* what() const noexcept override {
        return message_.c_str();
    }
};

// OAuth Token Revocation Request validation
struct OAuthTokenRevocationRequest {
    string token;
    optional<string> token_type_hint;

    static bool validate(const JSON& request_body) {
        if (!request_body.contains("token") || !request_body["token"].is_string()) {
            return false;
        }

        if (request_body.contains("token_type_hint") &&
            !request_body["token_type_hint"].is_string()) {
            return false;
        }

        return true;
    }

    static OAuthTokenRevocationRequest from_json(const JSON& request_body) {
        OAuthTokenRevocationRequest request;
        request.token = request_body["token"].get<string>();

        if (request_body.contains("token_type_hint")) {
            request.token_type_hint = request_body["token_type_hint"].get<string>();
        }

        return request;
    }
};

// Request handler function type
using RequestHandler = function<void(shared_ptr<HttpRequest>, shared_ptr<HttpResponse>)>;

// Async request handler coroutine
struct AsyncRequestHandler {
    struct promise_type {
        AsyncRequestHandler get_return_object() {
            return AsyncRequestHandler{handle_type::from_promise(*this)};
        }

        suspend_never initial_suspend() { return {}; }
        suspend_never final_suspend() noexcept { return {}; }

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
        if (coro) {
            coro.destroy();
        }
    }

    AsyncRequestHandler(const AsyncRequestHandler&) = delete;
    AsyncRequestHandler& operator=(const AsyncRequestHandler&) = delete;

    AsyncRequestHandler(AsyncRequestHandler&& other) noexcept : coro(other.coro) {
        other.coro = {};
    }

    AsyncRequestHandler& operator=(AsyncRequestHandler&& other) noexcept {
        if (this != &other) {
            if (coro) {
                coro.destroy();
            }
            coro = other.coro;
            other.coro = {};
        }
        return *this;
    }
};

// Main revocation handler function
RequestHandler revocation_handler(const RevocationHandlerOptions& options) {
    if (!options.provider || !options.provider->has_revoke_token_support()) {
        throw runtime_error("Auth provider does not support revoking tokens");
    }

    return [options](shared_ptr<HttpRequest> req, shared_ptr<HttpResponse> res) -> AsyncRequestHandler {
        try {
            // Set cache control header
            res->set_header("Cache-Control", "no-store");

            // Validate request method
            if (req->get_method() != "POST") {
                throw InvalidRequestError("Method not allowed");
            }

            // Parse and validate request body
            JSON request_body = req->get_json_body();
            if (!OAuthTokenRevocationRequest::validate(request_body)) {
                throw InvalidRequestError("Invalid request format");
            }

            // Extract client information (should be set by authentication middleware)
            auto client = req->get_client_info();
            if (!client) {
                cerr << "Missing client information after authentication" << endl;
                throw ServerError("Internal Server Error");
            }

            // Parse the validated request
            auto revocation_request = OAuthTokenRevocationRequest::from_json(request_body);

            // Revoke the token
            co_await options.provider->revoke_token(*client, revocation_request);

            // Send successful response
            res->set_status(200);
            res->send_json(JSON{});

        } catch (const OAuthError& error) {
            int status = dynamic_cast<const ServerError*>(&error) ? 500 : 400;
            res->set_status(status);
            res->send_json(error.to_response_object());
        } catch (const exception& error) {
            cerr << "Unexpected error revoking token: " << error.what() << endl;
            ServerError server_error("Internal Server Error");
            res->set_status(500);
            res->send_json(server_error.to_response_object());
        }

        co_return;
    };
}

// Factory function to create a configured router with middleware
shared_ptr<HttpRouter> create_revocation_router(const RevocationHandlerOptions& options) {
    auto router = make_shared<HttpRouter>();

    // Configure CORS to allow any origin, to make accessible to web-based MCP clients
    router->use_cors();

    // Restrict to POST method only
    router->allow_methods({"POST"});

    // Parse URL-encoded bodies
    router->use_url_encoded_parser(false); // extended: false

    // Apply rate limiting unless explicitly disabled
    if (options.rate_limit.has_value()) {
        RateLimitOptions rate_limit_config = options.rate_limit.value();

        // Set default rate limit message if not provided
        if (rate_limit_config.message.empty()) {
            TooManyRequestsError rate_limit_error(
                "You have exceeded the rate limit for token revocation requests"
            );
            rate_limit_config.message = rate_limit_error.to_response_object().dump();
        }

        router->use_rate_limit(rate_limit_config);
    }

    // Authenticate and extract client details
    router->use_client_authentication(options.provider->get_clients_store());

    // Add the main handler
    router->post("/", revocation_handler(options));

    return router;
}

MCP_NAMESPACE_END
