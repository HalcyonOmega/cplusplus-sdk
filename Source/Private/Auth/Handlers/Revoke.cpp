#include "Auth/Handlers/Revoke.h"

MCP_NAMESPACE_BEGIN

// TODO: Fix External Ref: OAuthServerProvider
// TODO: Fix External Ref: HttpRouter (Express equivalent)
// TODO: Fix External Ref: HttpRequest/HttpResponse
// TODO: Fix External Ref: CorsMiddleware
// TODO: Fix External Ref: RateLimitMiddleware
// TODO: Fix External Ref: ClientAuthMiddleware
// TODO: Fix External Ref: OAuthTokenRevocationRequestSchema
// TODO: Fix External Ref: HttpMethod restrictions

bool OAuthTokenRevocationRequest::validate(const JSON& request_body) {
    if (!request_body.contains("token") || !request_body["token"].is_string()) { return false; }

    if (request_body.contains("token_type_hint") && !request_body["token_type_hint"].is_string()) {
        return false;
    }

    return true;
}

OAuthTokenRevocationRequest OAuthTokenRevocationRequest::from_json(const JSON& request_body) {
    OAuthTokenRevocationRequest request;
    request.token = request_body["token"].get<string>();

    if (request_body.contains("token_type_hint")) {
        request.token_type_hint = request_body["token_type_hint"].get<string>();
    }

    return request;
}

// Main revocation handler function
RequestHandler RevocationHandler(const RevocationHandlerOptions& options) {
    if (!options.provider || !options.provider->has_revoke_token_support()) {
        throw runtime_error("Auth provider does not support revoking tokens");
    }

    return [options](shared_ptr<HttpRequest> req,
                     shared_ptr<HttpResponse> res) -> AsyncRequestHandler {
        try {
            // Set cache control header
            res->set_header("Cache-Control", "no-store");

            // Validate request method
            if (req->get_method() != MTHD_POST) { throw InvalidRequestError("Method not allowed"); }

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
            res->set_status(HTTPStatus::Ok);
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
    router->allow_methods({MTHD_POST});

    // Parse URL-encoded bodies
    router->use_url_encoded_parser(false); // extended: false

    // Apply rate limiting unless explicitly disabled
    if (options.rate_limit.has_value()) {
        RateLimitOptions rate_limit_config = options.rate_limit.value();

        // Set default rate limit message if not provided
        if (rate_limit_config.message.empty()) {
            TooManyRequestsError rate_limit_error(
                "You have exceeded the rate limit for token revocation requests");
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
