#pragma once

#include "Core.h"

// TODO: Fix External Ref: express (HTTP server framework)
// TODO: Fix External Ref: cors (CORS middleware)
// TODO: Fix External Ref: express-rate-limit (Rate limiting middleware)
// TODO: Fix External Ref: OAuthClientInformationFull
// TODO: Fix External Ref: OAuthClientMetadataSchema
// TODO: Fix External Ref: OAuthRegisteredClientsStore
// TODO: Fix External Ref: allowedMethods middleware

MCP_NAMESPACE_BEGIN

// Client registration handler options
struct ClientRegistrationHandlerOptions {
    /**
     * A store used to save information about dynamically registered OAuth clients.
     */
    shared_ptr<OAuthRegisteredClientsStore> ClientsStore;

    /**
     * The number of seconds after which to expire issued client secrets, or 0 to prevent expiration
     * of client secrets (not recommended).
     *
     * If not set, defaults to 30 days.
     */
    optional<int> ClientSecretExpirySeconds;

    /**
     * Rate limiting configuration for the client registration endpoint.
     * Set to false to disable rate limiting for this endpoint.
     * Registration endpoints are particularly sensitive to abuse and should be rate limited.
     */
    optional<RateLimitOptions> RateLimit;
    bool RateLimitDisabled = false;
};

// Constants
constexpr int DEFAULT_CLIENT_SECRET_EXPIRY_SECONDS = 30 * 24 * 60 * 60; // 30 days

// OAuth client metadata validation
class OAuthClientMetadata {
  public:
    string TokenEndpointAuthMethod;
    // Add other OAuth metadata fields as needed

    static expected<OAuthClientMetadata, string> SafeParse(const JSON& json) {
        try {
            OAuthClientMetadata metadata;
            if (json.contains("token_endpoint_auth_method")) {
                metadata.TokenEndpointAuthMethod = json["token_endpoint_auth_method"];
            }
            return metadata;
        } catch (const exception& e) { return unexpected(e.what()); }
    }
};

// OAuth client information structure
struct OAuthClientInformation {
    string ClientId;
    optional<string> ClientSecret;
    int ClientIdIssuedAt;
    optional<int> ClientSecretExpiresAt;
    string TokenEndpointAuthMethod;

    JSON ToJSON() const {
        JSON result;
        result[MSG_CLIENT_ID] = ClientId;
        if (ClientSecret.has_value()) { result["client_secret"] = *ClientSecret; }
        result["client_id_issued_at"] = ClientIdIssuedAt;
        if (ClientSecretExpiresAt.has_value()) {
            result["client_secret_expires_at"] = *ClientSecretExpiresAt;
        }
        result["token_endpoint_auth_method"] = TokenEndpointAuthMethod;
        return result;
    }
};

// OAuth registered clients store interface
class OAuthRegisteredClientsStore {
  public:
    virtual ~OAuthRegisteredClientsStore() = default;
    virtual task<OAuthClientInformation>
    RegisterClient(const OAuthClientInformation& clientInfo) = 0;
    virtual bool SupportsRegistration() const = 0;
};

// Utility functions
string GenerateUUID() {
    // Simple UUID v4 generation using random numbers
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, 15);
    uniform_int_distribution<> dis2(8, 11);

    stringstream ss;
    ss << hex;
    for (int i = 0; i < 8; i++) { ss << dis(gen); }
    ss << "-";
    for (int i = 0; i < 4; i++) { ss << dis(gen); }
    ss << "-4";
    for (int i = 0; i < 3; i++) { ss << dis(gen); }
    ss << "-";
    ss << dis2(gen);
    for (int i = 0; i < 3; i++) { ss << dis(gen); }
    ss << "-";
    for (int i = 0; i < 12; i++) { ss << dis(gen); }
    return ss.str();
}

string GenerateRandomBytes(int length) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, 255);

    stringstream ss;
    ss << hex;
    for (int i = 0; i < length; i++) { ss << setw(2) << setfill('0') << dis(gen); }
    return ss.str();
}

int GetCurrentTimestamp() {
    return static_cast<int>(
        chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch())
            .count());
}

// Main client registration handler function
RequestHandler ClientRegistrationHandler(const ClientRegistrationHandlerOptions& options) {
    if (!options.ClientsStore || !options.ClientsStore->SupportsRegistration()) {
        throw runtime_error("Client registration store does not support registering clients");
    }

    auto clientsStore = options.ClientsStore;
    int clientSecretExpirySeconds =
        options.ClientSecretExpirySeconds.value_or(DEFAULT_CLIENT_SECRET_EXPIRY_SECONDS);
    auto rateLimitConfig = options.RateLimit;
    bool rateLimitDisabled = options.RateLimitDisabled;

    return [=](const HTTP_Request& req, HTTP_Response& res) -> task<void> {
        // Set cache control header
        res.SetHeader("Cache-Control", "no-store");

        try {
            // Validate HTTP method
            if (req.Method != MTHD_POST) { throw runtime_error("Method not allowed"); }

            // Apply rate limiting logic (simplified - actual implementation would need middleware)
            if (!rateLimitDisabled && rateLimitConfig.has_value()) {
                // TODO: Implement actual rate limiting logic
                // This would typically be handled by middleware in a real HTTP framework
            }

            // Parse and validate client metadata
            auto parseResult = OAuthClientMetadata::SafeParse(req.Body);
            if (!parseResult.has_value()) { throw InvalidClientMetadataError(parseResult.error()); }

            auto clientMetadata = parseResult.value();
            bool isPublicClient = clientMetadata.TokenEndpointAuthMethod == "none";

            // Generate client credentials
            string clientId = GenerateUUID();
            optional<string> clientSecret =
                isPublicClient ? nullopt : optional<string>(GenerateRandomBytes(32));
            int clientIdIssuedAt = GetCurrentTimestamp();

            // Calculate client secret expiry time
            bool clientsDoExpire = clientSecretExpirySeconds > 0;
            int secretExpiryTime =
                clientsDoExpire ? clientIdIssuedAt + clientSecretExpirySeconds : 0;
            optional<int> clientSecretExpiresAt =
                isPublicClient ? nullopt : optional<int>(secretExpiryTime);

            // Create client information
            OAuthClientInformation clientInfo;
            clientInfo.ClientId = clientId;
            clientInfo.ClientSecret = clientSecret;
            clientInfo.ClientIdIssuedAt = clientIdIssuedAt;
            clientInfo.ClientSecretExpiresAt = clientSecretExpiresAt;
            clientInfo.TokenEndpointAuthMethod = clientMetadata.TokenEndpointAuthMethod;

            // Register client with store
            auto registeredClient = co_await clientsStore->RegisterClient(clientInfo);

            // Send response
            res.Status(201);
            res.SendJSON(registeredClient.ToJSON());

        } catch (const OAuthError& error) {
            auto serverError = dynamic_cast<const ServerError*>(&error);
            int status = serverError ? 500 : HTTPStatus::BadRequest;
            res.Status(status);
            res.SendJSON(error.ToResponseObject());
        } catch (const exception& error) {
            // Log unexpected error (in real implementation)
            // console.error("Unexpected error registering client:", error);
            ServerError serverError("Internal Server Error");
            res.Status(500);
            res.SendJSON(serverError.ToResponseObject());
        }
    };
}

MCP_NAMESPACE_END