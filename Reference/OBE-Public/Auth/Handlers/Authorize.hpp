#pragma once

#include <chrono>
#include <coroutine>
#include <functional>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <unordered_set>
#include <vector>

#include "../Core/Common.hpp"

MCP_NAMESPACE_BEGIN

// TODO: Fix External Ref: Express RequestHandler equivalent
// TODO: Fix External Ref: OAuthServerProvider
// TODO: Fix External Ref: RateLimit functionality
// TODO: Fix External Ref: AllowedMethods middleware
// TODO: Fix External Ref: OAuth error classes

// Forward declarations for external dependencies
class OAuthServerProvider;
class OAuthError;
class InvalidRequestError;
class InvalidClientError;
class InvalidScopeError;
class ServerError;
class TooManyRequestsError;

// HTTP Request/Response types (simplified for this conversion)
struct HTTP_Request {
    string Method;
    map<string, string> Query;
    map<string, string> Body;
};

struct HTTP_Response {
    int StatusCode = 200;
    map<string, string> Headers;
    JSON ResponseBody;

    void SetHeader(const string& Key, const string& Value) {
        Headers[Key] = Value;
    }

    void Status(int Code) {
        StatusCode = Code;
    }

    void JsonResponse(const JSON& Body) {
        ResponseBody = Body;
    }

    void Redirect(int Code, const string& URL) {
        StatusCode = Code;
        Headers["Location"] = URL;
    }
};

// Rate limit options structure
struct RateLimitOptions {
    chrono::milliseconds WindowMs = chrono::minutes(15);
    int Max = 100;
    bool StandardHeaders = true;
    bool LegacyHeaders = false;
    JSON Message;
};

// Authorization handler options
struct AuthorizationHandlerOptions {
    shared_ptr<OAuthServerProvider> Provider;
    optional<RateLimitOptions> RateLimit; // nullopt means disabled
};

// Client authorization parameters validation
struct ClientAuthorizationParams {
    string ClientId;
    optional<string> RedirectUri;

    static bool Validate(const map<string, string>& Params, ClientAuthorizationParams& Output,
                         string& ErrorMessage) {
        auto ClientIdIt = Params.find("client_id");
        if (ClientIdIt == Params.end() || ClientIdIt->second.empty()) {
            ErrorMessage = "client_id is required";
            return false;
        }
        Output.ClientId = ClientIdIt->second;

        auto RedirectUriIt = Params.find("redirect_uri");
        if (RedirectUriIt != Params.end() && !RedirectUriIt->second.empty()) {
            // Validate URL format
            regex URLPattern(R"(^https?://[^\s]+$)");
            if (!regex_match(RedirectUriIt->second, URLPattern)) {
                ErrorMessage = "redirect_uri must be a valid URL";
                return false;
            }
            Output.RedirectUri = RedirectUriIt->second;
        }

        return true;
    }
};

// Request authorization parameters validation
struct RequestAuthorizationParams {
    string ResponseType;
    string CodeChallenge;
    string CodeChallengeMethod;
    optional<string> Scope;
    optional<string> State;

    static bool Validate(const map<string, string>& Params, RequestAuthorizationParams& Output,
                         string& ErrorMessage) {
        auto ResponseTypeIt = Params.find("response_type");
        if (ResponseTypeIt == Params.end() || ResponseTypeIt->second != MSG_KEY_CODE) {
            ErrorMessage = "response_type must be 'code'";
            return false;
        }
        Output.ResponseType = ResponseTypeIt->second;

        auto CodeChallengeIt = Params.find("code_challenge");
        if (CodeChallengeIt == Params.end() || CodeChallengeIt->second.empty()) {
            ErrorMessage = "code_challenge is required";
            return false;
        }
        Output.CodeChallenge = CodeChallengeIt->second;

        auto CodeChallengeMethodIt = Params.find("code_challenge_method");
        if (CodeChallengeMethodIt == Params.end() || CodeChallengeMethodIt->second != "S256") {
            ErrorMessage = "code_challenge_method must be 'S256'";
            return false;
        }
        Output.CodeChallengeMethod = CodeChallengeMethodIt->second;

        auto ScopeIt = Params.find("scope");
        if (ScopeIt != Params.end() && !ScopeIt->second.empty()) {
            Output.Scope = ScopeIt->second;
        }

        auto StateIt = Params.find("state");
        if (StateIt != Params.end() && !StateIt->second.empty()) {
            Output.State = StateIt->second;
        }

        return true;
    }
};

// Client information structure
struct OAuthClient {
    string ClientId;
    vector<string> RedirectUris;
    optional<string> Scope;
};

// Authorization request structure
struct AuthorizationRequest {
    optional<string> State;
    vector<string> Scopes;
    string RedirectUri;
    string CodeChallenge;
};

