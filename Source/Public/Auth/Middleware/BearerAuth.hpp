#pragma once

#include "../Core/Common.hpp"

namespace MCP::Auth {

// TODO: Fix External Ref: RequestHandler (Express equivalent)
// TODO: Fix External Ref: InsufficientScopeError
// TODO: Fix External Ref: InvalidTokenError
// TODO: Fix External Ref: OAuthError
// TODO: Fix External Ref: ServerError
// TODO: Fix External Ref: OAuthTokenVerifier
// TODO: Fix External Ref: AuthInfo

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
struct AuthenticatedRequest {
    /**
     * Information about the validated access token, if the RequireBearerAuth middleware was used.
     */
    optional<AuthInfo> Auth;

    // TODO: Add other request properties as needed (headers, body, etc.)
    map<string, string> Headers;
    string Method;
    string Path;
};

/**
 * Response context for middleware
 */
struct MiddlewareResponse {
    int StatusCode = 200;
    map<string, string> Headers;
    JSON Body;

    void SetStatus(int Status) {
        StatusCode = Status;
    }

    void SetHeader(const string& Key, const string& Value) {
        Headers[Key] = Value;
    }

    void SendJSON(const JSON& JsonData) {
        Body = JsonData;
    }
};

/**
 * Middleware function type
 */
using MiddlewareFunction = function<void(AuthenticatedRequest&, MiddlewareResponse&, function<void()>)>;

/**
 * Helper function for case-insensitive header lookup
 */
string GetHeaderCaseInsensitive(const map<string, string>& Headers, const string& HeaderName) {
    string LowerHeaderName = HeaderName;
    transform(LowerHeaderName.begin(), LowerHeaderName.end(), LowerHeaderName.begin(), ::tolower);

    for (const auto& [Key, Value] : Headers) {
        string LowerKey = Key;
        transform(LowerKey.begin(), LowerKey.end(), LowerKey.begin(), ::tolower);
        if (LowerKey == LowerHeaderName) {
            return Value;
        }
    }
    return "";
}

/**
 * Middleware that requires a valid Bearer token in the Authorization header.
 *
 * This will validate the token with the auth provider and add the resulting auth info to the request object.
 *
 * If ResourceMetadataUrl is provided, it will be included in the WWW-Authenticate header
 * for 401 responses as per the OAuth 2.0 Protected Resource Metadata spec.
 */
MiddlewareFunction RequireBearerAuth(const BearerAuthMiddlewareOptions& Options) {
    return [Options](AuthenticatedRequest& Request, MiddlewareResponse& Response, function<void()> Next) {
        try {
            // Check for Authorization header (case-insensitive)
            string AuthHeader = GetHeaderCaseInsensitive(Request.Headers, "authorization");
            if (AuthHeader.empty()) {
                throw InvalidTokenError("Missing Authorization header");
            }

            // Parse Bearer token - handle multiple spaces properly
            size_t SpacePos = AuthHeader.find(' ');
            if (SpacePos == string::npos) {
                throw InvalidTokenError("Invalid Authorization header format, expected 'Bearer TOKEN'");
            }

            string Type = AuthHeader.substr(0, SpacePos);
            string Token = AuthHeader.substr(SpacePos + 1);

            // Remove any leading/trailing whitespace from token
            Token.erase(0, Token.find_first_not_of(" \t"));
            Token.erase(Token.find_last_not_of(" \t") + 1);

            // Convert type to lowercase for comparison
            transform(Type.begin(), Type.end(), Type.begin(), ::tolower);

            if (Type != "bearer" || Token.empty()) {
                throw InvalidTokenError("Invalid Authorization header format, expected 'Bearer TOKEN'");
            }

            // Verify the access token (should be async in real implementation)
            AuthInfo TokenAuthInfo = Options.Verifier->VerifyAccessToken(Token);

            // Check if token has the required scopes (handle default empty array behavior)
            vector<string> RequiredScopes = Options.RequiredScopes.value_or(vector<string>{});
            if (!RequiredScopes.empty()) {
                bool HasAllScopes = true;
                for (const string& RequiredScope : RequiredScopes) {
                    bool HasScope = false;
                    for (const string& TokenScope : TokenAuthInfo.Scopes) {
                        if (TokenScope == RequiredScope) {
                            HasScope = true;
                            break;
                        }
                    }
                    if (!HasScope) {
                        HasAllScopes = false;
                        break;
                    }
                }

                if (!HasAllScopes) {
                    throw InsufficientScopeError("Insufficient scope");
                }
            }

            // Check if the token is expired
            if (TokenAuthInfo.ExpiresAt.has_value() && *TokenAuthInfo.ExpiresAt < chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count()) {
                throw InvalidTokenError("Token has expired");
            }

            // Set auth info in request
            Request.Auth = TokenAuthInfo;
            Next();

        } catch (const InvalidTokenError& Error) {
            string WWW_AuthValue;
            if (Options.ResourceMetadataUrl.has_value()) {
                WWW_AuthValue = "Bearer error=\"" + Error.GetErrorCode() + "\", error_description=\"" + Error.GetMessage() + "\", resource_metadata=\"" + *Options.ResourceMetadataUrl + "\"";
            } else {
                WWW_AuthValue = "Bearer error=\"" + Error.GetErrorCode() + "\", error_description=\"" + Error.GetMessage() + "\"";
            }
            Response.SetHeader("WWW-Authenticate", WWW_AuthValue);
            Response.SetStatus(401);
            Response.SendJSON(Error.ToResponseObject());
        } catch (const InsufficientScopeError& Error) {
            string WWW_AuthValue;
            if (Options.ResourceMetadataUrl.has_value()) {
                WWW_AuthValue = "Bearer error=\"" + Error.GetErrorCode() + "\", error_description=\"" + Error.GetMessage() + "\", resource_metadata=\"" + *Options.ResourceMetadataUrl + "\"";
            } else {
                WWW_AuthValue = "Bearer error=\"" + Error.GetErrorCode() + "\", error_description=\"" + Error.GetMessage() + "\"";
            }
            Response.SetHeader("WWW-Authenticate", WWW_AuthValue);
            Response.SetStatus(403);
            Response.SendJSON(Error.ToResponseObject());
        } catch (const ServerError& Error) {
            Response.SetStatus(500);
            Response.SendJSON(Error.ToResponseObject());
        } catch (const OAuthError& Error) {
            Response.SetStatus(400);
            Response.SendJSON(Error.ToResponseObject());
        } catch (const exception& Error) {
            // Log unexpected error
            cerr << "Unexpected error authenticating bearer token: " << Error.what() << endl;
            ServerError InternalError("Internal Server Error");
            Response.SetStatus(500);
            Response.SendJSON(InternalError.ToResponseObject());
        }
    };
}

MCP_NAMESPACE_END
