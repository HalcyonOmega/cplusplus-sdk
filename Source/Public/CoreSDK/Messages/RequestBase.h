#pragma once

#include <optional>
#include <string>
#include <utility>
#include <variant>

#include "CoreSDK/Messages/MessageBase.h"
#include "JSONProxy.h"
#include "UUIDProxy.h"

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

    RequestBase(std::string_view InMethod, std::optional<JSONValue> InParams = std::nullopt)
        : ID(GenerateUUID()), Method(InMethod), Params(std::move(InParams)) {}

    RequestBase(RequestID InID, std::string_view InMethod,
                std::optional<JSONValue> InParams = std::nullopt)
        : ID(std::move(InID)), Method(InMethod), Params(std::move(InParams)) {}
};

MCP_NAMESPACE_END