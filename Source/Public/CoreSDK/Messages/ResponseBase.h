#pragma once

#include <utility>

#include "CoreSDK/Messages/MessageBase.h"
#include "CoreSDK/Messages/RequestBase.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

struct ResultParams {
    std::optional<JSONValue> Meta;

    JKEY(METAKEY, Meta, "_meta")

    DEFINE_TYPE_JSON(ResultParams, METAKEY)
};

struct PaginatedResultParams : ResultParams {
    std::optional<std::string> NextCursor; // An opaque token representing the next pagination
                                           // position. If provided, the client should use this
                                           // cursor to fetch the next page of results.

    JKEY(NEXT_CURSORKEY, NextCursor, "nextCursor")

    DEFINE_TYPE_JSON_DERIVED(PaginatedResultParams, ResultParams, NEXT_CURSORKEY)
};

struct ResponseBase : MessageBase {
    RequestID ID;
    ResultParams Result;

    JKEY(IDKEY, ID, "id")
    JKEY(RESULTKEY, Result, "result")

    DEFINE_TYPE_JSON_DERIVED(ResponseBase, MessageBase, IDKEY, RESULTKEY)

    ResponseBase(RequestID InID, ResultParams InResult = ResultParams{})
        : ID(std::move(InID)), Result(std::move(InResult)) {}
};

MCP_NAMESPACE_END