#pragma once

#include "Macros.h"
#include "Utilities/JSON/JSONLayer.hpp"

MCP_NAMESPACE_BEGIN

static constexpr const char* ROLE_USER = "user";
static constexpr const char* ROLE_ASSISTANT = "assistant";

// Role {
//   MSG_DESCRIPTION
//       : "The sender or recipient of messages and data in a conversation.",
//         MSG_ENUM : [ "assistant", "user" ],
//                  MSG_TYPE : MSG_STRING
// };

// The sender or recipient of messages and data in a conversation.
enum class Role { User, Assistant };

DEFINE_ENUM_JSON(Role, {{Role::User, ROLE_USER}, {Role::Assistant, ROLE_ASSISTANT}})

MCP_NAMESPACE_END