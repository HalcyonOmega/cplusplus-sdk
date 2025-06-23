#pragma once

#include <optional>
#include <string>
#include <variant>

#include "CoreSDK/Messages/MessageBase.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

using RequestID = std::variant<std::string, int64_t>;

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