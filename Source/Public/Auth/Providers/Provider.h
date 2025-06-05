#pragma once

#include "Auth/Types/Auth.h"
#include "Core.h"

MCP_NAMESPACE_BEGIN

struct AuthorizationParams {
    optional<string> State;
    optional<vector<string>> Scopes;
    string CodeChallenge;
    string RedirectURI;
};

/**
 * Implements an end-to-end OAuth server.
 */
// TODO: Consider making this a class because it has functionality
struct OAuthServerProvider {
    virtual ~OAuthServerProvider() = default;

    /**
     * A store used to read information about registered OAuth clients.
     */
    virtual OAuthRegisteredClientsStore& GetClientsStore() = 0;

    /**
     * Begins the authorization flow, which can either be implemented by this server itself or via
     * redirection to a separate authorization server.
     *
     * This server must eventually issue a redirect with an authorization response or an error
     * response to the given redirect URI. Per OAuth 2.1:
     * - In the successful case, the redirect MUST include the `code` and `state` (if present) query
     * parameters.
     * - In the error case, the redirect MUST include the `error` query parameter, and MAY include
     * an optional `error_description` query parameter.
     */
    virtual future<void> Authorize(const OAuthClientInformationFull& Client,
                                   const AuthorizationParams& Params, HTTP_Response& Response) = 0;

    /**
     * Returns the `codeChallenge` that was used when the indicated authorization began.
     */
    virtual future<string> ChallengeForAuthorizationCode(const OAuthClientInformationFull& Client,
                                                         const string& AuthorizationCode) = 0;

    /**
     * Exchanges an authorization code for an access token.
     */
    virtual future<OAuthTokens>
    ExchangeAuthorizationCode(const OAuthClientInformationFull& Client,
                              const string& AuthorizationCode,
                              const optional<string>& CodeVerifier = nullopt,
                              const optional<string>& RedirectURI = nullopt) = 0;

    /**
     * Exchanges a refresh token for an access token.
     */
    virtual future<OAuthTokens>
    ExchangeRefreshToken(const OAuthClientInformationFull& Client, const string& RefreshToken,
                         const optional<vector<string>>& Scopes = nullopt) = 0;

    /**
     * Verifies an access token and returns information about it.
     */
    virtual future<AuthInfo> VerifyAccessToken(const string& Token) = 0;

    /**
     * Revokes an access or refresh token. If unimplemented, token revocation is not supported (not
     * recommended).
     *
     * If the given token is invalid or already revoked, this method should do nothing.
     */
    virtual future<void> RevokeToken(const OAuthClientInformationFull& Client,
                                     const OAuthTokenRevocationRequest& Request) {
        return async(launch::deferred, []() {});
    }

    /**
     * Whether to skip local PKCE validation.
     *
     * If true, the server will not perform PKCE validation locally and will pass the code_verifier
     * to the upstream server.
     *
     * NOTE: This should only be true if the upstream server is performing the actual PKCE
     * validation.
     */
    bool SkipLocalPKCE_Validation = false;
};

/**
 * Slim implementation useful for token verification
 */
// TODO: Consider making this a class because it has functionality
struct OAuthTokenVerifier {
    virtual ~OAuthTokenVerifier() = default;

    /**
     * Verifies an access token and returns information about it.
     */
    virtual future<AuthInfo> VerifyAccessToken(const string& Token) = 0;
};

MCP_NAMESPACE_END
