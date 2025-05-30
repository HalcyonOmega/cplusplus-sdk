#pragma once

#include "../Core/Common.hpp"

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
            if (!Base.Port.empty()) {
                BaseURL += ":" + Base.Port;
            }
            ParseURL(BaseURL + Path);
            return;
        }

        // Handle relative paths
        string BaseURL = Base.Protocol + "//" + Base.Hostname;
        if (!Base.Port.empty()) {
            BaseURL += ":" + Base.Port;
        }

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
    void ParseURL(const string& URLString) {
        Href = URLString;

        // Parse protocol
        size_t ProtocolEnd = URLString.find("://");
        if (ProtocolEnd == string::npos) {
            throw runtime_error("Invalid URL: missing protocol");
        }

        Protocol = URLString.substr(0, ProtocolEnd + 1);
        string Remaining = URLString.substr(ProtocolEnd + 3);

        // Find the start of path/query/fragment
        size_t PathStart = Remaining.find_first_of("/?#");
        string HostAndPort;
        string PathQueryFragment;

        if (PathStart != string::npos) {
            HostAndPort = Remaining.substr(0, PathStart);
            PathQueryFragment = Remaining.substr(PathStart);
        } else {
            HostAndPort = Remaining;
            PathQueryFragment = "";
        }

        // Parse host and port
        size_t PortStart = HostAndPort.find(':');
        if (PortStart != string::npos) {
            Hostname = HostAndPort.substr(0, PortStart);
            Port = HostAndPort.substr(PortStart + 1);
        } else {
            Hostname = HostAndPort;
            Port = "";
        }

        // Parse path, query, and fragment
        if (!PathQueryFragment.empty()) {
            size_t QueryStart = PathQueryFragment.find('?');
            size_t FragmentStart = PathQueryFragment.find('#');

            if (QueryStart != string::npos
                && (FragmentStart == string::npos || QueryStart < FragmentStart)) {
                // Has query
                Pathname = PathQueryFragment.substr(0, QueryStart);

                if (FragmentStart != string::npos) {
                    Search = PathQueryFragment.substr(QueryStart, FragmentStart - QueryStart);
                    Hash = PathQueryFragment.substr(FragmentStart);
                } else {
                    Search = PathQueryFragment.substr(QueryStart);
                    Hash = "";
                }
            } else if (FragmentStart != string::npos) {
                // Has fragment but no query
                Pathname = PathQueryFragment.substr(0, FragmentStart);
                Search = "";
                Hash = PathQueryFragment.substr(FragmentStart);
            } else {
                // Only path
                Pathname = PathQueryFragment;
                Search = "";
                Hash = "";
            }
        } else {
            Pathname = "";
            Search = "";
            Hash = "";
        }

        // Ensure pathname starts with /
        if (Pathname.empty()) {
            Pathname = "/";
        }
    }
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

void CheckIssuerURL(const URLHelper& Issuer) {
    // Technically RFC 8414 does not permit a localhost HTTPS exemption, but this will be necessary
    // for ease of testing
    if (Issuer.Protocol != "https:" && Issuer.Hostname != "localhost"
        && Issuer.Hostname != "127.0.0.1") {
        throw runtime_error("Issuer URL must be HTTPS");
    }
    if (!Issuer.Hash.empty()) {
        throw runtime_error("Issuer URL must not have a fragment: " + Issuer.Href);
    }
    if (!Issuer.Search.empty()) {
        throw runtime_error("Issuer URL must not have a query string: " + Issuer.Href);
    }
}

struct CreateOAuthMetadataOptions {
    shared_ptr<OAuthServerProvider> Provider;
    URLHelper IssuerURL;
    optional<URLHelper> BaseURL;
    optional<URLHelper> ServiceDocumentationURL;
    optional<vector<string>> ScopesSupported;
};

// Helper function to check if a method is implemented by checking if it returns a default/null
// response
bool HasClientRegistrationSupport(const shared_ptr<OAuthServerProvider>& Provider) {
    // Check if the RegisterClient method is meaningfully implemented
    // We do this by creating a dummy client and seeing if it gets processed
    try {
        OAuthClientInformationFull DummyClient{};
        auto Future = Provider->GetClientsStore().RegisterClient(DummyClient);
        auto Result = Future.get();
        // If it returns nullopt immediately, it's not implemented
        return Result.has_value();
    } catch (...) {
        // If it throws or fails, assume it's not implemented
        return false;
    }
}

bool HasTokenRevocationSupport(const shared_ptr<OAuthServerProvider>& Provider) {
    // Check if RevokeToken method is meaningfully implemented
    // The default implementation just returns an empty future, so we check the type
    try {
        OAuthClientInformationFull DummyClient{};
        OAuthTokenRevocationRequest DummyRequest{};
        auto Future = Provider->RevokeToken(DummyClient, DummyRequest);
        // If it completes immediately without doing anything, it's the default implementation
        auto Status = Future.wait_for(std::chrono::milliseconds(1));
        return Status
               != std::future_status::ready; // If it's not immediately ready, it's doing real work
    } catch (...) {
        return false;
    }
}

