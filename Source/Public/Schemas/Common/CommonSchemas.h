// Common/shared schemas for Model Context Protocol
#pragma once

#include "Constants.h"
#include "Core.h"

MCP_NAMESPACE_BEGIN

// struct ProgressToken {
//   "description" : "A progress token, used to associate progress "
//                   "notifications with the original request.",
//                   "type" : [ "string", "integer" ]
// };

// struct Role {
//   "description"
//       : "The sender or recipient of messages and data in a conversation.",
//         "enum" : [ "assistant", "user" ],
//                  "type" : "string"
// };

// struct Cursor {
//   "description" : "An opaque token used to represent a cursor for
//   pagination.",
//                   "type" : "string"
// };

// struct Implementation {
//   "description" : "Describes the name and version of an MCP implementation.",
//                   "properties"
//       : {"name" : {"type" : "string"}, "version" : {"type" : "string"}},
//         "required" : [ "name", "version" ],
//                      "type" : "object"
// };

/**
 * An opaque token used to represent a cursor for pagination.
 */
using Cursor = string;

/**
 * Describes the name and version of an MCP implementation.
 */
struct Implementation {
    string name;
    string version;
};

/**
 * The sender or recipient of messages and data in a conversation.
 */
enum class Role { user, assistant };

MCP_NAMESPACE_END