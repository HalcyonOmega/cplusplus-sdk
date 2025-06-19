#pragma once

#include <Poco/Net/HTTPCredentials.h>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "MCPTask.h"
#include "Macros.h"
#include "Poco/Timespan.h"
#include "Poco/Timestamp.h"
#include "json.hpp"

// TODO: @HalcyonOmega - Is this using proper Poco auth?

// Forward declarations
namespace Poco::Net {
class HTTPClientSession;
class HTTPServerRequest;
} // namespace Poco::Net

MCP_NAMESPACE_BEGIN

// Authorization result
struct AuthResult {
    bool IsAuthorized = false;
    std::string ClientID;
    std::vector<std::string> Scopes;
    std::string ErrorMessage;
};

// OAuth 2.1 configuration
struct OAuth2Config {
    std::string AuthServerURL;
    std::string ClientID;
    std::string ClientSecret;
    std::string Scope;
    bool ValidateSSL = true;
    Poco::Timespan TokenCacheTimeout{5 * 60, 0}; // 5 minutes
};

// Base authorization provider interface
class MCPAuthProvider {
  public:
    virtual ~MCPAuthProvider() = default;
    virtual MCPTask<bool> ValidateToken(const std::string& InToken) = 0;
    virtual MCPTask<AuthResult> AuthorizeRequest(const std::string& InMethod,
                                                 const std::string& InToken) = 0;
};

// OAuth 2.1 authorization provider
class OAuth2AuthProvider : public MCPAuthProvider {
  public:
    explicit OAuth2AuthProvider(const OAuth2Config& InConfig);
    ~OAuth2AuthProvider() override;

    MCPTask<bool> ValidateToken(const std::string& InToken) override;
    MCPTask<AuthResult> AuthorizeRequest(const std::string& InMethod,
                                         const std::string& InToken) override;

  private:
    MCPTask<nlohmann::json> ValidateTokenWithAuthServer(const std::string& InToken);
    bool IsMethodAllowed(const std::string& InMethod, const std::vector<std::string>& InScopes);

    OAuth2Config m_Config;
    std::shared_ptr<Poco::Net::HTTPClientSession> m_AuthSession;

    // Token cache for performance
    struct CachedToken {
        AuthResult Result;
        Poco::Timestamp CachedAt;
    };
    std::unordered_map<std::string, CachedToken> m_TokenCache;
    mutable std::mutex m_CacheMutex;
};

// Simple Bearer token provider (for development/testing)
class BearerTokenAuthProvider : public MCPAuthProvider {
  public:
    explicit BearerTokenAuthProvider(
        const std::unordered_map<std::string, std::vector<std::string>>& InValidTokens);

    MCPTask<bool> ValidateToken(const std::string& InToken) override;
    MCPTask<AuthResult> AuthorizeRequest(const std::string& InMethod,
                                         const std::string& InToken) override;

  private:
    std::unordered_map<std::string, std::vector<std::string>> m_ValidTokens; // token -> scopes
};

// Authorization utilities
class AuthUtils {
  public:
    static std::optional<std::string> ExtractBearerToken(Poco::Net::HTTPServerRequest& InRequest);
    static bool IsPublicMethod(const std::string& InMethod);
    static std::vector<std::string> GetRequiredScopes(const std::string& InMethod);
};

MCP_NAMESPACE_END