// Coroutine return type for async operations
template <typename T> struct Task {
    struct promise_type {
        T Value;
        exception_ptr Exception = nullptr;

        Task<T> get_return_object() {
            return Task<T>{coroutine_handle<promise_type>::from_promise(*this)};
        }

        suspend_never initial_suspend() {
            return {};
        }
        suspend_never final_suspend() noexcept {
            return {};
        }

        void return_value(T Value) {
            this->Value = move(Value);
        }

        void unhandled_exception() {
            Exception = current_exception();
        }
    };

    coroutine_handle<promise_type> Handle;

    Task(coroutine_handle<promise_type> Handle) : Handle(Handle) {}

    ~Task() {
        if (Handle) {
            Handle.destroy();
        }
    }

    T Get() {
        if (Handle.promise().Exception) {
            rethrow_exception(Handle.promise().Exception);
        }
        return Handle.promise().Value;
    }
};

// Rate limiter class
class RateLimiter {
  private:
    RateLimitOptions Options;
    map<string, pair<chrono::steady_clock::time_point, int>> ClientRequests;

  public:
    RateLimiter(const RateLimitOptions& Options) : Options(Options) {}

    bool CheckRateLimit(const string& ClientIp) {
        auto Now = chrono::steady_clock::now();
        auto& [LastWindow, RequestCount] = ClientRequests[ClientIp];

        if (Now - LastWindow > Options.WindowMs) {
            LastWindow = Now;
            RequestCount = 1;
            return true;
        }

        if (RequestCount >= Options.Max) {
            return false;
        }

        RequestCount++;
        return true;
    }
};

// Authorization handler class
class AuthorizationHandler {
  private:
    shared_ptr<OAuthServerProvider> Provider;
    unique_ptr<RateLimiter> Limiter;

    string CreateErrorRedirect(const string& RedirectUri, const OAuthError& Error,
                               const optional<string>& State) {
        string ErrorUrl = RedirectUri;
        char Separator = (RedirectUri.find('?') != string::npos) ? '&' : '?';

        ErrorUrl += Separator;
        ErrorUrl += "error=" + Error.GetErrorCode();
        ErrorUrl += "&error_description=" + Error.GetMessage();

        if (!Error.GetErrorUri().empty()) {
            ErrorUrl += "&error_uri=" + Error.GetErrorUri();
        }

        if (State.has_value()) {
            ErrorUrl += "&state=" + State.value();
        }

        return ErrorUrl;
    }

    vector<string> SplitString(const string& Str, char Delimiter) {
        vector<string> Result;
        stringstream Ss(Str);
        string Item;

        while (getline(Ss, Item, Delimiter)) {
            if (!Item.empty()) {
                Result.push_back(Item);
            }
        }

        return Result;
    }

  public:
    AuthorizationHandler(const AuthorizationHandlerOptions& Options) : Provider(Options.Provider) {
        if (Options.RateLimit.has_value()) {
            Limiter = make_unique<RateLimiter>(Options.RateLimit.value());
        }
    }

    Task<void> HandleRequest(const HTTP_Request& Request, HTTP_Response& Response,
                             const string& ClientIp) {
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
        const auto& Params = (Request.Method == "POST") ? Request.Body : Request.Query;

        // Phase 1: Validate client_id and redirect_uri
        string ClientId, RedirectUri;
        shared_ptr<OAuthClient> Client;

        try {
            ClientAuthorizationParams ClientParams;
            string ErrorMessage;

            if (!ClientAuthorizationParams::Validate(Params, ClientParams, ErrorMessage)) {
                throw InvalidRequestError(ErrorMessage);
            }

            ClientId = ClientParams.ClientId;

            // TODO: Fix External Ref: Provider->ClientsStore->GetClient
            // Client = co_await Provider->ClientsStore->GetClient(ClientId);
            // For now, simulate the call
            Client = nullptr; // This should be: co_await Provider->GetClient(ClientId);

            if (!Client) {
                throw InvalidClientError("Invalid client_id");
            }

            if (ClientParams.RedirectUri.has_value()) {
                RedirectUri = ClientParams.RedirectUri.value();
                auto& RedirectUris = Client->RedirectUris;
                if (find(RedirectUris.begin(), RedirectUris.end(), RedirectUri)
                    == RedirectUris.end()) {
                    throw InvalidRequestError("Unregistered redirect_uri");
                }
            } else if (Client->RedirectUris.size() == 1) {
                RedirectUri = Client->RedirectUris[0];
            } else {
                throw InvalidRequestError(
                    "redirect_uri must be specified when client has multiple registered URIs");
            }

        } catch (const OAuthError& Error) {
            int Status = (typeid(Error) == typeid(ServerError)) ? 500 : 400;
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
            string ErrorMessage;

            if (!RequestAuthorizationParams::Validate(Params, RequestParams, ErrorMessage)) {
                throw InvalidRequestError(ErrorMessage);
            }

            State = RequestParams.State;
            string CodeChallenge = RequestParams.CodeChallenge;
            optional<string> Scope = RequestParams.Scope;

            // Validate scopes
            vector<string> RequestedScopes;
            if (Scope.has_value()) {
                RequestedScopes = SplitString(Scope.value(), ' ');

                unordered_set<string> AllowedScopes;
                if (Client->Scope.has_value()) {
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
};

// Factory function to create authorization handler
unique_ptr<AuthorizationHandler>
CreateAuthorizationHandler(const AuthorizationHandlerOptions& Options) {
    return make_unique<AuthorizationHandler>(Options);
}

MCP_NAMESPACE_END