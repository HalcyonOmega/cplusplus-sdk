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
    ResultParams Result;

    JKEY(IDKEY, ID, "id")
    JKEY(RESULTKEY, Result, "result")

    DEFINE_TYPE_JSON_DERIVED(ResponseBase, MessageBase, IDKEY, RESULTKEY)

    ResponseBase(RequestID InID, ResultParams InResult = ResultParams{})
        : MessageBase(), ID(std::move(InID)), Result(std::move(InResult)) {}

    // Get typed result - cast the base Result to the derived response's Result type
    template <typename TResultType> [[nodiscard]] const TResultType& GetResult() const {
        return static_cast<const TResultType&>(Result);
    }
};

template <typename T>
concept ConcreteResponse = std::is_base_of_v<ResponseBase, T>;

MCP_NAMESPACE_END