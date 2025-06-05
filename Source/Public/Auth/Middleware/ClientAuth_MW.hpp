#pragma once

#include "../Core/Common.hpp"

MCP_NAMESPACE_BEGIN

// TODO: Fix External Ref: OAuthRegisteredClientsStore
// TODO: Fix External Ref: OAuthClientInformationFull
// TODO: Fix External Ref: InvalidRequestError
// TODO: Fix External Ref: InvalidClientError
// TODO: Fix External Ref: ServerError
// TODO: Fix External Ref: OAuthError

// Forward declarations for external types
class OAuthRegisteredClientsStore;
struct OAuthClientInformationFull;
class InvalidRequestError;
class InvalidClientError;
class ServerError;
class OAuthError;

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

    void SetStatus(int Status) {
        StatusCode = Status;
    }
    void SetJSON(const JSON& Data) {
        Body = Data;
    }
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

    static optional<ClientAuthenticatedRequestValidation> Validate(const JSON& Body) {
        try {
            ClientAuthenticatedRequestValidation Result;

            // Validate client_id (required string)
            if (!Body.contains("client_id") || !Body["client_id"].is_string()) {
                return nullopt;
            }
            Result.ClientID = Body["client_id"].get<string>();

            // Validate client_secret (optional string)
            if (Body.contains("client_secret")) {
                if (!Body["client_secret"].is_string()) {
                    return nullopt;
                }
                Result.ClientSecret = Body["client_secret"].get<string>();
            }

            return Result;
        } catch (...) {
            return nullopt;
        }
    }
};

// Main middleware function (matches original TypeScript authenticateClient)
RequestHandler AuthenticateClient(const ClientAuthenticationMiddlewareOptions& Options) {
    return [Options](HTTPRequest& Request, HTTPResponse& Response,
                     NextFunction Next) -> future<void> {
        auto Promise = make_shared<promise<void>>();
        auto FutureResult = Promise->get_future();

        try {
            auto ValidationResult = ClientAuthenticatedRequestValidation::Validate(Request.Body);
            if (!ValidationResult.has_value()) {
                InvalidRequestError Error("Invalid request format");
                Response.SetStatus(400);
                Response.SetJSON(Error.ToResponseObject());
                Promise->set_value();
                return FutureResult;
            }

            const auto& ValidatedData = ValidationResult.value();
            string ClientID = ValidatedData.ClientID;
            optional<string> ClientSecret = ValidatedData.ClientSecret;

            // Modern C++20 async client retrieval with lambda callback
            OnClientRetrievedCallback OnClientRetrieved =
                [Promise, ClientSecret, &Request, &Response,
                 Next](shared_ptr<OAuthClientInformationFull> Client) {
                    try {
                        if (!Client) {
                            InvalidClientError Error("Invalid client_id");
                            Response.SetStatus(400);
                            Response.SetJSON(Error.ToResponseObject());
                            Promise->set_value();
                            return;
                        }

                        // If client has a secret, validate it
                        if (!Client->ClientSecret.empty()) {
                            // Check if client_secret is required but not provided
                            if (!ClientSecret.has_value()) {
                                InvalidClientError Error("Client secret is required");
                                Response.SetStatus(400);
                                Response.SetJSON(Error.ToResponseObject());
                                Promise->set_value();
                                return;
                            }

                            // Check if client_secret matches
                            if (Client->ClientSecret != ClientSecret.value()) {
                                InvalidClientError Error("Invalid client_secret");
                                Response.SetStatus(400);
                                Response.SetJSON(Error.ToResponseObject());
                                Promise->set_value();
                                return;
                            }

                            // Check if client_secret has expired (C++20 chrono)
                            if (Client->ClientSecretExpiresAt.has_value()
                                && Client->ClientSecretExpiresAt.value()
                                       < chrono::duration_cast<chrono::seconds>(
                                             chrono::system_clock::now().time_since_epoch())
                                             .count()) {
                                InvalidClientError Error("Client secret has expired");
                                Response.SetStatus(400);
                                Response.SetJSON(Error.ToResponseObject());
                                Promise->set_value();
                                return;
                            }
                        }

                        // Success! Set the client and call next middleware
                        Request.Client = Client;
                        if (Next) {
                            Next();
                        }
                        Promise->set_value();

                    } catch (const OAuthError& Error) {
                        // Determine status code based on error type
                        int Status = 400;
                        try {
                            const ServerError& ServerErr = dynamic_cast<const ServerError&>(Error);
                            Status = 500;
                        } catch (const bad_cast&) {
                            // Not a ServerError, keep status as 400
                        }

                        Response.SetStatus(Status);
                        Response.SetJSON(Error.ToResponseObject());
                        Promise->set_value();
                    } catch (const exception& Error) {
                        // Log unexpected error
                        cerr << "Unexpected error authenticating client: " << Error.what() << endl;

                        ServerError ServerErr("Internal Server Error");
                        Response.SetStatus(500);
                        Response.SetJSON(ServerErr.ToResponseObject());
                        Promise->set_value();
                    }
                };

            // Call the async GetClient method with callback
            Options.ClientsStore->GetClientAsync(ClientID, OnClientRetrieved);

        } catch (const exception& Error) {
            cerr << "Failed to start client authentication: " << Error.what() << endl;
            ServerError ServerErr("Internal Server Error");
            Response.SetStatus(500);
            Response.SetJSON(ServerErr.ToResponseObject());
            Promise->set_value();
        }

        return FutureResult;
    };
}

// C++20 Coroutine version (if you want to use co_await)
#if __has_include(<coroutine>)
    #include <coroutine>

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
        if (Handle)
            Handle.destroy();
    }

    bool GetResult() const {
        return Handle.promise().Result;
    }
    bool IsReady() const {
        return Handle.done();
    }
};

// Alternative coroutine implementation (optional)
AuthenticationTask AuthenticateClientCoroutine(const ClientAuthenticationMiddlewareOptions& Options,
                                               HTTPRequest& Request, HTTPResponse& Response) {
    // TODO: This would need a more complex coroutine implementation
    // For now, keeping it simple
    co_return false;
}
#endif

MCP_NAMESPACE_END