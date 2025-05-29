#pragma once

#include "../Core/Common.hpp"

namespace MCP::Auth {

// TODO: Fix External Ref: express
// TODO: Fix External Ref: RequestHandler
// TODO: Fix External Ref: OAuthMetadata
// TODO: Fix External Ref: OAuthProtectedResourceMetadata
// TODO: Fix External Ref: cors
// TODO: Fix External Ref: allowedMethods

// Forward declarations for Express-like types
class Router;
class Request;
class Response;
using RequestHandler = function<void(const Request&, Response&)>;

// Union type for metadata
using MetadataType = variant<OAuthMetadata, OAuthProtectedResourceMetadata>;

RequestHandler MetadataHandler(const MetadataType& Metadata) {
    // Nested router so we can configure middleware and restrict HTTP method
    auto Router = CreateRouter();

    // Configure CORS to allow any origin, to make accessible to web-based MCP clients
    Router.Use(CORS());

    Router.Use(AllowedMethods({"GET"}));

    Router.Get("/", [Metadata](const Request& Req, Response& Res) {
        Res.Status(200).JSON(Metadata);
    });

    return Router;
}

} // namespace MCP::Auth