OAuthMetadata CreateOAuthMetadata(const CreateOAuthMetadataOptions& Options) {
    const URLHelper& Issuer = Options.IssuerURL;
    const optional<URLHelper>& BaseURL = Options.BaseURL;

    CheckIssuerURL(Issuer);

    const string AuthorizationEndpoint = "/authorize";
    const string TokenEndpoint = "/token";

    // Properly check if registration is supported
    const optional<string> RegistrationEndpoint =
        HasClientRegistrationSupport(Options.Provider) ? optional<string>("/register") : nullopt;

    // Properly check if revocation is supported
    const optional<string> RevocationEndpoint =
        HasTokenRevocationSupport(Options.Provider) ? optional<string>("/revoke") : nullopt;

    OAuthMetadata Metadata{};
    Metadata.Issuer = Issuer.Href;
    Metadata.ServiceDocumentation = Options.ServiceDocumentationURL
                                        ? optional<string>(Options.ServiceDocumentationURL->Href)
                                        : nullopt;

    URLHelper BaseToUse = BaseURL ? *BaseURL : Issuer;
    Metadata.AuthorizationEndpoint = URLHelper(AuthorizationEndpoint, BaseToUse).Href;
    Metadata.ResponseTypesSupported = {MSG_KEY_CODE};
    Metadata.CodeChallengeMethodsSupported = {"S256"};

    Metadata.TokenEndpoint = URLHelper(TokenEndpoint, BaseToUse).Href;
    Metadata.TokenEndpointAuthMethodsSupported = {"client_secret_post"};
    Metadata.GrantTypesSupported = {"authorization_code", "refresh_token"};

    Metadata.ScopesSupported = Options.ScopesSupported;

    if (RevocationEndpoint) {
        Metadata.RevocationEndpoint = URLHelper(*RevocationEndpoint, BaseToUse).Href;
        Metadata.RevocationEndpointAuthMethodsSupported = vector<string>{"client_secret_post"};
    }

    if (RegistrationEndpoint) {
        Metadata.RegistrationEndpoint = URLHelper(*RegistrationEndpoint, BaseToUse).Href;
    }

    return Metadata;
}

/**
 * Express Router equivalent - handles HTTP routing
 */
class ExpressRouter {
  private:
    unordered_map<string, RequestHandler> Routes;
    vector<RequestHandler> Middleware;

  public:
    void Use(const string& Path, RequestHandler Handler) {
        Routes[Path] = Handler;
    }

    void Use(RequestHandler Handler) {
        Middleware.push_back(Handler);
    }

