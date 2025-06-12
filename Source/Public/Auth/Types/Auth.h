#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// RFC 9728 OAuth Protected Resource Metadata
struct OAuthProtectedResourceMetadata {
    string Resource;                               // TODO: string.url()
    optional<vector<string>> AuthorizationServers; // TODO: array(string.url())
    optional<string> JWKS_URI;                     // TODO: string.url()
    optional<vector<string>> ScopesSupported;
    optional<vector<string>> BearerMethodsSupported;
    optional<vector<string>> ResourceSigningAlgValuesSupported;
    optional<string> ResourceName;
    optional<string> ResourceDocumentation;
    optional<string> ResourcePolicyURI; // TODO: string.url()
    optional<string> Resource_TOS_URI;  // TODO: string.url()
    optional<bool> TLS_ClientCertificateBoundAccessTokens;
    optional<vector<string>> AuthorizationDetailsTypesSupported;
    optional<vector<string>> DPOP_SigningAlgValuesSupported;
    optional<bool> DPOP_BoundAccessTokensRequired;
    unordered_map<string, JSON> Additional; // For passthrough properties
};

// RFC 8414 OAuth 2.0 Authorization Server Metadata
struct OAuthMetadata {
    string Issuer;
    string AuthorizationEndpoint;
    string TokenEndpoint;
    optional<string> RegistrationEndpoint;
    optional<vector<string>> ScopesSupported;
    vector<string> ResponseTypesSupported;
    optional<vector<string>> ResponseModesSupported;
    optional<vector<string>> GrantTypesSupported;
    optional<vector<string>> TokenEndpointAuthMethodsSupported;
    optional<vector<string>> TokenEndpointAuthSigningAlgValuesSupported;
    optional<string> ServiceDocumentation;
    optional<string> RevocationEndpoint;
    optional<vector<string>> RevocationEndpointAuthMethodsSupported;
    optional<vector<string>> RevocationEndpointAuthSigningAlgValuesSupported;
    optional<string> IntrospectionEndpoint;
    optional<vector<string>> IntrospectionEndpointAuthMethodsSupported;
    optional<vector<string>> IntrospectionEndpointAuthSigningAlgValuesSupported;
    optional<vector<string>> CodeChallengeMethodsSupported;
    unordered_map<string, JSON> Additional; // For passthrough properties
};

// OAuth 2.1 token response
struct OAuthTokens {
    string AccessToken;
    string TokenType;
    optional<int> ExpiresIn;
    optional<string> Scope;
    optional<string> RefreshToken;
};

// RFC 7591 OAuth 2.0 Dynamic Client Registration metadata
struct OAuthClientMetadata {
    vector<string>
        RedirectURIs; // TODO: z.array(z.string()).refine((uris) => uris.every((uri) =>
                      // URL.canParse(uri)), { message: "redirect_uris must contain valid URLs" }),
    optional<string> TokenEndpointAuthMethod;
    optional<vector<string>> GrantTypes;
    optional<vector<string>> ResponseTypes;
    optional<string> ClientName;
    optional<string> ClientURI;
    optional<string> LogoURI;
    optional<string> Scope;
    optional<vector<string>> Contacts;
    optional<string> TOS_URI;
    optional<string> PolicyURI;
    optional<string> JWKS_URI;
    optional<any> JWKS;
    optional<string> SoftwareID;
    optional<string> SoftwareVersion;

    // TODO: Verify if this needs the following
    // static expected<OAuthClientMetadata, string> SafeParse(const JSON& json) {
    //     try {
    //         OAuthClientMetadata metadata;
    //         if (json.contains("token_endpoint_auth_method")) {
    //             metadata.TokenEndpointAuthMethod = json["token_endpoint_auth_method"];
    //         }
    //         return metadata;
    //     } catch (const exception& e) { return unexpected(e.what()); }
    // }
};

// RFC 7591 OAuth 2.0 Dynamic Client Registration client information
struct OAuthClientInformation {
    string ClientID;
    optional<string> ClientSecret;
    optional<int> ClientID_IssuedAt;
    optional<int> ClientSecretExpiresAt;
    // TODO: Verify if this needs the following
    // string TokenEndpointAuthMethod;

    // JSON ToJSON() const {
    //     JSON result;
    //     result[MSG_CLIENT_ID] = ClientId;
    //     if (ClientSecret()) { result["client_secret"] = *ClientSecret; }
    //     result["client_id_issued_at"] = ClientIdIssuedAt;
    //     if (ClientSecretExpiresAt()) {
    //         result["client_secret_expires_at"] = *ClientSecretExpiresAt;
    //     }
    //     result["token_endpoint_auth_method"] = TokenEndpointAuthMethod;
    //     return result;
    // }
};

// RFC 7591 OAuth 2.0 Dynamic Client Registration full response (client information plus metadata)
struct OAuthClientInformationFull {
    OAuthClientMetadata Metadata;
    OAuthClientInformation Information;
};

// RFC 7591 OAuth 2.0 Dynamic Client Registration error response
struct OAuthClientRegistrationError {
    string Error;
    optional<string> ErrorDescription;
};

// RFC 7009 OAuth 2.0 Token Revocation request
struct OAuthTokenRevocationRequest {
    string Token;
    optional<string> TokenTypeHint;
};

// Information about a validated access token, provided to request handlers.
struct AuthInfo {
    string Token;               // The access token.
    string ClientID;            // The client ID associated with this token.
    vector<string> Scopes;      // The scopes associated with this token.
    optional<double> ExpiresAt; // When the token expires (in seconds since epoch).

    // Additional data associated with the token.
    // This field should be used for any additional data that needs to be attached to the auth info.
    optional<unordered_map<string, any>> Extra;
};

MCP_NAMESPACE_END