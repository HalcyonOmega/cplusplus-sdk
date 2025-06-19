#include "Auth/Handlers/Authorize.h"

MCP_NAMESPACE_BEGIN

// TODO: Fix External Ref: Express RequestHandler equivalent
// TODO: Fix External Ref: RateLimit functionality
// TODO: Fix External Ref: Nate - HTTP::Request and HTTP::Response

bool ClientAuthorizationParams::Validate(const map<string, string>& Params,
                                         ClientAuthorizationParams& Output, string& ErrorBase) {
    auto ClientIdIt = Params.find(MSG_CLIENT_ID);
    if (ClientIdIt == Params.end() || ClientIdIt->second.empty()) {
        ErrorBase = "client_id is required";
        return false;
    }
    Output.ClientId = ClientIdIt->second;

    auto RedirectUriIt = Params.find(MSG_REDIRECT_URI);
    if (RedirectUriIt != Params.end() && !RedirectUriIt->second.empty()) {
        // Validate URL format
        regex URLPattern(R"(^https?://[^\s]+$)");
        if (!regex_match(RedirectUriIt->second, URLPattern)) {
            ErrorBase = "redirect_uri must be a valid URL";
            return false;
        }
        Output.RedirectUri = RedirectUriIt->second;
    }

    return true;
}

bool RequestAuthorizationParams::Validate(const map<string, string>& Params,
                                          RequestAuthorizationParams& Output, string& ErrorBase) {
    auto ResponseTypeIt = Params.find(MSG_RESPONSE_TYPE);
    if (ResponseTypeIt == Params.end() || ResponseTypeIt->second != MSG_CODE) {
        ErrorBase = "response_type must be 'code'";
        return false;
    }
    Output.ResponseType = ResponseTypeIt->second;

    auto CodeChallengeIt = Params.find(MSG_CODE_CHALLENGE);
    if (CodeChallengeIt == Params.end() || CodeChallengeIt->second.empty()) {
        ErrorBase = "code_challenge is required";
        return false;
    }
    Output.CodeChallenge = CodeChallengeIt->second;

    auto CodeChallengeMethodIt = Params.find(MSG_CODE_CHALLENGE_METHOD);
    if (CodeChallengeMethodIt == Params.end() || CodeChallengeMethodIt->second != "S256") {
        ErrorBase = "code_challenge_method must be 'S256'";
        return false;
    }
    Output.CodeChallengeMethod = CodeChallengeMethodIt->second;

    auto ScopeIt = Params.find(MSG_SCOPE);
    if (ScopeIt != Params.end() && !ScopeIt->second.empty()) { Output.Scope = ScopeIt->second; }

    auto StateIt = Params.find(MSG_STATE);
    if (StateIt != Params.end() && !StateIt->second.empty()) { Output.State = StateIt->second; }

    return true;
}

bool RateLimiter::CheckRateLimit(const string& ClientIp) {
    auto Now = chrono::steady_clock::now();
    auto& [LastWindow, RequestCount] = ClientRequests[ClientIp];

    if (Now - LastWindow > Options.WindowMs) {
        LastWindow = Now;
        RequestCount = 1;
        return true;
    }

    if (RequestCount >= Options.Max) { return false; }

    RequestCount++;
    return true;
}

string AuthorizationHandler::CreateErrorRedirect(const string& RedirectUri, const OAuthError& Error,
                                                 const optional<string>& State) {
    string ErrorUrl = RedirectUri;
    char Separator = (RedirectUri.find('?') != string::npos) ? '&' : '?';

    ErrorUrl += Separator;
    ErrorUrl += "error=" + Error.GetErrorCode();
    ErrorUrl += "&error_description=" + Error.GetMessage();

    if (!Error.GetErrorUri().empty()) { ErrorUrl += "&error_uri=" + Error.GetErrorUri(); }

    if (State) { ErrorUrl += "&state=" + State.value(); }

    return ErrorUrl;
}

vector<string> AuthorizationHandler::SplitString(const string& Str, char Delimiter) {
    vector<string> Result;
    stringstream Ss(Str);
    string Item;

    while (getline(Ss, Item, Delimiter)) {
        if (!Item.empty()) { Result.push_back(Item); }
    }

    return Result;
}

