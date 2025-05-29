#pragma once

#include "../Core/Common.hpp"

namespace MCP::Auth {

// TODO: Fix External Ref: OAuthErrorResponse from shared auth

/**
 * Base class for all OAuth errors
 */
class OAuthError : public Error {
private:
    string errorCode_;
    string message_;
    optional<string> errorURI_;

public:
    OAuthError(const string& ErrorCode, const string& Message, const optional<string>& ErrorURI = nullopt)
        : errorCode_(ErrorCode), message_(Message), errorURI_(ErrorURI) {}

    const char* what() const noexcept override {
        return message_.c_str();
    }

    const string& GetErrorCode() const { return errorCode_; }
    const string& GetMessage() const { return message_; }
    const optional<string>& GetErrorURI() const { return errorURI_; }

    /**
     * Converts the error to a standard OAuth error response object
     */
    Auth::OAuthErrorResponse ToResponseObject() const {
        Auth::OAuthErrorResponse response;
        response.Error = errorCode_;
        response.ErrorDescription = message_;

        if (errorURI_.has_value()) {
            response.ErrorURI = errorURI_.value();
        }

        return response;
    }
};

/**
 * Invalid request error - The request is missing a required parameter,
 * includes an invalid parameter value, includes a parameter more than once,
 * or is otherwise malformed.
 */
class InvalidRequestError : public OAuthError {
public:
    InvalidRequestError(const string& Message, const optional<string>& ErrorURI = nullopt)
        : OAuthError("invalid_request", Message, ErrorURI) {}
};

/**
 * Invalid client error - Client authentication failed (e.g., unknown client, no client
 * authentication included, or unsupported authentication method).
 */
class InvalidClientError : public OAuthError {
public:
    InvalidClientError(const string& Message, const optional<string>& ErrorURI = nullopt)
        : OAuthError("invalid_client", Message, ErrorURI) {}
};

/**
 * Invalid grant error - The provided authorization grant or refresh token is
 * invalid, expired, revoked, does not match the redirection URI used in the
 * authorization request, or was issued to another client.
 */
class InvalidGrantError : public OAuthError {
public:
    InvalidGrantError(const string& Message, const optional<string>& ErrorURI = nullopt)
        : OAuthError("invalid_grant", Message, ErrorURI) {}
};

/**
 * Unauthorized client error - The authenticated client is not authorized to use
 * this authorization grant type.
 */
class UnauthorizedClientError : public OAuthError {
public:
    UnauthorizedClientError(const string& Message, const optional<string>& ErrorURI = nullopt)
        : OAuthError("unauthorized_client", Message, ErrorURI) {}
};

/**
 * Unsupported grant type error - The authorization grant type is not supported
 * by the authorization server.
 */
class UnsupportedGrantTypeError : public OAuthError {
public:
    UnsupportedGrantTypeError(const string& Message, const optional<string>& ErrorURI = nullopt)
        : OAuthError("unsupported_grant_type", Message, ErrorURI) {}
};

/**
 * Invalid scope error - The requested scope is invalid, unknown, malformed, or
 * exceeds the scope granted by the resource owner.
 */
class InvalidScopeError : public OAuthError {
public:
    InvalidScopeError(const string& Message, const optional<string>& ErrorURI = nullopt)
        : OAuthError("invalid_scope", Message, ErrorURI) {}
};

/**
 * Access denied error - The resource owner or authorization server denied the request.
 */
class AccessDeniedError : public OAuthError {
public:
    AccessDeniedError(const string& Message, const optional<string>& ErrorURI = nullopt)
        : OAuthError("access_denied", Message, ErrorURI) {}
};

/**
 * Server error - The authorization server encountered an unexpected condition
 * that prevented it from fulfilling the request.
 */
class ServerError : public OAuthError {
public:
    ServerError(const string& Message, const optional<string>& ErrorURI = nullopt)
        : OAuthError("server_error", Message, ErrorURI) {}
};

/**
 * Temporarily unavailable error - The authorization server is currently unable to
 * handle the request due to a temporary overloading or maintenance of the server.
 */
class TemporarilyUnavailableError : public OAuthError {
public:
    TemporarilyUnavailableError(const string& Message, const optional<string>& ErrorURI = nullopt)
        : OAuthError("temporarily_unavailable", Message, ErrorURI) {}
};

/**
 * Unsupported response type error - The authorization server does not support
 * obtaining an authorization code using this method.
 */
class UnsupportedResponseTypeError : public OAuthError {
public:
    UnsupportedResponseTypeError(const string& Message, const optional<string>& ErrorURI = nullopt)
        : OAuthError("unsupported_response_type", Message, ErrorURI) {}
};

/**
 * Unsupported token type error - The authorization server does not support
 * the requested token type.
 */
class UnsupportedTokenTypeError : public OAuthError {
public:
    UnsupportedTokenTypeError(const string& Message, const optional<string>& ErrorURI = nullopt)
        : OAuthError("unsupported_token_type", Message, ErrorURI) {}
};

/**
 * Invalid token error - The access token provided is expired, revoked, malformed,
 * or invalid for other reasons.
 */
class InvalidTokenError : public OAuthError {
public:
    InvalidTokenError(const string& Message, const optional<string>& ErrorURI = nullopt)
        : OAuthError("invalid_token", Message, ErrorURI) {}
};

/**
 * Method not allowed error - The HTTP method used is not allowed for this endpoint.
 * (Custom, non-standard error)
 */
class MethodNotAllowedError : public OAuthError {
public:
    MethodNotAllowedError(const string& Message, const optional<string>& ErrorURI = nullopt)
        : OAuthError("method_not_allowed", Message, ErrorURI) {}
};

/**
 * Too many requests error - Rate limit exceeded.
 * (Custom, non-standard error based on RFC 6585)
 */
class TooManyRequestsError : public OAuthError {
public:
    TooManyRequestsError(const string& Message, const optional<string>& ErrorURI = nullopt)
        : OAuthError("too_many_requests", Message, ErrorURI) {}
};

/**
 * Invalid client metadata error - The client metadata is invalid.
 * (Custom error for dynamic client registration - RFC 7591)
 */
class InvalidClientMetadataError : public OAuthError {
public:
    InvalidClientMetadataError(const string& Message, const optional<string>& ErrorURI = nullopt)
        : OAuthError("invalid_client_metadata", Message, ErrorURI) {}
};

/**
 * Insufficient scope error - The request requires higher privileges than provided by the access token.
 */
class InsufficientScopeError : public OAuthError {
public:
    InsufficientScopeError(const string& Message, const optional<string>& ErrorURI = nullopt)
        : OAuthError("insufficient_scope", Message, ErrorURI) {}
};

} // namespace MCP::Auth
