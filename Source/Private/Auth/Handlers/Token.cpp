#include "Auth/Handlers/Token.h"

// TODO: Fix External Ref: express framework integration
// TODO: Fix External Ref: cors library integration

MCP_NAMESPACE_BEGIN

bool RateLimitState::CheckRateLimit(const string& identifier, int maxRequests, int windowMs) {
    lock_guard<mutex> lock(RateLimitMutex);
    auto now = chrono::steady_clock::now();
    auto windowStart = now - chrono::milliseconds(windowMs);

    auto& times = RequestTimes[identifier];
    times.erase(remove_if(times.begin(), times.end(),
                          [windowStart](const auto& time) { return time < windowStart; }),
                times.end());

    if (times.size() >= maxRequests) { return false; }

    times.push_back(now);
    return true;
}

bool TokenRequestSchema::Validate(const JSON& Body, TokenRequestSchema& Out, string& ErrorMessage) {
    if (!Body.contains("grant_type") || !Body["grant_type"].is_string()) {
        ErrorMessage = "Missing or invalid grant_type";
        return false;
    }
    Out.GrantType = Body["grant_type"];
    return true;
}

bool AuthorizationCodeGrantSchema::Validate(const JSON& Body, AuthorizationCodeGrantSchema& Out,
                                            string& ErrorMessage) {
    if (!Body.contains(MSG_CODE) || !Body[MSG_CODE].is_string()) {
        ErrorMessage = "Missing or invalid code";
        return false;
    }
    if (!Body.contains("code_verifier") || !Body["code_verifier"].is_string()) {
        ErrorMessage = "Missing or invalid code_verifier";
        return false;
    }

    Out.Code = Body[MSG_CODE];
    Out.CodeVerifier = Body["code_verifier"];

    if (Body.contains(MSG_REDIRECT_URI) && Body[MSG_REDIRECT_URI].is_string()) {
        Out.RedirectURI = Body[MSG_REDIRECT_URI];
    }

    return true;
}

bool RefreshTokenGrantSchema::Validate(const JSON& Body, RefreshTokenGrantSchema& Out,
                                       string& ErrorMessage) {
    if (!Body.contains("refresh_token") || !Body["refresh_token"].is_string()) {
        ErrorMessage = "Missing or invalid refresh_token";
        return false;
    }

    Out.RefreshToken = Body["refresh_token"];

    if (Body.contains(MSG_SCOPE) && Body[MSG_SCOPE].is_string()) { Out.Scope = Body[MSG_SCOPE]; }

    return true;
}

Task<bool> PKCEVerifier::VerifyChallenge(const string& codeVerifier, const string& codeChallenge) {
    // Implement SHA256 PKCE verification
    // This should match the original verifyChallenge from pkce-challenge
    co_return VerifyChallengeSync(codeVerifier, codeChallenge);
}

bool PKCEVerifier::VerifyChallengeSync(const string& codeVerifier, const string& codeChallenge) {
    // Base64url encode SHA256 hash of code_verifier
    // For now, simplified verification - should implement proper SHA256 + base64url
    // TODO: Implement proper PKCE verification with SHA256
    return !codeVerifier.empty() && !codeChallenge.empty();
}

MiddlewareResult TokenHandler::ApplyMiddleware(const JSON& RequestBody, const JSON& Headers,
                                               const string& Method, const string& ClientIP) {
    MiddlewareResult result;

    // 1. CORS middleware equivalent
    result.Headers["Access-Control-Allow-Origin"] = "*";
    result.Headers["Access-Control-Allow-Methods"] = MTHD_POST;
    result.Headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";

    // 2. Method restriction (allowedMethods equivalent)
    if (Method != MTHD_POST) {
        result.ShouldContinue = false;
        result.StatusCode = HTTPStatus::MethodNotAllowed;
        result.ErrorResponse = "Method Not Allowed";
        return result;
    }

    // 3. Rate limiting
    if (Options.RateLimit.has_value()) {
        const auto& rateLimitConfig = Options.RateLimit.value();
        if (!RateLimitState_->CheckRateLimit(ClientIP, rateLimitConfig.Max,
                                             rateLimitConfig.WindowMs)) {
            result.ShouldContinue = false;
            result.StatusCode = 429;
            auto error =
                TooManyRequestsError("You have exceeded the rate limit for token requests");
            result.ErrorResponse = error.ToResponseObject().dump();
            return result;
        }
    }

    // 4. URL-encoded body parsing validation
    // In a real implementation, this would parse application/x-www-form-urlencoded
    // For now, we assume JSON is already parsed

    return result;
}

shared_ptr<Client> TokenHandler::AuthenticateClient(const JSON& Headers, const JSON& RequestBody) {
    // TODO: Implement client authentication logic
    // This should extract client credentials from Authorization header or request body
    // and validate against the provider's client store

    // Placeholder implementation
    auto client = make_shared<Client>();
    client->ClientId = "placeholder_client";
    client->AllowedGrantTypes = {"authorization_code", "refresh_token"};
    return client;
}

