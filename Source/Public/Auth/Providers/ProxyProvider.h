#pragma once

#include "Auth/Client/AuthClient.hpp"
#include "Auth/Providers/Provider.hpp"
#include "Auth/Types/Auth.h"
#include "Auth/Types/AuthErrors.h"
#include "Core.h"

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
    function<future<AuthInfo>(const string&)> VerifyAccessToken;

    /**
     * Function to fetch client information from the upstream server
     */
    function<future<optional<OAuthClientInformationFull>>(const string&)> GetClient;
};

/**
 * Implements an OAuth server that proxies requests to another OAuth server.
 */
class ProxyOAuthServerProvider : public OAuthServerProvider {
  protected:
    const ProxyEndpoints _Endpoints;
    const function<future<AuthInfo>(const string&)> _VerifyAccessToken;
    const function<future<optional<OAuthClientInformationFull>>(const string&)> _GetClient;

  public:
    bool SkipLocalPkceValidation = true;

    optional<function<future<void>(const OAuthClientInformationFull&,
                                   const OAuthTokenRevocationRequest&)>>
        RevokeToken;

    explicit ProxyOAuthServerProvider(const ProxyOptions& Options)
        : _Endpoints(Options.Endpoints), _VerifyAccessToken(Options.VerifyAccessToken),
          _GetClient(Options.GetClient) {
        if (Options.Endpoints.RevocationUrl.has_value()) {
            RevokeToken = [this](const OAuthClientInformationFull& Client,
                                 const OAuthTokenRevocationRequest& Request) -> future<void> {
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
                    throw ServerError("Token revocation failed: " + to_string(Response.Status));
                }

                co_return;
            };
        }
    }

    OAuthRegisteredClientsStore GetClientsStore() const;

    future<void> Authorize(const OAuthClientInformationFull& Client,
                           const AuthorizationParams& Params, Response& Res) const;

    future<string> ChallengeForAuthorizationCode(const OAuthClientInformationFull& Client,
                                                 const string& AuthorizationCode) const;

    future<OAuthTokens>
    ExchangeAuthorizationCode(const OAuthClientInformationFull& Client,
                              const string& AuthorizationCode,
                              const optional<string>& CodeVerifier = nullopt,
                              const optional<string>& RedirectUri = nullopt) const;

    future<OAuthTokens>
    ExchangeRefreshToken(const OAuthClientInformationFull& Client, const string& RefreshToken,
                         const optional<vector<string>>& Scopes = nullopt) const;

    future<AuthInfo> VerifyAccessToken(const string& Token) const;

  private:
    // TODO: Fix External Ref: HTTP client functionality
    struct HttpResponse {
        bool Ok;
        int Status;
        string Body;
    };

    static future<HttpResponse> HttpPost(const string& Url, const map<string, string>& Headers,
                                         const string& Body);

    static string BuildFormEncodedBody(const map<string, string>& Params);

    static string BuildQueryString(const map<string, string>& Params);

    static string URLEncode(const string& Value);

    static OAuthClientInformationFull ParseOAuthClientInformationFull(const JSON& Data);

    static OAuthTokens ParseOAuthTokens(const JSON& Data);
};

MCP_NAMESPACE_END