#pragma once

#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

enum class Role { User, Assistant };

DEFINE_ENUM_JSON(Role, {Role::User, "user"}, {Role::Assistant, "assistant"})

MCP_NAMESPACE_END