Task<void> AuthorizationHandler::HandleRequest(const HTTP::Request& Request,
                                               HTTP::Response& Response, const string& ClientIp) {
    Response.SetHeader("Cache-Control", "no-store");

    // Check rate limiting if enabled
    if (Limiter && !Limiter->CheckRateLimit(ClientIp)) {
        auto Error =
            TooManyRequestsError("You have exceeded the rate limit for authorization requests");
        Response.Status(429);
        Response.JsonResponse(Error.ToResponseObject());
        co_return;
    }

    // Get parameters based on request method
    const auto& Params = (Request.Method == MTHD_POST) ? Request.Body : Request.Query;

    // Phase 1: Validate client_id and redirect_uri
    string ClientId, RedirectUri;
    shared_ptr<OAuthClient> Client;

    try {
        ClientAuthorizationParams ClientParams;
        string ErrorBase;

        if (!ClientAuthorizationParams::Validate(Params, ClientParams, ErrorBase)) {
            throw InvalidRequestError(ErrorBase);
        }

        ClientId = ClientParams.ClientId;

        // TODO: Fix External Ref: Provider->ClientsStore->GetClient
        // Client = co_await Provider->ClientsStore->GetClient(ClientId);
        // For now, simulate the call
        Client = nullptr; // This should be: co_await Provider->GetClient(ClientId);

        if (!Client) { throw InvalidClientError("Invalid client_id"); }

        if (ClientParams.RedirectUri()) {
            RedirectUri = ClientParams.RedirectUri.value();
            auto& RedirectUris = Client->RedirectUris;
            if (find(RedirectUris.begin(), RedirectUris.end(), RedirectUri) == RedirectUris.end()) {
                throw InvalidRequestError("Unregistered redirect_uri");
            }
        } else if (Client->RedirectUris.size() == 1) {
            RedirectUri = Client->RedirectUris[0];
        } else {
            throw InvalidRequestError(
                "redirect_uri must be specified when client has multiple registered URIs");
        }

    } catch (const OAuthError& Error) {
        int Status = (typeid(Error) == typeid(ServerError)) ? 500 : HTTP::Status::BadRequest;
        Response.Status(Status);
        Response.JsonResponse(Error.ToResponseObject());
        co_return;
    } catch (const exception& Error) {
        cout << "Unexpected error looking up client: " << Error.what() << endl;
        auto ServerErr = ServerError("Internal Server Error");
        Response.Status(500);
        Response.JsonResponse(ServerErr.ToResponseObject());
        co_return;
    }

    // Phase 2: Validate other parameters
    optional<string> State;

    try {
        RequestAuthorizationParams RequestParams;
        string ErrorBase;

        if (!RequestAuthorizationParams::Validate(Params, RequestParams, ErrorBase)) {
            throw InvalidRequestError(ErrorBase);
        }

        State = RequestParams.State;
        string CodeChallenge = RequestParams.CodeChallenge;
        optional<string> Scope = RequestParams.Scope;

        // Validate scopes
        vector<string> RequestedScopes;
        if (Scope()) {
            RequestedScopes = SplitString(Scope.value(), ' ');

            unordered_set<string> AllowedScopes;
            if (Client->Scope()) {
                auto ClientScopes = SplitString(Client->Scope.value(), ' ');
                AllowedScopes.insert(ClientScopes.begin(), ClientScopes.end());
            }

            for (const auto& RequestedScope : RequestedScopes) {
                if (AllowedScopes.find(RequestedScope) == AllowedScopes.end()) {
                    throw InvalidScopeError("Client was not registered with scope "
                                            + RequestedScope);
                }
            }
        }

        // All validation passed, proceed with authorization
        AuthorizationRequest AuthReq{.State = State,
                                     .Scopes = RequestedScopes,
                                     .RedirectUri = RedirectUri,
                                     .CodeChallenge = CodeChallenge};

        // TODO: Fix External Ref: Provider->Authorize
        // co_await Provider->Authorize(Client, AuthReq, Response);

    } catch (const OAuthError& Error) {
        string ErrorRedirect = CreateErrorRedirect(RedirectUri, Error, State);
        Response.Redirect(302, ErrorRedirect);
    } catch (const exception& Error) {
        cout << "Unexpected error during authorization: " << Error.what() << endl;
        auto ServerErr = ServerError("Internal Server Error");
        string ErrorRedirect = CreateErrorRedirect(RedirectUri, ServerErr, State);
        Response.Redirect(302, ErrorRedirect);
    }

    co_return;
}

// Factory function to create authorization handler
unique_ptr<AuthorizationHandler>
CreateAuthorizationHandler(const AuthorizationHandlerOptions& Options) {
    return make_unique<AuthorizationHandler>(Options);
}

MCP_NAMESPACE_END