#pragma once

#include "Auth/Types/Auth.h"
#include "Core.h"
#include "Utilities/HTTP/HTTPLayer.hpp"

// TODO: Fix External Ref: pkce-challenge (PKCE challenge generation)
// TODO: Fix External Ref: MCP_LATEST_PROTOCOL_VERSION from "../types.js"
// TODO: Fix External Ref: Zod schemas validation

MCP_NAMESPACE_BEGIN

/**
 * Implements an end-to-end OAuth client to be used with one MCP server.
 *
 * This client relies upon a concept of an authorized "session," the exact
 * meaning of which is application-defined. Tokens, authorization codes, and
 * code verifiers should not cross different sessions.
 */
class OAuthClientProvider {
  public:
    virtual ~OAuthClientProvider() = default;

    /**
     * The URL to redirect the user agent to after authorization.
     */
    virtual string GetRedirectURL() const = 0; // TODO: Return string | URL

    /**
     * Metadata about this OAuth client.
     */
    virtual OAuthClientMetadata GetClientMetadata() const = 0;

    /**
     * Returns a OAuth2 state parameter.
     */
    virtual variant<string, future<string>> GetState();

    /**
     * Loads information about this OAuth client, as registered already with the
     * server, or returns `nullopt` if the client is not registered with the
     * server.
     */
    virtual variant<optional<OAuthClientInformation>, future<optional<OAuthClientInformation>>>
    GetClientInformation() = 0;

    /**
     * If implemented, this permits the OAuth client to dynamically register with
     * the server. Client information saved this way should later be read via
     * `GetClientInformation()`.
     *
     * This method is not required to be implemented if client information is
     * statically known (e.g., pre-registered).
     */
    virtual variant<void, future<void>>
    SaveClientInformation(const OAuthClientInformationFull& ClientInformation);

    /**
     * Loads any existing OAuth tokens for the current session, or returns
     * `nullopt` if there are no saved tokens.
     */
    virtual variant<optional<OAuthTokens>, future<optional<OAuthTokens>>> GetTokens() = 0;

    /**
     * Stores new OAuth tokens for the current session, after a successful
     * authorization.
     */
    virtual variant<void, future<void>> SaveTokens(const OAuthTokens& Tokens) = 0;

    /**
     * Invoked to redirect the user agent to the given URL to begin the authorization flow.
     */
    virtual variant<void, future<void>> RedirectToAuthorization(
        const string& AuthorizationURL) = 0; // TODO: Ingest URL Type, not string

    /**
     * Saves a PKCE code verifier for the current session, before redirecting to
     * the authorization flow.
     */
    virtual variant<void, future<void>> SaveCodeVerifier(const string& CodeVerifier) = 0;

    /**
     * Loads the PKCE code verifier for the current session, necessary to validate
     * the authorization result.
     */
    virtual variant<string, future<string>> GetCodeVerifier() = 0;
};

enum class AuthResult { AUTHORIZED, REDIRECT };

class UnauthorizedError : public exception {
  public:
    explicit UnauthorizedError(const string& Message = "Unauthorized") : exception(Message) {}
};

struct AuthParams {
    string ServerURL;
    optional<string> AuthorizationCode;
    optional<string> Scope;
    optional<string> ResourceMetadataURL;
};

struct PKCE_Challenge {
    string CodeVerifier;
    string CodeChallenge;
};

struct AuthorizationResult {
    string AuthorizationURL;
    string CodeVerifier;
};

struct StartAuthorizationParams {
    optional<OAuthMetadata> Metadata;
    OAuthClientInformation ClientInformation;
    string RedirectURL;
    optional<string> Scope;
    optional<string> State;
};

struct ExchangeAuthorizationParams {
    optional<OAuthMetadata> Metadata;
    OAuthClientInformation ClientInformation;
    string AuthorizationCode;
    string CodeVerifier;
    string RedirectURI;
};

struct RefreshAuthorizationParams {
    optional<OAuthMetadata> Metadata;
    OAuthClientInformation ClientInformation;
    string RefreshToken;
};

struct RegisterClientParams {
    optional<OAuthMetadata> Metadata;
    OAuthClientMetadata ClientMetadata;
};

struct DiscoverMetadataOptions {
    optional<string> ProtocolVersion;
    optional<string> ResourceMetadataURL;
};

/**
 * Orchestrates the full auth flow with a server.
 *
 * This can be used as a single entry point for all authorization functionality,
 * instead of linking together the other lower-level functions in this module.
 */
