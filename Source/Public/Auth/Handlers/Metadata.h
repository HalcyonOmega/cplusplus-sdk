#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// TODO: Fix External Ref: express
// TODO: Fix External Ref: RequestHandler
// TODO: Fix External Ref: OAuthMetadata
// TODO: Fix External Ref: OAuthProtectedResourceMetadata
// TODO: Fix External Ref: cors

// Union type for metadata
using MetadataType = variant<OAuthMetadata, OAuthProtectedResourceMetadata>;

RequestHandler MetadataHandler(const MetadataType& Metadata);

MCP_NAMESPACE_END
