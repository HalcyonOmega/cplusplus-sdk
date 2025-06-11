// Common/shared schemas for Model Context Protocol
#pragma once

#include "Constants.h"
#include "Core.h"

MCP_NAMESPACE_BEGIN

// Implementation {
//   MSG_DESCRIPTION : "Describes the name and version of an MCP implementation.",
//                   MSG_PROPERTIES
//       : {MSG_NAME : {MSG_TYPE : MSG_STRING}, MSG_VERSION : {MSG_TYPE : MSG_STRING}},
//         MSG_REQUIRED : [ MSG_NAME, MSG_VERSION ],
//                      MSG_TYPE : MSG_OBJECT
// };

/**
 * Describes the name and version of an MCP implementation.
 */
struct Implementation {
    string name;
    string version;
};

// Role {
//   MSG_DESCRIPTION
//       : "The sender or recipient of messages and data in a conversation.",
//         "enum" : [ "assistant", "user" ],
//                  MSG_TYPE : MSG_STRING
// };

/**
 * The sender or recipient of messages and data in a conversation.
 */
enum class Role { user, assistant };

MCP_NAMESPACE_END