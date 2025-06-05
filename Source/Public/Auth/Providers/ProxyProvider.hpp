#pragma once

#include "../../Auth.hpp"
#include "../AuthErrors.hpp"
#include "../Clients.hpp"
#include "../Core/Common.hpp"
#include "../Provider.hpp"

MCP_NAMESPACE_BEGIN

// TODO: Fix External Ref: Response (express equivalent)
// TODO: Fix External Ref: URL encoding functionality

struct ProxyEndpoints {
    string AuthorizationUrl;
    string TokenUrl;
    optional<string> RevocationUrl;
    optional<string> RegistrationUrl;
};

struct ProxyOptions {
    /**
     * Individual endpoint URLs for proxying specific OAuth operations
     */
    ProxyEndpoints Endpoints;

    /**
     * Function to verify access tokens and return auth info
     */
    function<future<Auth::AuthInfo>(const string&)> VerifyAccessToken;

    /**
     * Function to fetch client information from the upstream server
     */
    function<future<optional<Auth::OAuthClientInformationFull>>(const string&)> GetClient;
};

/**
 * Implements an OAuth server that proxies requests to another OAuth server.
 */
class ProxyOAuthServerProvider : public Auth::OAuthServerProvider {
  protected:
    const ProxyEndpoints _Endpoints;
    const function<future<Auth::AuthInfo>(const string&)> _VerifyAccessToken;
    const function<future<optional<Auth::OAuthClientInformationFull>>(const string&)> _GetClient;

  public:
    bool SkipLocalPkceValidation = true;

    optional<function<future<void>(const Auth::OAuthClientInformationFull&,
                                   const Auth::OAuthTokenRevocationRequest&)>>
        RevokeToken;

    explicit ProxyOAuthServerProvider(const ProxyOptions& Options)
        : _Endpoints(Options.Endpoints), _VerifyAccessToken(Options.VerifyAccessToken),
          _GetClient(Options.GetClient) {
        if (Options.Endpoints.RevocationUrl.has_value()) {
            RevokeToken = [this](const Auth::OAuthClientInformationFull& Client,
                                 const Auth::OAuthTokenRevocationRequest& Request) -> future<void> {
                const auto& RevocationUrl = this->_Endpoints.RevocationUrl;

                if (!RevocationUrl.has_value()) {
                    throw runtime_error("No revocation endpoint configured");
                }

                map<string, string> Params;
                Params["token"] = Request.Token;
                Params["client_id"] = Client.Information.ClientID;
                if (Client.Information.ClientSecret.has_value()) {
                    Params["client_secret"] = Client.Information.ClientSecret.value();
                }
                if (Request.TokenTypeHint.has_value()) {
                    Params["token_type_hint"] = Request.TokenTypeHint.value();
                }

                // Build form-encoded body
                string Body = BuildFormEncodedBody(Params);

                auto Response = co_await HttpPost(
                    RevocationUrl.value(), {{"Content-Type", "application/x-www-form-urlencoded"}},
                    Body);

                if (!Response.Ok) {
                    throw Auth::ServerError("Token revocation failed: "
                                            + to_string(Response.Status));
                }

                co_return;
            };
        }
    }

    Auth::OAuthRegisteredClientsStore GetClientsStore() const {
        const auto& RegistrationUrl = _Endpoints.RegistrationUrl;

        Auth::OAuthRegisteredClientsStore Store;
        Store.GetClient = _GetClient;

        if (RegistrationUrl.has_value()) {
            Store.RegisterClient = [RegistrationUrl](const Auth::OAuthClientInformationFull& Client)
                -> future<Auth::OAuthClientInformationFull> {
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

                auto Response =
                    co_await HttpPost(RegistrationUrl.value(),
                                      {{"Content-Type", "application/json"}}, ClientJson.dump());

                if (!Response.Ok) {
                    throw Auth::ServerError("Client registration failed: "
                                            + to_string(Response.Status));
                }

                JSON Data = JSON::parse(Response.Body);
                co_return ParseOAuthClientInformationFull(Data);
            };
        }

        return Store;
    }

    future<void> Authorize(const Auth::OAuthClientInformationFull& Client,
                           const Auth::AuthorizationParams& Params, Response& Res) const {
        string TargetUrl = _Endpoints.AuthorizationUrl;

        map<string, string> SearchParams = {{"client_id", Client.Information.ClientID},
                                            {"response_type", MSG_KEY_CODE},
                                            {"redirect_uri", Params.RedirectUri},
                                            {"code_challenge", Params.CodeChallenge},
                                            {"code_challenge_method", "S256"}};

        // Add optional standard OAuth parameters
        if (Params.State.has_value()) {
            SearchParams["state"] = Params.State.value();
        }
        if (!Params.Scopes.empty()) {
            string ScopeStr;
            for (size_t I = 0; I < Params.Scopes.size(); ++I) {
                if (I > 0)
                    ScopeStr += " ";
                ScopeStr += Params.Scopes[I];
            }
            SearchParams["scope"] = ScopeStr;
        }

        string QueryString = BuildQueryString(SearchParams);
        string FullUrl = TargetUrl + "?" + QueryString;
        Res.Redirect(FullUrl);

        co_return;
    }

