#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// TODO: Fix External Ref: Express Router functionality
// TODO: Fix External Ref: HTTP Request/Response handling
// TODO: Fix External Ref: Client registration handler
// TODO: Fix External Ref: Token handler
// TODO: Fix External Ref: Authorization handler
// TODO: Fix External Ref: Revocation handler
// TODO: Fix External Ref: Metadata handler

// Forward declarations for handlers
struct ClientRegistrationHandlerOptions;
struct TokenHandlerOptions;
struct AuthorizationHandlerOptions;
struct RevocationHandlerOptions;
struct OAuthServerProvider;

// Basic HTTP request handler type
using RequestHandler = function<void(const JSON&, JSON&)>;

// Robust URL helper class for C++ equivalent of JavaScript URL class
class URLHelper {
  public:
    string Href;
    string Protocol;
    string Hostname;
    string Port;
    string Pathname;
    string Search;
    string Hash;

    URLHelper(const string& URLString) {
        ParseURL(URLString);
    }

    URLHelper(const string& Path, const URLHelper& Base) {
        // Handle absolute URLs
        if (Path.starts_with("http://") || Path.starts_with("https://")) {
            ParseURL(Path);
            return;
        }

        // Handle protocol-relative URLs
        if (Path.starts_with("//")) {
            ParseURL(Base.Protocol + Path);
            return;
        }

        // Handle absolute paths
        if (Path.starts_with("/")) {
            string BaseURL = Base.Protocol + "//" + Base.Hostname;
            if (!Base.Port.empty()) { BaseURL += ":" + Base.Port; }
            ParseURL(BaseURL + Path);
            return;
        }

        // Handle relative paths
        string BaseURL = Base.Protocol + "//" + Base.Hostname;
        if (!Base.Port.empty()) { BaseURL += ":" + Base.Port; }

        string BasePath = Base.Pathname;
        if (!BasePath.ends_with("/")) {
            // Remove filename from base path
            size_t LastSlash = BasePath.find_last_of('/');
            if (LastSlash != string::npos) {
                BasePath = BasePath.substr(0, LastSlash + 1);
            } else {
                BasePath = "/";
            }
        }

        ParseURL(BaseURL + BasePath + Path);
    }

  private:
    void ParseURL(const string& URLString);
};

struct AuthRouterOptions {
    /**
     * A provider implementing the actual authorization logic for this router.
     */
    shared_ptr<OAuthServerProvider> Provider;

    /**
     * The authorization server's issuer identifier, which is a URL that uses the "https" scheme and
     * has no query or fragment components.
     */
    URLHelper IssuerURL;

    /**
     * The base URL of the authorization server to use for the metadata endpoints.
     *
     * If not provided, the issuer URL will be used as the base URL.
     */
    optional<URLHelper> BaseURL;

    /**
     * An optional URL of a page containing human-readable information that developers might want or
     * need to know when using the authorization server.
     */
    optional<URLHelper> ServiceDocumentationURL;

    /**
     * An optional list of scopes supported by this authorization server
     */
    optional<vector<string>> ScopesSupported;

    /**
     * The resource name to be displayed in protected resource metadata
     */
    optional<string> ResourceName;

    // Individual options per route - TODO: Define these option structs
    optional<AuthorizationHandlerOptions> AuthorizationOptions;
    optional<ClientRegistrationHandlerOptions> ClientRegistrationOptions;
    optional<RevocationHandlerOptions> RevocationOptions;
    optional<TokenHandlerOptions> TokenOptions;
};

void CheckIssuerURL(const URLHelper& Issuer);

struct CreateOAuthMetadataOptions {
    shared_ptr<OAuthServerProvider> Provider;
    URLHelper IssuerURL;
    optional<URLHelper> BaseURL;
    optional<URLHelper> ServiceDocumentationURL;
    optional<vector<string>> ScopesSupported;
};

// Helper function to check if a method is implemented by checking if it returns a default/null
// response
bool HasClientRegistrationSupport(const shared_ptr<OAuthServerProvider>& Provider);

bool HasTokenRevocationSupport(const shared_ptr<OAuthServerProvider>& Provider);

OAuthMetadata CreateOAuthMetadata(const CreateOAuthMetadataOptions& Options);

/**
 * Express Router equivalent - handles HTTP routing
 */
class ExpressRouter {
  private:
    unordered_map<string, RequestHandler> Routes;
    vector<RequestHandler> Middleware;

  public:
    void Use(const string& Path, RequestHandler Handler);

    void Use(RequestHandler Handler);

    RequestHandler CreateHandler();
};

/**
 * Installs standard MCP authorization server endpoints, including dynamic client registration and
 * token revocation (if supported). Also advertises standard authorization server metadata, for
 * easier discovery of supported configurations by clients. Note: if your MCP server is only a
 * resource server and not an authorization server, use MCP_AuthMetadataRouter instead.
 *
 * By default, rate limiting is applied to all endpoints to prevent abuse.
 *
 * This router MUST be installed at the application root, like so:
 *
 *  const app = express();
 *  app.use(MCP_AuthRouter(...));
 */
RequestHandler MCP_AuthRouter(const AuthRouterOptions& Options);

struct AuthMetadataOptions {
    /**
     * OAuth Metadata as would be returned from the authorization server
     * this MCP server relies on
     */
    OAuthMetadata OAuthMetadata;

    /**
     * The url of the MCP server, for use in protected resource metadata
     */
    URLHelper ResourceServerURL;

    /**
     * The url for documentation for the MCP server
     */
    optional<URLHelper> ServiceDocumentationURL;

    /**
     * An optional list of scopes supported by this MCP server
     */
    optional<vector<string>> ScopesSupported;

    /**
     * An optional resource name to display in resource metadata
     */
    optional<string> ResourceName;
};

// Helper to create metadata handler
RequestHandler MetadataHandler(const JSON& Metadata);

RequestHandler MCP_AuthMetadataRouter(const AuthMetadataOptions& Options);

/**
 * Helper function to construct the OAuth 2.0 Protected Resource Metadata URL
 * from a given server URL. This replaces the path with the standard metadata endpoint.
 *
 * @param ServerURL - The base URL of the protected resource server
 * @returns The URL for the OAuth protected resource metadata endpoint
 *
 * @example
 * GetOAuthProtectedResourceMetadataURL(URLHelper('https://api.example.com/mcp'))
 * // Returns: 'https://api.example.com/.well-known/oauth-protected-resource'
 */
string GetOAuthProtectedResourceMetadataURL(const URLHelper& ServerURL);

MCP_NAMESPACE_END
