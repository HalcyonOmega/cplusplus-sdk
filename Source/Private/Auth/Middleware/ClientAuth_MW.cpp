#include "Auth/Middleware/ClientAuth_MW.h"

MCP_NAMESPACE_BEGIN

void HTTP_Response::SetStatus(int Status) {
    StatusCode = Status;
}

void HTTP_Response::SetJSON(const JSON& Data) {
    Body = Data;
}

optional<ClientAuthenticatedRequestValidation>
ClientAuthenticatedRequestValidation::Validate(const JSON& Body) {
    try {
        ClientAuthenticatedRequestValidation Result;

        // Validate client_id (required string)
        if (!Body.contains(MSG_CLIENT_ID) || !Body[MSG_CLIENT_ID].is_string()) { return nullopt; }
        Result.ClientID = Body[MSG_CLIENT_ID].get<string>();

        // Validate client_secret (optional string)
        if (Body.contains("client_secret")) {
            if (!Body["client_secret"].is_string()) { return nullopt; }
            Result.ClientSecret = Body["client_secret"].get<string>();
        }

        return Result;
    } catch (...) { return nullopt; }
}

// Main middleware function (matches original TypeScript authenticateClient)
RequestHandler AuthenticateClient(const ClientAuthenticationMiddlewareOptions& Options) {
    return [Options](HTTP_Request& Request, HTTP_Response& Response,
                     NextFunction Next) -> future<void> {
        auto Promise = make_shared<promise<void>>();
        auto FutureResult = Promise->get_future();

        try {
            auto ValidationResult = ClientAuthenticatedRequestValidation::Validate(Request.Body);
            if (!ValidationResult.has_value()) {
                InvalidRequestError Error("Invalid request format");
                Response.SetStatus(HTTPStatus::BadRequest);
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
                            Response.SetStatus(HTTPStatus::BadRequest);
                            Response.SetJSON(Error.ToResponseObject());
                            Promise->set_value();
                            return;
                        }

                        // If client has a secret, validate it
                        if (!Client->ClientSecret.empty()) {
                            // Check if client_secret is required but not provided
                            if (!ClientSecret.has_value()) {
                                InvalidClientError Error("Client secret is required");
                                Response.SetStatus(HTTPStatus::BadRequest);
                                Response.SetJSON(Error.ToResponseObject());
                                Promise->set_value();
                                return;
                            }

                            // Check if client_secret matches
                            if (Client->ClientSecret != ClientSecret.value()) {
                                InvalidClientError Error("Invalid client_secret");
                                Response.SetStatus(HTTPStatus::BadRequest);
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
                                Response.SetStatus(HTTPStatus::BadRequest);
                                Response.SetJSON(Error.ToResponseObject());
                                Promise->set_value();
                                return;
                            }
                        }

                        // Success! Set the client and call next middleware
                        Request.Client = Client;
                        if (Next) { Next(); }
                        Promise->set_value();

                    } catch (const OAuthError& Error) {
                        // Determine status code based on error type
                        HTTPStatus Status = HTTPStatus::BadRequest;
                        try {
                            const ServerError& ServerErr = dynamic_cast<const ServerError&>(Error);
                            Status = 500;
                        } catch (const bad_cast&) {
                            // Not a ServerError, keep status as HTTPStatus::BadRequest
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

// Alternative coroutine implementation (optional)
AuthenticationTask AuthenticateClientCoroutine(const ClientAuthenticationMiddlewareOptions& Options,
                                               HTTP_Request& Request, HTTP_Response& Response) {
    // TODO: This would need a more complex coroutine implementation
    // For now, keeping it simple
    co_return false;
}

MCP_NAMESPACE_END