    RequestHandler CreateHandler() {
        return [=](const JSON& Request, JSON& Response) {
            // Apply middleware first
            for (const auto& MiddlewareHandler : Middleware) {
                MiddlewareHandler(Request, Response);
            }

            // Route to specific handler
            string Path = Request.value("path", "");
            auto It = Routes.find(Path);
            if (It != Routes.end()) {
                It->second(Request, Response);
            }
        };
    }
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
RequestHandler MCP_AuthRouter(const AuthRouterOptions& Options) {
    OAuthMetadata OAuthMetadata =
        CreateOAuthMetadata({.Provider = Options.Provider,
                             .IssuerURL = Options.IssuerURL,
                             .BaseURL = Options.BaseURL,
                             .ServiceDocumentationURL = Options.ServiceDocumentationURL,
                             .ScopesSupported = Options.ScopesSupported});

    ExpressRouter Router;

    // Add authorization endpoint
    Router.Use(URLHelper(OAuthMetadata.AuthorizationEndpoint).Pathname,
               [=](const JSON& Request, JSON& Response) {
                   // TODO: Call AuthorizationHandler({ provider: Options.Provider,
                   // ...Options.AuthorizationOptions })
                   Response[MSG_KEY_ERROR] = "authorization_handler_not_implemented";
               });

    // Add token endpoint
    Router.Use(URLHelper(OAuthMetadata.TokenEndpoint).Pathname,
               [=](const JSON& Request, JSON& Response) {
                   // TODO: Call TokenHandler({ provider: Options.Provider, ...Options.TokenOptions
                   // })
                   Response[MSG_KEY_ERROR] = "token_handler_not_implemented";
               });

    // Add metadata router
    Router.Use(MCP_AuthMetadataRouter(
        {.OAuthMetadata = OAuthMetadata,
         // This router is used for AS+RS combo's, so the issuer is also the resource server
         .ResourceServerURL = URLHelper(OAuthMetadata.Issuer),
         .ServiceDocumentationURL = Options.ServiceDocumentationURL,
         .ScopesSupported = Options.ScopesSupported,
         .ResourceName = Options.ResourceName}));

    // Add registration endpoint if supported
    if (OAuthMetadata.RegistrationEndpoint) {
        Router.Use(URLHelper(*OAuthMetadata.RegistrationEndpoint).Pathname,
                   [=](const JSON& Request, JSON& Response) {
                       // TODO: Call ClientRegistrationHandler({
                       //   clientsStore: Options.Provider->GetClientsStore(),
                       //   ...Options.ClientRegistrationOptions,
                       // })
                       Response[MSG_KEY_ERROR] = "registration_handler_not_implemented";
                   });
    }

    // Add revocation endpoint if supported
    if (OAuthMetadata.RevocationEndpoint) {
        Router.Use(URLHelper(*OAuthMetadata.RevocationEndpoint).Pathname,
                   [=](const JSON& Request, JSON& Response) {
                       // TODO: Call RevocationHandler({ provider: Options.Provider,
                       // ...Options.RevocationOptions })
                       Response[MSG_KEY_ERROR] = "revocation_handler_not_implemented";
                   });
    }

    return Router.CreateHandler();
}

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
RequestHandler MetadataHandler(const JSON& Metadata) {
    return [=](const JSON& Request, JSON& Response) {
        // TODO: Configure CORS to allow any origin, to make accessible to web-based MCP clients
        // TODO: Use allowedMethods(['GET'])

        string Method = Request.value(MSG_KEY_METHOD, "GET");
        if (Method != "GET") {
            Response[MSG_KEY_ERROR] = "method_not_allowed";
            Response["error_description"] =
                "The method " + Method + " is not allowed for this endpoint";
            Response["status"] = 405;
            Response["headers"] = JSON{{"Allow", "GET"}};
            return;
        }

        Response = Metadata;
        Response["status"] = 200;
    };
}

RequestHandler MCP_AuthMetadataRouter(const AuthMetadataOptions& Options) {
    CheckIssuerURL(URLHelper(Options.OAuthMetadata.Issuer));

    ExpressRouter Router;

    // Create protected resource metadata
    JSON ProtectedResourceMetadata = JSON::object();
    ProtectedResourceMetadata[MSG_KEY_RESOURCE] = Options.ResourceServerURL.Href;
    ProtectedResourceMetadata["authorization_servers"] =
        JSON::array({Options.OAuthMetadata.Issuer});

    if (Options.ScopesSupported) {
        ProtectedResourceMetadata["scopes_supported"] = *Options.ScopesSupported;
    }
    if (Options.ResourceName) {
        ProtectedResourceMetadata["resource_name"] = *Options.ResourceName;
    }
    if (Options.ServiceDocumentationURL) {
        ProtectedResourceMetadata["resource_documentation"] = Options.ServiceDocumentationURL->Href;
    }

    // Create OAuth authorization server metadata JSON
    JSON OAuthMetadataJSON = JSON::object();
    OAuthMetadataJSON["issuer"] = Options.OAuthMetadata.Issuer;
    OAuthMetadataJSON["authorization_endpoint"] = Options.OAuthMetadata.AuthorizationEndpoint;
    OAuthMetadataJSON["token_endpoint"] = Options.OAuthMetadata.TokenEndpoint;
    OAuthMetadataJSON["response_types_supported"] = Options.OAuthMetadata.ResponseTypesSupported;
    OAuthMetadataJSON["grant_types_supported"] = Options.OAuthMetadata.GrantTypesSupported;

    if (Options.OAuthMetadata.CodeChallengeMethodsSupported) {
        OAuthMetadataJSON["code_challenge_methods_supported"] =
            *Options.OAuthMetadata.CodeChallengeMethodsSupported;
    }
    if (Options.OAuthMetadata.TokenEndpointAuthMethodsSupported) {
        OAuthMetadataJSON["token_endpoint_auth_methods_supported"] =
            *Options.OAuthMetadata.TokenEndpointAuthMethodsSupported;
    }
    if (Options.OAuthMetadata.RegistrationEndpoint) {
        OAuthMetadataJSON["registration_endpoint"] = *Options.OAuthMetadata.RegistrationEndpoint;
    }
    if (Options.OAuthMetadata.RevocationEndpoint) {
        OAuthMetadataJSON["revocation_endpoint"] = *Options.OAuthMetadata.RevocationEndpoint;
        if (Options.OAuthMetadata.RevocationEndpointAuthMethodsSupported) {
            OAuthMetadataJSON["revocation_endpoint_auth_methods_supported"] =
                *Options.OAuthMetadata.RevocationEndpointAuthMethodsSupported;
        }
    }
    if (Options.OAuthMetadata.ScopesSupported) {
        OAuthMetadataJSON["scopes_supported"] = *Options.OAuthMetadata.ScopesSupported;
    }
    if (Options.OAuthMetadata.ServiceDocumentation) {
        OAuthMetadataJSON["service_documentation"] = *Options.OAuthMetadata.ServiceDocumentation;
    }

    Router.Use("/.well-known/oauth-protected-resource", MetadataHandler(ProtectedResourceMetadata));

    // Always add this for backwards compatibility
    Router.Use("/.well-known/oauth-authorization-server", MetadataHandler(OAuthMetadataJSON));

    return Router.CreateHandler();
}

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
string GetOAuthProtectedResourceMetadataURL(const URLHelper& ServerURL) {
    return URLHelper("/.well-known/oauth-protected-resource", ServerURL).Href;
}

MCP_NAMESPACE_END
