#pragma once

#include <functional>
#include <optional>
#include <string>

#include "CoreSDK/Messages/MessageBase.h"
#include "CoreSDK/Messages/RequestBase.h"
#include "JSONProxy.h"

struct ResponseBase;

MCP_NAMESPACE_BEGIN

using ResponseHandler =
    std::function<void(const ResponseBase& InResponse, std::optional<MCPContext*> InContext)>;

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
    ResultParams ResultData;

    JKEY(IDKEY, ID, "id")
    JKEY(RESULTKEY, ResultData, "result")

    DEFINE_TYPE_JSON_DERIVED(ResponseBase, MessageBase, IDKEY, RESULTKEY)

    ResponseBase(RequestID InID, ResultParams InResult = ResultParams{})
        : MessageBase(), ID(std::move(InID)), ResultData(std::move(InResult)) {}
};

template <typename T>
concept ConcreteResponse = std::is_base_of_v<ResponseBase, T>;

// Get typed result - cast the base Result to the derived response's Result type
template <typename TResultType, ConcreteResponse T>
[[nodiscard]] TResultType GetResponseResult(T& InResponse) {
    return static_cast<const TResultType&>(InResponse.ResultData);
}

MCP_NAMESPACE_END