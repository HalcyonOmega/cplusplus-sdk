#pragma once

#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

// ERole {
//   MSG_DESCRIPTION: "The sender or recipient of messages and data in a conversation.",
//         MSG_ENUM: [ "assistant", "user" ],
//                  MSG_TYPE: MSG_STRING
// };

// The sender or recipient of messages and data in a conversation.
enum class ERole
{
	Unknown,
	User,
	Assistant
};

DEFINE_ENUM_JSON(ERole, { ERole::Unknown, "unknown" }, { ERole::User, "user" }, { ERole::Assistant, "assistant" })

MCP_NAMESPACE_END