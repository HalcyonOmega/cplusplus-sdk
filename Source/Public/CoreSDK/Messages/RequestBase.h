#pragma once

#include "CoreSDK/Common/MCPTypes.h"
#include "CoreSDK/Messages/MessageBase.h"

MCP_NAMESPACE_BEGIN

struct RequestBase : MessageBase {
    RequestID ID;
    std::string Method;
    std::optional<JSONValue> Params;

    JKEY(IDKEY, ID, "id")
    JKEY(METHODKEY, Method, "method")
    JKEY(PARAMSKEY, Params, "params")

    DEFINE_TYPE_JSON_DERIVED(RequestBase, MessageBase, IDKEY, METHODKEY, PARAMSKEY)
};

MCP_NAMESPACE_END