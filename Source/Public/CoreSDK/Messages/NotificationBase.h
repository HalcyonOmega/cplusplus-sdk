
#pragma once

#include "CoreSDK/Common/MCPTypes.h"
#include "CoreSDK/Messages/MessageBase.h"

MCP_NAMESPACE_BEGIN

struct NotificationBase : MessageBase {
    std::string Method;
    std::optional<JSONValue> Params;

    JKEY(METHODKEY, Method, "method")
    JKEY(PARAMSKEY, Params, "params")

    DEFINE_TYPE_JSON_DERIVED(NotificationBase, MessageBase, METHODKEY, PARAMSKEY)
};

MCP_NAMESPACE_END