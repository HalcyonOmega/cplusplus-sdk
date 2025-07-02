#pragma once

#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

// Role {
//   MSG_DESCRIPTION
//       : "The sender or recipient of messages and data in a conversation.",
//         MSG_ENUM : [ "assistant", "user" ],
//                  MSG_TYPE : MSG_STRING
// };

// The sender or recipient of messages and data in a conversation.
enum class Role { User, Assistant };

DEFINE_ENUM_JSON(Role, {Role::User, "user"}, {Role::Assistant, "assistant"})

MCP_NAMESPACE_END