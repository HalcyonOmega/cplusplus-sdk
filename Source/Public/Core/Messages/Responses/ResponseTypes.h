#pragma once

#include "Core.h"

// TODO: @HalcyonOmega - Fix Includes

MCP_NAMESPACE_BEGIN

/* Client messages */
using ClientResult = variant<EmptyResult, CreateMessageResult, ListRootsResult>;

/* Server messages */
using ServerResult = variant<EmptyResult, InitializeResult, CompleteResult, GetPromptResult,
                             ListPromptsResult, ListResourcesResult, ListResourceTemplatesResult,
                             ReadResourceResult, CallToolResult, ListToolsResult>;

MCP_NAMESPACE_END