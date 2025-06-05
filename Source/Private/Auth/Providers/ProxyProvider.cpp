#include "Auth/Providers/ProxyProvider.h"

MCP_NAMESPACE_BEGIN

// TODO: Fix External Ref: Response (express equivalent)
// TODO: Fix External Ref: URL encoding functionality

OAuthRegisteredClientsStore ProxyOAuthServerProvider::GetClientsStore() const {
    const auto& RegistrationUrl = _Endpoints.RegistrationUrl;

    OAuthRegisteredClientsStore Store;
    Store.GetClient = _GetClient;

    if (RegistrationUrl.has_value()) {
        Store.RegisterClient =
            [RegistrationUrl](
                const OAuthClientInformationFull& Client) -> future<OAuthClientInformationFull> {
            // Convert client to JSON
            JSON ClientJson;
            ClientJson["client_id"] = Client.Information.ClientID;
            if (Client.Information.ClientSecret.has_value()) {
                ClientJson["client_secret"] = Client.Information.ClientSecret.value();
            }
            // Add metadata fields
            ClientJson["redirect_uris"] = Client.Metadata.RedirectURIs;
            if (Client.Metadata.ClientName.has_value()) {
                ClientJson["client_name"] = Client.Metadata.ClientName.value();
            }
            // Add other metadata fields as needed

            auto Response = co_await HttpPost(
                RegistrationUrl.value(), {{"Content-Type", "application/json"}}, ClientJson.dump());

            if (!Response.Ok) {
                throw ServerError("Client registration failed: " + to_string(Response.Status));
            }

            JSON Data = JSON::parse(Response.Body);
            co_return ParseOAuthClientInformationFull(Data);
        };
    }

    return Store;
}

future<void> ProxyOAuthServerProvider::Authorize(const OAuthClientInformationFull& Client,
                                                 const AuthorizationParams& Params,
                                                 Response& Res) const {
    string TargetUrl = _Endpoints.AuthorizationUrl;

    map<string, string> SearchParams = {{"client_id", Client.Information.ClientID},
                                        {"response_type", MSG_KEY_CODE},
                                        {"redirect_uri", Params.RedirectUri},
                                        {"code_challenge", Params.CodeChallenge},
                                        {"code_challenge_method", "S256"}};

    // Add optional standard OAuth parameters
    if (Params.State.has_value()) { SearchParams["state"] = Params.State.value(); }
    if (!Params.Scopes.empty()) {
        string ScopeStr;
        for (size_t I = 0; I < Params.Scopes.size(); ++I) {
            if (I > 0) ScopeStr += " ";
            ScopeStr += Params.Scopes[I];
        }
        SearchParams["scope"] = ScopeStr;
    }

    string QueryString = BuildQueryString(SearchParams);
    string FullUrl = TargetUrl + "?" + QueryString;
    Res.Redirect(FullUrl);

    co_return;
}

future<string>
ProxyOAuthServerProvider::ChallengeForAuthorizationCode(const OAuthClientInformationFull& Client,
                                                        const string& AuthorizationCode) const {
    // In a proxy setup, we don't store the code challenge ourselves
    // Instead, we proxy the token request and let the upstream server validate it
    co_return "";
}

future<OAuthTokens> ProxyOAuthServerProvider::ExchangeAuthorizationCode(
    const OAuthClientInformationFull& Client, const string& AuthorizationCode,
    const optional<string>& CodeVerifier = nullopt,
    const optional<string>& RedirectUri = nullopt) const {
    map<string, string> Params = {{"grant_type", "authorization_code"},
                                  {"client_id", Client.Information.ClientID},
                                  {MSG_KEY_CODE, AuthorizationCode}};

    if (Client.Information.ClientSecret.has_value()) {
        Params["client_secret"] = Client.Information.ClientSecret.value();
    }

    if (CodeVerifier.has_value()) { Params["code_verifier"] = CodeVerifier.value(); }

    if (RedirectUri.has_value()) { Params["redirect_uri"] = RedirectUri.value(); }

    string Body = BuildFormEncodedBody(Params);

    auto Response = co_await HttpPost(
        _Endpoints.TokenUrl, {{"Content-Type", "application/x-www-form-urlencoded"}}, Body);

    if (!Response.Ok) { throw ServerError("Token exchange failed: " + to_string(Response.Status)); }

    JSON Data = JSON::parse(Response.Body);
    co_return ParseOAuthTokens(Data);
}

