#pragma once

#include "Macros.h"
#include "json.hpp"

MCP_NAMESPACE_BEGIN

static constexpr const char* MSG_USER = "user";
static constexpr const char* MSG_ASSISTANT = "assistant";

// Role {
//   MSG_DESCRIPTION
//       : "The sender or recipient of messages and data in a conversation.",
//         "enum" : [ "assistant", "user" ],
//                  MSG_TYPE : MSG_STRING
// };

/**
 * The sender or recipient of messages and data in a conversation.
 */
enum class Role { User, Assistant };

NLOHMANN_JSON_SERIALIZE_ENUM(Role, {{Role::User, MSG_USER}, {Role::Assistant, MSG_ASSISTANT}})

MCP_NAMESPACE_END