#include "Auth/Handlers/Metadata.h"

MCP_NAMESPACE_BEGIN

RequestHandler MetadataHandler(const MetadataType& Metadata) {
    // Nested router so we can configure middleware and restrict HTTP method
    auto Router = CreateRouter();

    // Configure CORS to allow any origin, to make accessible to web-based MCP clients
    Router.Use(CORS());

    Router.Use(AllowedMethods({"GET"}));

    Router.Get("/",
               [Metadata](const Request& Req, Response& Res) { Res.Status(200).JSON(Metadata); });

    return Router;
}

MCP_NAMESPACE_END