future<OAuthTokens> ProxyOAuthServerProvider::ExchangeRefreshToken(
    const OAuthClientInformationFull& Client, const string& RefreshToken,
    const optional<vector<string>>& Scopes = nullopt) const {
    map<string, string> Params = {{"grant_type", "refresh_token"},
                                  {"client_id", Client.Information.ClientID},
                                  {"refresh_token", RefreshToken}};

    if (Client.Information.ClientSecret.has_value()) {
        Params["client_secret"] = Client.Information.ClientSecret.value();
    }

    if (Scopes.has_value() && !Scopes.value().empty()) {
        string ScopeStr;
        for (size_t I = 0; I < Scopes.value().size(); ++I) {
            if (I > 0) ScopeStr += " ";
            ScopeStr += Scopes.value()[I];
        }
        Params["scope"] = ScopeStr;
    }

    string Body = BuildFormEncodedBody(Params);

    auto Response = co_await HttpPost(
        _Endpoints.TokenUrl, {{"Content-Type", "application/x-www-form-urlencoded"}}, Body);

    if (!Response.Ok) { throw ServerError("Token refresh failed: " + to_string(Response.Status)); }

    JSON Data = JSON::parse(Response.Body);
    co_return ParseOAuthTokens(Data);
}

future<AuthInfo> ProxyOAuthServerProvider::VerifyAccessToken(const string& Token) const {
    co_return co_await _VerifyAccessToken(Token);
}

future<HttpResponse> ProxyOAuthServerProvider::HttpPost(const string& Url,
                                                        const map<string, string>& Headers,
                                                        const string& Body) {
    // TODO: Implement actual HTTP POST request
    co_return HttpResponse{false, 500, ""};
}

string ProxyOAuthServerProvider::BuildFormEncodedBody(const map<string, string>& Params) {
    string Body;
    bool First = true;
    for (const auto& [Key, Value] : Params) {
        if (!First) Body += "&";
        Body += URLEncode(Key) + "=" + URLEncode(Value);
        First = false;
    }
    return Body;
}

string ProxyOAuthServerProvider::BuildQueryString(const map<string, string>& Params) {
    return BuildFormEncodedBody(Params); // Same format
}

string ProxyOAuthServerProvider::URLEncode(const string& Value) {
    // TODO: Implement proper URL encoding
    return Value;
}

OAuthClientInformationFull
ProxyOAuthServerProvider::ParseOAuthClientInformationFull(const JSON& Data) {
    OAuthClientInformationFull Result;

    // Parse client information
    Result.Information.ClientID = Data.at("client_id").get<string>();
    if (Data.contains("client_secret")) {
        Result.Information.ClientSecret = Data.at("client_secret").get<string>();
    }

    // Parse metadata
    if (Data.contains("redirect_uris")) {
        Result.Metadata.RedirectURIs = Data.at("redirect_uris").get<vector<string>>();
    }
    if (Data.contains("client_name")) {
        Result.Metadata.ClientName = Data.at("client_name").get<string>();
    }
    // Add other fields as needed

    return Result;
}

OAuthTokens ProxyOAuthServerProvider::ParseOAuthTokens(const JSON& Data) {
    OAuthTokens Result;
    Result.AccessToken = Data.at("access_token").get<string>();
    Result.TokenType = Data.at("token_type").get<string>();

    if (Data.contains("expires_in")) { Result.ExpiresIn = Data.at("expires_in").get<int>(); }
    if (Data.contains("scope")) { Result.Scope = Data.at("scope").get<string>(); }
    if (Data.contains("refresh_token")) {
        Result.RefreshToken = Data.at("refresh_token").get<string>();
    }

    return Result;
}

MCP_NAMESPACE_END