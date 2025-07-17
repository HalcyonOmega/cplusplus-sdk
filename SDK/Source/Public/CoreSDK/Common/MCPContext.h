#pragma once

#include "CoreSDK/Common/Macros.h"

MCP_NAMESPACE_BEGIN

/**
 * MCPContext provides access to MCP session capabilities within tools, resources, and prompts.
 *
 * The context object allows functions to:
 * - Log messages back to the MCP client
 * - Report progress on long-running operations
 * - Read resources from the server
 * - Request LLM sampling from the client
 * - Access request metadata
 * - Interact with the underlying server
 */
class MCPContext {
  public:
    MCPContext();
};

MCP_NAMESPACE_END