#pragma once

#include "Core.h"

// TODO: @HalcyonOmega - Fix Includes

MCP_NAMESPACE_BEGIN

/* Client messages */
using ClientRequest = variant<PingRequest, InitializeRequest, CompleteRequest, SetLevelRequest,
                              GetPromptRequest, ListPromptsRequest, ListResourcesRequest,
                              ListResourceTemplatesRequest, ReadResourceRequest, SubscribeRequest,
                              UnsubscribeRequest, CallToolRequest, ListToolsRequest>;

/* Server messages */
using ServerRequest = variant<PingRequest, CreateMessageRequest, ListRootsRequest>;

MCP_NAMESPACE_END