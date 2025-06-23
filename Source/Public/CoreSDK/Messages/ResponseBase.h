
#pragma once

#include "CoreSDK/Common/MCPTypes.h"
#include "CoreSDK/Messages/ErrorBase.h"
#include "CoreSDK/Messages/MessageBase.h"

MCP_NAMESPACE_BEGIN

struct ResponseBase : MessageBase {
    RequestID ID;
    std::optional<ErrorBase> Error;
    std::optional<JSONValue> Result;

    JKEY(IDKEY, ID, "id")
    JKEY(RESULTKEY, Result, "result")
    JKEY(ERRORKEY, Error, "error")

    DEFINE_TYPE_JSON_DERIVED(ResponseBase, MessageBase, IDKEY, RESULTKEY, ERRORKEY)
};

MCP_NAMESPACE_END