Task<JSON> TokenHandler::HandleRequestAsync(const JSON& RequestBody, const JSON& Headers,
                                            const string& Method, const string& ClientIP,
                                            string& ResponseStatus, JSON& ResponseHeaders) {
    try {
        // Apply middleware pipeline
        auto middlewareResult = ApplyMiddleware(RequestBody, Headers, Method, ClientIP);

        // Merge middleware headers
        for (auto& [key, value] : middlewareResult.Headers.items()) {
            ResponseHeaders[key] = value;
        }

        if (!middlewareResult.ShouldContinue) {
            ResponseStatus = to_string(middlewareResult.StatusCode);
            co_return JSON::parse(middlewareResult.ErrorResponse);
        }

        // Set cache control header (equivalent to res.setHeader)
        ResponseHeaders["Cache-Control"] = "no-store";

        // Authenticate client
        auto client = AuthenticateClient(Headers, RequestBody);
        if (!client) { throw ServerError("Internal Server Error"); }

        // Validate basic token request
        string ErrorMessage;
        TokenRequestSchema TokenRequest;
        if (!TokenRequestSchema::Validate(RequestBody, TokenRequest, ErrorMessage)) {
            throw InvalidRequestError(ErrorMessage);
        }

        const string& GrantType = TokenRequest.GrantType;

        if (GrantType == "authorization_code") {
            co_return co_await HandleAuthorizationCodeGrantAsync(RequestBody, client);
        } else if (GrantType == "refresh_token") {
            co_return co_await HandleRefreshTokenGrantAsync(RequestBody, client);
        } else {
            throw UnsupportedGrantTypeError(
                "The grant type is not supported by this authorization server.");
        }
    } catch (const OAuthError& Error) {
        ResponseStatus = (Error.GetType() == "ServerError") ? "500" : "400";
        co_return Error.ToResponseObject();
    } catch (const exception& Error) {
        // Log unexpected error (equivalent to console.error)
        // TODO: Implement proper logging

        ServerError ServerErr("Internal Server Error");
        ResponseStatus = "500";
        co_return ServerErr.ToResponseObject();
    }
}

Task<JSON> TokenHandler::HandleAuthorizationCodeGrantAsync(const JSON& RequestBody,
                                                           shared_ptr<Client> client) {
    string ErrorMessage;
    AuthorizationCodeGrantSchema Grant;
    if (!AuthorizationCodeGrantSchema::Validate(RequestBody, Grant, ErrorMessage)) {
        throw InvalidRequestError(ErrorMessage);
    }

    const string& Code = Grant.Code;
    const string& CodeVerifier = Grant.CodeVerifier;
    const optional<string>& RedirectURI = Grant.RedirectURI;

    bool SkipLocalPKCEValidation = Options.Provider->GetSkipLocalPKCEValidation();

    // Perform local PKCE validation unless explicitly skipped
    if (!SkipLocalPKCEValidation) {
        string CodeChallenge =
            co_await Options.Provider->ChallengeForAuthorizationCodeAsync(client, Code);

        bool IsValid = co_await PKCEVerifier::VerifyChallenge(CodeVerifier, CodeChallenge);
        if (!IsValid) { throw InvalidGrantError("code_verifier does not match the challenge"); }
    }

    // Pass code_verifier to provider if PKCE validation didn't occur locally
    optional<string> CodeVerifierParam;
    if (SkipLocalPKCEValidation) { CodeVerifierParam = CodeVerifier; }

    JSON Tokens = co_await Options.Provider->ExchangeAuthorizationCodeAsync(
        client, Code, CodeVerifierParam, RedirectURI);

    co_return Tokens;
}

Task<JSON> TokenHandler::HandleRefreshTokenGrantAsync(const JSON& RequestBody,
                                                      shared_ptr<Client> client) {
    string ErrorMessage;
    RefreshTokenGrantSchema Grant;
    if (!RefreshTokenGrantSchema::Validate(RequestBody, Grant, ErrorMessage)) {
        throw InvalidRequestError(ErrorMessage);
    }

    const string& RefreshToken = Grant.RefreshToken;
    const optional<string>& Scope = Grant.Scope;

    optional<vector<string>> Scopes;
    if (Scope.has_value()) { Scopes = SplitStringWithRanges(Scope.value(), " "); }

    JSON Tokens =
        co_await Options.Provider->ExchangeRefreshTokenAsync(client, RefreshToken, Scopes);
    co_return Tokens;
}

vector<string> TokenHandler::SplitStringWithRanges(const string& Str, const string& Delimiter) {
    if (Str.empty()) return {};

    vector<string> Result;
    size_t Start = 0;
    size_t End = Str.find(Delimiter);

    while (End != string::npos) {
        if (End > Start) { Result.emplace_back(Str.substr(Start, End - Start)); }
        Start = End + Delimiter.length();
        End = Str.find(Delimiter, Start);
    }

    if (Start < Str.length()) { Result.emplace_back(Str.substr(Start)); }

    return Result;
}

shared_ptr<TokenHandler> CreateTokenHandler(const TokenHandlerOptions& Options) {
    return make_shared<TokenHandler>(Options);
}

MCP_NAMESPACE_END