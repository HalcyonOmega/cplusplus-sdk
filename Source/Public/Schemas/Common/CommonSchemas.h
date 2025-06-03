// Common/shared schemas for Model Context Protocol
#pragma once

#include "Constants.h"
#include "Core.h"

MCP_NAMESPACE_BEGIN

// Implementation {
//   "description" : "Describes the name and version of an MCP implementation.",
//                   "properties"
//       : {"name" : {"type" : "string"}, "version" : {"type" : "string"}},
//         "required" : [ "name", "version" ],
//                      "type" : "object"
// };

/**
 * Describes the name and version of an MCP implementation.
 */
struct Implementation {
    string name;
    string version;
};

// Role {
//   "description"
//       : "The sender or recipient of messages and data in a conversation.",
//         "enum" : [ "assistant", "user" ],
//                  "type" : "string"
// };

/**
 * The sender or recipient of messages and data in a conversation.
 */
enum class Role { user, assistant };

MCP_NAMESPACE_END