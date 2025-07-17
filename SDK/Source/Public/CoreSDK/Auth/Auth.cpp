#include "Auth.h"

#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPCredentials.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/StreamCopier.h>
#include <Poco/Timespan.h>
#include <Poco/Timestamp.h>
#include <Poco/URI.h>

#include <algorithm>
#include <mutex>
#include <sstream>
#include <unordered_set>
#include <utility>

MCP_NAMESPACE_BEGIN

static constexpr short DEFAULT_AUTH_SESSION_TIMEOUT{ 30 }; // 30 seconds

// OAuth2AuthProvider Implementation
OAuth2AuthProvider::OAuth2AuthProvider(OAuth2Config InConfig) : m_Config{ std::move(InConfig) }
{
	Poco::URI authURI{ m_Config.AuthServerURL };
	m_AuthSession = std::make_shared<Poco::Net::HTTPClientSession>(authURI.getHost(), authURI.getPort());
	m_AuthSession->setTimeout(Poco::Timespan{ DEFAULT_AUTH_SESSION_TIMEOUT, 0 });
}

OAuth2AuthProvider::~OAuth2AuthProvider() noexcept = default;

Task<bool> OAuth2AuthProvider::ValidateToken(const std::string& InToken)
{
	try
	{
		// Check cache first
		{
			std::lock_guard lock(m_CacheMutex);
			if (auto iter = m_TokenCache.find(InToken); iter != m_TokenCache.end())
			{
				Poco::Timestamp now;

				if (Poco::Timespan age = now - iter->second.CachedAt; age < m_Config.TokenCacheTimeout)
				{
					co_return iter->second.Result.IsAuthorized;
				}

				// Cache expired, remove entry
				m_TokenCache.erase(iter);
			}
		}

		// Validate with auth server
		auto tokenInfo = co_await ValidateTokenWithAuthServer(InToken);
		bool isValid = tokenInfo.contains("active") && tokenInfo["active"].get<bool>();

		// Cache result
		if (isValid)
		{
			std::lock_guard lock(m_CacheMutex);
			CachedToken cached{};
			cached.Result.IsAuthorized = true;
			cached.Result.ClientID = tokenInfo.value("client_id", "");
			if (tokenInfo.contains("scope"))
			{
				auto scope = tokenInfo["scope"].get<std::string>();
				// Split scope string into vector
				std::istringstream scopeStream{ scope };
				std::string item;
				while (std::getline(scopeStream, item, ' '))
				{
					if (!item.empty())
					{
						cached.Result.Scopes.push_back(item);
					}
				}
			}
			cached.CachedAt = Poco::Timestamp{};
			m_TokenCache[InToken] = cached;
		}

		co_return isValid;
	}
	catch (const std::exception& Except)
	{
		(void)Except;
		co_return false;
	}
}

Task<AuthResult> OAuth2AuthProvider::AuthorizeRequest(const std::string& InMethod, const std::string& InToken)
{
	AuthResult result{};

	try
	{
		// Check if method is public (doesn't require auth)
		if (AuthUtils::IsPublicMethod(InMethod))
		{
			result.IsAuthorized = true;
			co_return result;
		}

		// Validate token
		if (const bool isValid = co_await ValidateToken(InToken); !isValid)
		{
			result.ErrorMessage = "Invalid or expired token";
			co_return result;
		}

		// Get cached token info
		{
			std::lock_guard lock(m_CacheMutex);
			if (const auto iter = m_TokenCache.find(InToken); iter != m_TokenCache.end())
			{
				result = iter->second.Result;
			}
		}

		// Check if token has required scopes for the method
		if (const auto requiredScopes = AuthUtils::GetRequiredScopes(InMethod); !requiredScopes.empty())
		{
			const std::unordered_set<std::string> clientScopes(result.Scopes.begin(), result.Scopes.end());
			bool hasRequiredScope = false;
			for (const auto& requiredScope : requiredScopes)
			{
				if (clientScopes.contains(requiredScope))
				{
					hasRequiredScope = true;
					break;
				}
			}

			if (!hasRequiredScope)
			{
				result.IsAuthorized = false;
				result.ErrorMessage = Poco::format("Insufficient scope for method: {}", InMethod);
				co_return result;
			}
		}

		result.IsAuthorized = true;
		co_return result;
	}
	catch (const std::exception& e)
	{
		result.ErrorMessage = Poco::format("Authorization error: {}", e.what());
		co_return result;
	}
}

