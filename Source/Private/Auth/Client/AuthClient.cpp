#include "Auth/Client/AuthClient.h"

// TODO: Fix External Ref: pkce-challenge (PKCE challenge generation)
// TODO: Fix External Ref: fetch API (HTTP requests)
// TODO: Fix External Ref: MCP_LATEST_PROTOCOL_VERSION from "../types.js"
// TODO: Fix External Ref: Zod schemas validation

MCP_NAMESPACE_BEGIN

variant<string, future<string>> OAuthClientProvider::GetState() {
    return string(""); // Default empty state
}

variant<void, future<void>>
OAuthClientProvider::SaveClientInformation(const OAuthClientInformationFull& ClientInformation) {
    // Default implementation does nothing
}

future<AuthResult> Auth(shared_ptr<OAuthClientProvider> Provider, const AuthParams& Params);

optional<string> ExtractResourceMetadataURL(const unordered_map<string, string>& ResponseHeaders);

future<OAuthProtectedResourceMetadata> DiscoverOAuthProtectedResourceMetadataAsync(
    const string& ServerURL, const optional<DiscoverMetadataOptions>& Options = nullopt);

future<optional<OAuthMetadata>>
DiscoverOAuthMetadataAsync(const string& AuthorizationServerURL,
                           const optional<DiscoverMetadataOptions>& Options = nullopt);

future<AuthorizationResult> StartAuthorizationAsync(const string& AuthorizationServerURL,
                                                    const StartAuthorizationParams& Params);

future<OAuthTokens> ExchangeAuthorizationAsync(const string& AuthorizationServerURL,
                                               const ExchangeAuthorizationParams& Params);

future<OAuthTokens> RefreshAuthorizationAsync(const string& AuthorizationServerURL,
                                              const RefreshAuthorizationParams& Params);

future<OAuthClientInformationFull> RegisterClientAsync(const string& AuthorizationServerURL,
                                                       const RegisterClientParams& Params);

PKCE_Challenge GeneratePKCE_Challenge();

bool HTTP_Response::IsOK() const {
    return StatusCode >= 200 && StatusCode < 300;
}

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

virtual future<optional<OAuthClientInformationFull>>
OAuthRegisteredClientsStore::RegisterClient(const OAuthClientInformationFull& Client) {
    return async(launch::deferred, []() {
        return nullopt;
    }); // Default implementation - dynamic client registration is unsupported
}

MCP_NAMESPACE_END