    future<string> ChallengeForAuthorizationCode(const Auth::OAuthClientInformationFull& Client,
                                                 const string& AuthorizationCode) const {
        // In a proxy setup, we don't store the code challenge ourselves
        // Instead, we proxy the token request and let the upstream server validate it
        co_return "";
    }

    future<Auth::OAuthTokens>
    ExchangeAuthorizationCode(const Auth::OAuthClientInformationFull& Client,
                              const string& AuthorizationCode,
                              const optional<string>& CodeVerifier = nullopt,
                              const optional<string>& RedirectUri = nullopt) const {
        map<string, string> Params = {{"grant_type", "authorization_code"},
                                      {"client_id", Client.Information.ClientID},
                                      {MSG_KEY_CODE, AuthorizationCode}};

        if (Client.Information.ClientSecret.has_value()) {
            Params["client_secret"] = Client.Information.ClientSecret.value();
        }

        if (CodeVerifier.has_value()) {
            Params["code_verifier"] = CodeVerifier.value();
        }

        if (RedirectUri.has_value()) {
            Params["redirect_uri"] = RedirectUri.value();
        }

        string Body = BuildFormEncodedBody(Params);

        auto Response = co_await HttpPost(
            _Endpoints.TokenUrl, {{"Content-Type", "application/x-www-form-urlencoded"}}, Body);

        if (!Response.Ok) {
            throw Auth::ServerError("Token exchange failed: " + to_string(Response.Status));
        }

        JSON Data = JSON::parse(Response.Body);
        co_return ParseOAuthTokens(Data);
    }

    future<Auth::OAuthTokens>
    ExchangeRefreshToken(const Auth::OAuthClientInformationFull& Client, const string& RefreshToken,
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
                if (I > 0)
                    ScopeStr += " ";
                ScopeStr += Scopes.value()[I];
            }
            Params["scope"] = ScopeStr;
        }

        string Body = BuildFormEncodedBody(Params);

        auto Response = co_await HttpPost(
            _Endpoints.TokenUrl, {{"Content-Type", "application/x-www-form-urlencoded"}}, Body);

        if (!Response.Ok) {
            throw Auth::ServerError("Token refresh failed: " + to_string(Response.Status));
        }

        JSON Data = JSON::parse(Response.Body);
        co_return ParseOAuthTokens(Data);
    }

    future<Auth::AuthInfo> VerifyAccessToken(const string& Token) const {
        co_return co_await _VerifyAccessToken(Token);
    }

  private:
    // TODO: Fix External Ref: HTTP client functionality
    struct HttpResponse {
        bool Ok;
        int Status;
        string Body;
    };

    static future<HttpResponse> HttpPost(const string& Url, const map<string, string>& Headers,
                                         const string& Body) {
        // TODO: Implement actual HTTP POST request
        co_return HttpResponse{false, 500, ""};
    }

    static string BuildFormEncodedBody(const map<string, string>& Params) {
        string Body;
        bool First = true;
        for (const auto& [Key, Value] : Params) {
            if (!First)
                Body += "&";
            Body += URLEncode(Key) + "=" + URLEncode(Value);
            First = false;
        }
        return Body;
    }

    static string BuildQueryString(const map<string, string>& Params) {
        return BuildFormEncodedBody(Params); // Same format
    }

    static string URLEncode(const string& Value) {
        // TODO: Implement proper URL encoding
        return Value;
    }

    static Auth::OAuthClientInformationFull ParseOAuthClientInformationFull(const JSON& Data) {
        Auth::OAuthClientInformationFull Result;

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

    static Auth::OAuthTokens ParseOAuthTokens(const JSON& Data) {
        Auth::OAuthTokens Result;
        Result.AccessToken = Data.at("access_token").get<string>();
        Result.TokenType = Data.at("token_type").get<string>();

        if (Data.contains("expires_in")) {
            Result.ExpiresIn = Data.at("expires_in").get<int>();
        }
        if (Data.contains("scope")) {
            Result.Scope = Data.at("scope").get<string>();
        }
        if (Data.contains("refresh_token")) {
            Result.RefreshToken = Data.at("refresh_token").get<string>();
        }

        return Result;
    }
};

MCP_NAMESPACE_END