Task<JSONData> OAuth2AuthProvider::ValidateTokenWithAuthServer(const std::string& InToken)
{
	try
	{
		Poco::URI authURI{ m_Config.AuthServerURL + "/oauth/introspect" };

		Poco::Net::HTTPRequest request{ Poco::Net::HTTPRequest::HTTP_POST, authURI.getPathAndQuery() };
		request.setContentType("application/x-www-form-urlencoded");

		// Add an authorization header using Poco HTTPCredentials for proper Basic auth
		Poco::Net::HTTPCredentials credentials{ m_Config.ClientID, m_Config.ClientSecret };
		Poco::Net::HTTPResponse dummyResponse{}; // Needed for authenticate method signature
		credentials.authenticate(request, dummyResponse);

		std::string body = "token=" + InToken;
		request.setContentLength(body.length());

		std::ostream& requestStream = m_AuthSession->sendRequest(request);
		requestStream << body;

		Poco::Net::HTTPResponse response;
		std::istream& responseStream = m_AuthSession->receiveResponse(response);

		if (response.getStatus() != Poco::Net::HTTPResponse::HTTP_OK)
		{
			throw std::runtime_error(Poco::format("Token validation failed with status: {}", response.getStatus()));
		}

		std::string responseBody;
		Poco::StreamCopier::copyToString(responseStream, responseBody);

		co_return JSONData::parse(responseBody);
	}
	catch (const std::exception& e)
	{
		throw std::runtime_error(Poco::format("Token validation error: {}", e.what()));
	}
}

bool OAuth2AuthProvider::IsMethodAllowed(const std::string& InMethod, const std::vector<std::string>& InScopes)
{
	const auto RequiredScopes = AuthUtils::GetRequiredScopes(InMethod);

	for (const auto& requiredScope : RequiredScopes)
	{
		if (std::ranges::find(InScopes, requiredScope) != InScopes.end())
		{
			return true;
		}
	}

	return RequiredScopes.empty(); // Allow if no specific scopes required
}

// BearerTokenAuthProvider Implementation
BearerTokenAuthProvider::BearerTokenAuthProvider(
	const std::unordered_map<std::string, std::vector<std::string>>& InValidTokens)
	: m_ValidTokens{ InValidTokens }
{}

Task<bool> BearerTokenAuthProvider::ValidateToken(const std::string& InToken)
{
	co_return m_ValidTokens.contains(InToken);
}

Task<AuthResult> BearerTokenAuthProvider::AuthorizeRequest(const std::string& InMethod, const std::string& InToken)
{
	AuthResult result{};

	try
	{
		// Check if method is public
		if (AuthUtils::IsPublicMethod(InMethod))
		{
			result.IsAuthorized = true;
			co_return result;
		}

		// Check if token exists
		const auto iter = m_ValidTokens.find(InToken);
		if (iter == m_ValidTokens.end())
		{
			result.ErrorMessage = "Invalid token";
			co_return result;
		}

		result.IsAuthorized = true;
		result.ClientID = "bearer_client";
		result.Scopes = iter->second;

		// Check scopes
		if (const auto requiredScopes = AuthUtils::GetRequiredScopes(InMethod); !requiredScopes.empty())
		{
			const std::unordered_set<std::string> clientScopes(result.Scopes.begin(), result.Scopes.end());
			bool hasRequiredScope = false;
			for (const auto& requiredScope : requiredScopes)
			{
				if (clientScopes.contains(requiredScope))
				{
					hasRequiredScope = true;
					break;
				}
			}

			if (!hasRequiredScope)
			{
				result.IsAuthorized = false;
				result.ErrorMessage = Poco::format("Insufficient scope for method: {}", InMethod);
			}
		}

		co_return result;
	}
	catch (const std::exception& e)
	{
		result.ErrorMessage = Poco::format("Authorization error: {}", e.what());
		co_return result;
	}
}

// AuthUtils Implementation
std::optional<std::string> AuthUtils::ExtractBearerToken(const Poco::Net::HTTPServerRequest& InRequest)
{
	const std::string authHeader = InRequest.get("Authorization", "");

	if (constexpr std::string_view bearerPrefix = "Bearer "; authHeader.starts_with(bearerPrefix))
	{
		return authHeader.substr(bearerPrefix.length());
	}

	return std::nullopt;
}

bool AuthUtils::IsPublicMethod(const std::string& InMethod)
{
	// These methods don't require authentication
	static const std::unordered_set<std::string> publicMethods
		= { "initialize", "initialized", "ping", "capabilities" };

	return publicMethods.contains(InMethod);
}

std::vector<std::string> AuthUtils::GetRequiredScopes(const std::string& InMethod)
{
	// Define scope requirements for different methods
	static const std::unordered_map<std::string, std::vector<std::string>> methodScopes
		= { { "tools/list", { "tools:read" } },
			  { "tools/call", { "tools:execute" } },
			  { "prompts/list", { "prompts:read" } },
			  { "prompts/get", { "prompts:read" } },
			  { "resources/list", { "resources:read" } },
			  { "resources/read", { "resources:read" } },
			  { "resources/subscribe", { "resources:subscribe" } },
			  { "resources/unsubscribe", { "resources:subscribe" } },
			  { "sampling/createMessage", { "sampling:create" } },
			  { "completion/complete", { "completion:read" } } };

	if (const auto iter = methodScopes.find(InMethod); iter != methodScopes.end())
	{
		return iter->second;
	}

	return {}; // No specific scopes required
}

MCP_NAMESPACE_END