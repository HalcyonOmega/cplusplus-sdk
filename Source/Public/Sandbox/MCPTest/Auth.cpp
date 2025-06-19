#include "Auth.h"

#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/StreamCopier.h>
#include <Poco/URI.h>

#include <algorithm>
#include <chrono>
#include <mutex>
#include <sstream>

// OAuth2AuthProvider Implementation
OAuth2AuthProvider::OAuth2AuthProvider(const OAuth2Config& InConfig) : m_Config(InConfig) {
    Poco::URI authURI(m_Config.AuthServerURL);
    m_AuthSession =
        std::make_shared<Poco::Net::HTTPSClientSession>(authURI.getHost(), authURI.getPort());
    m_AuthSession->setTimeout(Poco::Timespan(30, 0)); // 30 seconds
}

OAuth2AuthProvider::~OAuth2AuthProvider() = default;

MCPTask<bool> OAuth2AuthProvider::ValidateToken(const std::string& InToken) {
    try {
        // Check cache first
        {
            std::lock_guard<std::mutex> lock(m_CacheMutex);
            auto iter = m_TokenCache.find(InToken);
            if (iter != m_TokenCache.end()) {
                auto now = std::chrono::steady_clock::now();
                auto age =
                    std::chrono::duration_cast<std::chrono::seconds>(now - iter->second.CachedAt);

                if (age < m_Config.TokenCacheTimeout) {
                    co_return iter->second.Result.IsAuthorized;
                } else {
                    // Cache expired, remove entry
                    m_TokenCache.erase(iter);
                }
            }
        }

        // Validate with auth server
        auto tokenInfo = co_await ValidateTokenWithAuthServer(InToken);
        bool isValid = tokenInfo.contains("active") && tokenInfo["active"].get<bool>();

        // Cache result
        if (isValid) {
            std::lock_guard<std::mutex> lock(m_CacheMutex);
            CachedToken cached;
            cached.Result.IsAuthorized = true;
            cached.Result.ClientID = tokenInfo.value("client_id", "");
            if (tokenInfo.contains("scope")) {
                std::string scope = tokenInfo["scope"].get<std::string>();
                // Split scope string into vector
                std::istringstream scopeStream(scope);
                std::string item;
                while (std::getline(scopeStream, item, ' ')) {
                    if (!item.empty()) { cached.Result.Scopes.push_back(item); }
                }
            }
            cached.CachedAt = std::chrono::steady_clock::now();
            m_TokenCache[InToken] = cached;
        }

        co_return isValid;
    } catch (const std::exception& e) { co_return false; }
}

MCPTask<AuthResult> OAuth2AuthProvider::AuthorizeRequest(const std::string& InMethod,
                                                         const std::string& InToken) {
    AuthResult result;

    try {
        // Check if method is public (doesn't require auth)
        if (AuthUtils::IsPublicMethod(InMethod)) {
            result.IsAuthorized = true;
            co_return result;
        }

        // Validate token
        bool isValid = co_await ValidateToken(InToken);
        if (!isValid) {
            result.ErrorMessage = "Invalid or expired token";
            co_return result;
        }

        // Get cached token info
        {
            std::lock_guard<std::mutex> lock(m_CacheMutex);
            auto iter = m_TokenCache.find(InToken);
            if (iter != m_TokenCache.end()) { result = iter->second.Result; }
        }

        // Check if token has required scopes for the method
        auto requiredScopes = AuthUtils::GetRequiredScopes(InMethod);
        if (!requiredScopes.empty()) {
            bool hasRequiredScope = false;
            for (const auto& requiredScope : requiredScopes) {
                if (std::find(result.Scopes.begin(), result.Scopes.end(), requiredScope)
                    != result.Scopes.end()) {
                    hasRequiredScope = true;
                    break;
                }
            }

            if (!hasRequiredScope) {
                result.IsAuthorized = false;
                result.ErrorMessage = "Insufficient scope for method: " + InMethod;
                co_return result;
            }
        }

        result.IsAuthorized = true;
        co_return result;

    } catch (const std::exception& e) {
        result.ErrorMessage = "Authorization error: " + std::string(e.what());
        co_return result;
    }
}

