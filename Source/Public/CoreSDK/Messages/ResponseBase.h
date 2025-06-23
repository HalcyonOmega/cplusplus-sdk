#pragma once

#include <utility>

#include "CoreSDK/Messages/ErrorBase.h"
#include "CoreSDK/Messages/MessageBase.h"
#include "CoreSDK/Messages/RequestBase.h"

MCP_NAMESPACE_BEGIN

struct ResponseBase : MessageBase {
    RequestID ID;
    std::optional<ErrorBase> Error;
    std::optional<JSONValue> Result;

    JKEY(IDKEY, ID, "id")
    JKEY(RESULTKEY, Result, "result")
    JKEY(ERRORKEY, Error, "error")

    DEFINE_TYPE_JSON_DERIVED(ResponseBase, MessageBase, IDKEY, RESULTKEY, ERRORKEY)

    ResponseBase(RequestID InID, std::optional<JSONValue> InResult = std::nullopt,
                 std::optional<ErrorBase> InError = std::nullopt)
        : ID(std::move(InID)), Error(std::move(InError)), Result(std::move(InResult)) {}
};

MCP_NAMESPACE_END