future<AuthResult> Auth(shared_ptr<OAuthClientProvider> Provider, const AuthParams& Params);

/**
 * Extract resource_metadata from response header.
 */
optional<string> ExtractResourceMetadataURL(const unordered_map<string, string>& ResponseHeaders);

/**
 * Looks up RFC 9728 OAuth 2.0 Protected Resource Metadata.
 *
 * If the server returns a HTTPStatus::NotFound for the well-known endpoint, this function will
 * throw an exception. Any other errors will be thrown as exceptions.
 */
future<OAuthProtectedResourceMetadata> DiscoverOAuthProtectedResourceMetadataAsync(
    const string& ServerURL, const optional<DiscoverMetadataOptions>& Options = nullopt);

/**
 * Looks up RFC 8414 OAuth 2.0 Authorization Server Metadata.
 *
 * If the server returns a HTTPStatus::NotFound for the well-known endpoint, this function will
 * return `nullopt`. Any other errors will be thrown as exceptions.
 */
future<optional<OAuthMetadata>>
DiscoverOAuthMetadataAsync(const string& AuthorizationServerURL,
                           const optional<DiscoverMetadataOptions>& Options = nullopt);

/**
 * Begins the authorization flow with the given server, by generating a PKCE challenge and
 * constructing the authorization URL.
 */
future<AuthorizationResult> StartAuthorizationAsync(const string& AuthorizationServerURL,
                                                    const StartAuthorizationParams& Params);

/**
 * Exchanges an authorization code for an access token with the given server.
 */
future<OAuthTokens> ExchangeAuthorizationAsync(const string& AuthorizationServerURL,
                                               const ExchangeAuthorizationParams& Params);

/**
 * Exchange a refresh token for an updated access token.
 */
future<OAuthTokens> RefreshAuthorizationAsync(const string& AuthorizationServerURL,
                                              const RefreshAuthorizationParams& Params);

/**
 * Performs OAuth 2.0 Dynamic Client Registration according to RFC 7591.
 */
future<OAuthClientInformationFull> RegisterClientAsync(const string& AuthorizationServerURL,
                                                       const RegisterClientParams& Params);

// Helper functions and TODO implementations
PKCE_Challenge GeneratePKCE_Challenge();

future<HTTP_Response> FetchAsync(const string& URL,
                                 const unordered_map<string, string>& Headers = {});
future<HTTP_Response> FetchPostAsync(const string& URL, const string& Body,
                                     const unordered_map<string, string>& Headers = {});

// JSON schema validation functions (replacing Zod)
bool ValidateOAuthProtectedResourceMetadata(const JSON& Data);
bool ValidateOAuthMetadata(const JSON& Data);
bool ValidateOAuthTokens(const JSON& Data);
bool ValidateOAuthClientInformationFull(const JSON& Data);

OAuthProtectedResourceMetadata ParseOAuthProtectedResourceMetadata(const JSON& Data);
OAuthMetadata ParseOAuthMetadata(const JSON& Data);
OAuthTokens ParseOAuthTokens(const JSON& Data);
OAuthClientInformationFull ParseOAuthClientInformationFull(const JSON& Data);

/**
 * Stores information about registered OAuth clients for this server.
 */
// TODO: Consider making a class since it has logic
struct OAuthRegisteredClientsStore {
    virtual ~OAuthRegisteredClientsStore() = default;

    /**
     * Returns information about a registered client, based on its ID.
     */
    virtual future<optional<OAuthClientInformationFull>> GetClient(const string& ClientID) = 0;

    /**
     * Registers a new client with the server. The client ID and secret will be automatically
     * generated by the library. A modified version of the client information can be returned to
     * reflect specific values enforced by the server.
     *
     * NOTE: Implementations should NOT delete expired client secrets in-place. Auth middleware
     * provided by this library will automatically check the `client_secret_expires_at` field and
     * reject requests with expired secrets. Any custom logic for authenticating clients should
     * check the `client_secret_expires_at` field as well.
     *
     * If unimplemented, dynamic client registration is unsupported.
     */
    virtual future<optional<OAuthClientInformationFull>>
    RegisterClient(const OAuthClientInformationFull& Client) {
        return async(launch::deferred, []() {
            return nullopt;
        }); // Default implementation - dynamic client registration is unsupported
    }
};

MCP_NAMESPACE_END