MCPTask<nlohmann::json>
OAuth2AuthProvider::ValidateTokenWithAuthServer(const std::string& InToken) {
    try {
        Poco::URI authURI(m_Config.AuthServerURL + "/oauth/introspect");

        Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_POST,
                                       authURI.getPathAndQuery());
        request.setContentType("application/x-www-form-urlencoded");

        // Add authorization header for client credentials
        std::string credentials = m_Config.ClientID + ":" + m_Config.ClientSecret;
        // In production, use proper base64 encoding
        request.set("Authorization", "Basic " + credentials);

        std::string body = "token=" + InToken;
        request.setContentLength(body.length());

        std::ostream& requestStream = m_AuthSession->sendRequest(request);
        requestStream << body;

        Poco::Net::HTTPResponse response;
        std::istream& responseStream = m_AuthSession->receiveResponse(response);

        if (response.getStatus() != Poco::Net::HTTPResponse::HTTP_OK) {
            throw std::runtime_error("Token validation failed with status: "
                                     + std::to_string(response.getStatus()));
        }

        std::string responseBody;
        Poco::StreamCopier::copyToString(responseStream, responseBody);

        co_return nlohmann::json::parse(responseBody);

    } catch (const std::exception& e) {
        throw std::runtime_error("Token validation error: " + std::string(e.what()));
    }
}

bool OAuth2AuthProvider::IsMethodAllowed(const std::string& InMethod,
                                         const std::vector<std::string>& InScopes) {
    auto requiredScopes = AuthUtils::GetRequiredScopes(InMethod);

    for (const auto& requiredScope : requiredScopes) {
        if (std::find(InScopes.begin(), InScopes.end(), requiredScope) != InScopes.end()) {
            return true;
        }
    }

    return requiredScopes.empty(); // Allow if no specific scopes required
}

// BearerTokenAuthProvider Implementation
BearerTokenAuthProvider::BearerTokenAuthProvider(
    const std::unordered_map<std::string, std::vector<std::string>>& InValidTokens)
    : m_ValidTokens(InValidTokens) {}

MCPTask<bool> BearerTokenAuthProvider::ValidateToken(const std::string& InToken) {
    co_return m_ValidTokens.find(InToken) != m_ValidTokens.end();
}

MCPTask<AuthResult> BearerTokenAuthProvider::AuthorizeRequest(const std::string& InMethod,
                                                              const std::string& InToken) {
    AuthResult result;

    try {
        // Check if method is public
        if (AuthUtils::IsPublicMethod(InMethod)) {
            result.IsAuthorized = true;
            co_return result;
        }

        // Check if token exists
        auto iter = m_ValidTokens.find(InToken);
        if (iter == m_ValidTokens.end()) {
            result.ErrorMessage = "Invalid token";
            co_return result;
        }

        result.IsAuthorized = true;
        result.ClientID = "bearer_client";
        result.Scopes = iter->second;

        // Check scopes
        auto requiredScopes = AuthUtils::GetRequiredScopes(InMethod);
        if (!requiredScopes.empty()) {
            bool hasRequiredScope = false;
            for (const auto& requiredScope : requiredScopes) {
                if (std::find(result.Scopes.begin(), result.Scopes.end(), requiredScope)
                    != result.Scopes.end()) {
                    hasRequiredScope = true;
                    break;
                }
            }

            if (!hasRequiredScope) {
                result.IsAuthorized = false;
                result.ErrorMessage = "Insufficient scope for method: " + InMethod;
            }
        }

        co_return result;

    } catch (const std::exception& e) {
        result.ErrorMessage = "Authorization error: " + std::string(e.what());
        co_return result;
    }
}

// AuthUtils Implementation
std::optional<std::string> AuthUtils::ExtractBearerToken(Poco::Net::HTTPServerRequest& InRequest) {
    std::string authHeader = InRequest.get("Authorization", "");

    if (authHeader.substr(0, 7) == "Bearer ") { return authHeader.substr(7); }

    return std::nullopt;
}

bool AuthUtils::IsPublicMethod(const std::string& InMethod) {
    // These methods don't require authentication
    static const std::unordered_set<std::string> publicMethods = {"initialize", "initialized",
                                                                  "ping", "capabilities"};

    return publicMethods.find(InMethod) != publicMethods.end();
}

std::vector<std::string> AuthUtils::GetRequiredScopes(const std::string& InMethod) {
    // Define scope requirements for different methods
    static const std::unordered_map<std::string, std::vector<std::string>> methodScopes = {
        {"tools/list", {"tools:read"}},
        {"tools/call", {"tools:execute"}},
        {"prompts/list", {"prompts:read"}},
        {"prompts/get", {"prompts:read"}},
        {"resources/list", {"resources:read"}},
        {"resources/read", {"resources:read"}},
        {"resources/subscribe", {"resources:subscribe"}},
        {"resources/unsubscribe", {"resources:subscribe"}},
        {"sampling/createMessage", {"sampling:create"}},
        {"completion/complete", {"completion:read"}}};

    auto iter = methodScopes.find(InMethod);
    if (iter != methodScopes.end()) { return iter->second; }

    return {}; // No specific scopes required
}