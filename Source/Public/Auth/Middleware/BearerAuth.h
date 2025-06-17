#pragma once

#include "Auth/Types/Auth.h"
#include "Core.h"
#include "Core/Constants/TransportConstants.h"

MCP_NAMESPACE_BEGIN

// TODO: Fix External Ref: RequestHandler (Express equivalent)
// TODO: Fix External Ref: InsufficientScopeError
// TODO: Fix External Ref: InvalidTokenError
// TODO: Fix External Ref: ServerError
// TODO: Fix External Ref: OAuthTokenVerifier

/**
 * Configuration options for Bearer authentication middleware
 */
struct BearerAuthMiddlewareOptions {
    /**
     * A provider used to verify tokens.
     */
    shared_ptr<OAuthTokenVerifier> Verifier;

    /**
     * Optional scopes that the token must have.
     */
    optional<vector<string>> RequiredScopes;

    /**
     * Optional resource metadata URL to include in WWW-Authenticate header.
     */
    optional<string> ResourceMetadataUrl;
};

/**
 * Request context that includes authentication information
 */
// TODO: Consider child class of RequestBase
struct AuthenticatedRequest {
    /**
     * Information about the validated access token, if the RequireBearerAuth middleware was used.
     */
    optional<AuthInfo> Auth;

    // TODO: Add other request properties as needed (headers, body, etc.)
    HTTP::Headers Headers;
    string Method;
    string Path;
};

/**
 * Response context for middleware
 */
struct MiddlewareResponse {
    HTTP::Status StatusCode = HTTP::Status::Ok;
    HTTP::Headers Headers;
    JSON Body;

    void SetStatus(HTTP::Status Status);

    void SetHeader(const string& Key, const string& Value);

    void SendJSON(const JSON& JsonData);
};

/**
 * Middleware function type
 */
using MiddlewareFunction =
    function<void(AuthenticatedRequest&, MiddlewareResponse&, function<void()>)>;

/**
 * Helper function for case-insensitive header lookup
 */
string GetHeaderCaseInsensitive(const HTTP::Headers& Headers, const string& HeaderName);

/**
 * Middleware that requires a valid Bearer token in the Authorization header.
 *
 * This will validate the token with the auth provider and add the resulting auth info to the
 * request object.
 *
 * If ResourceMetadataUrl is provided, it will be included in the WWW-Authenticate header
 * for HTTP::Status::Unauthorized responses as per the OAuth 2.0 Protected Resource Metadata spec.
 */
MiddlewareFunction RequireBearerAuth(const BearerAuthMiddlewareOptions& Options);

MCP_NAMESPACE_END
