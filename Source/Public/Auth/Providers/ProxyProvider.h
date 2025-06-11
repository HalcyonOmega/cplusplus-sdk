#pragma once

#include "Auth/Client/AuthClient.h"
#include "Auth/Providers/Provider.hpp"
#include "Auth/Types/Auth.h"
#include "Auth/Types/AuthErrors.h"
#include "Core.h"

MCP_NAMESPACE_BEGIN

// TODO: Fix External: URL encoding functionality

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
    const ProxyEndpoints m_Endpoints;
    const function<future<AuthInfo>(const string&)> m_VerifyAccessToken;
    const function<future<optional<OAuthClientInformationFull>>(const string&)> m_GetClient;

  public:
    bool SkipLocalPkceValidation = true;

    optional<function<future<void>(const OAuthClientInformationFull&,
                                   const OAuthTokenRevocationRequest&)>>
        RevokeToken;

    explicit ProxyOAuthServerProvider(const ProxyOptions& InOptions)
        : m_Endpoints(InOptions.Endpoints), m_VerifyAccessToken(InOptions.VerifyAccessToken),
          m_GetClient(InOptions.GetClient) {
        if (InOptions.Endpoints.RevocationUrl.has_value()) {
            RevokeToken = [this](const OAuthClientInformationFull& InClient,
                                 const OAuthTokenRevocationRequest& InRequest) -> future<void> {
                const auto& RevocationUrl = this->m_Endpoints.RevocationUrl;

                if (!RevocationUrl.has_value()) {
                    throw runtime_error("No revocation endpoint configured");
                }

                map<string, string> Params;
                Params["token"] = Request.Token;
                Params[MSG_CLIENT_ID] = Client.Information.ClientID;
                if (Client.Information.ClientSecret.has_value()) {
                    Params["client_secret"] = Client.Information.ClientSecret.value();
                }
                if (Request.TokenTypeHint.has_value()) {
                    Params["token_type_hint"] = Request.TokenTypeHint.value();
                }

                // Build form-encoded body
                string Body = BuildFormEncodedBody(Params);

                auto Response = co_await HTTP_Post(
                    RevocationUrl.value(),
                    {{TSPT_CONTENT_TYPE, "application/x-www-form-urlencoded"}}, Body);

                if (!Response.IsOK()) {
                    throw ServerError("Token revocation failed: " + to_string(Response.StatusCode));
                }

                co_return;
            };
        }
    }

    OAuthRegisteredClientsStore GetClientsStore() const;

    future<void> Authorize(const OAuthClientInformationFull& InClient,
                           const AuthorizationParams& InParams, Response& InResponse) const;

    future<string> ChallengeForAuthorizationCode(const OAuthClientInformationFull& InClient,
                                                 const string& InAuthorizationCode) const;

    future<OAuthTokens>
    ExchangeAuthorizationCode(const OAuthClientInformationFull& InClient,
                              const string& AuthorizationCode,
                              const optional<string>& CodeVerifier = nullopt,
                              const optional<string>& RedirectUri = nullopt) const;

    future<OAuthTokens>
    ExchangeRefreshToken(const OAuthClientInformationFull& InClient, const string& InRefreshToken,
                         const optional<vector<string>>& InScopes = nullopt) const;

    future<AuthInfo> VerifyAccessToken(const string& InToken) const;

  private:
    static future<HTTP_Response>
    HTTP_Post(const string& InUrl, const map<string, string>& InHeaders, const string& InBody);

    static string BuildFormEncodedBody(const map<string, string>& InParams);

    static string BuildQueryString(const map<string, string>& InParams);

    static string URLEncode(const string& InValue);

    static OAuthClientInformationFull ParseOAuthClientInformationFull(const JSON& InData);

    static OAuthTokens ParseOAuthTokens(const JSON& InData);
};

MCP_NAMESPACE_END