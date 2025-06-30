#pragma once

#include <functional>
#include <optional>
#include <string>

#include "CoreSDK/Common/MCPContext.h"
#include "CoreSDK/Messages/MessageBase.h"
#include "CoreSDK/Messages/RequestBase.h"
#include "JSONProxy.h"

struct ErrorResponseBase;

MCP_NAMESPACE_BEGIN

using ErrorResponseHandler =
    std::function<void(const ErrorResponseBase& InError, std::optional<MCPContext*> InContext)>;

struct MCPError {
    int64_t Code;
    std::string Message;
    std::optional<JSONValue> Data;

    JKEY(CODEKEY, Code, "code")
    JKEY(MESSAGEKEY, Message, "message")
    JKEY(DATAKEY, Data, "data")

    DEFINE_TYPE_JSON(MCPError, CODEKEY, MESSAGEKEY, DATAKEY)
};

// Standard JSON-RPC 2.0 error codes
namespace ErrorCodes {
static constexpr int64_t PARSE_ERROR = -32700;
static constexpr int64_t INVALID_REQUEST = -32600;
static constexpr int64_t METHOD_NOT_FOUND = -32601;
static constexpr int64_t INVALID_PARAMS = -32602;
static constexpr int64_t INTERNAL_ERROR = -32603;
} // namespace ErrorCodes

struct ErrorResponseBase : MessageBase {
    RequestID ID;
    MCPError Error;

    JKEY(IDKEY, ID, "id")
    JKEY(ERRORKEY, Error, "error")

    DEFINE_TYPE_JSON_DERIVED(ErrorResponseBase, MessageBase, IDKEY, ERRORKEY)

    ErrorResponseBase(RequestID InID, MCPError InError)
        : MessageBase(), ID(std::move(InID)), Error(std::move(InError)) {}

    static constexpr std::string_view MessageName{"DefaultErrorResponse"};
};

template <typename T>
concept ConcreteErrorResponse = std::is_base_of_v<ErrorResponseBase, T> && requires {
    { T::MessageName } -> std::same_as<std::string_view>;
};

MCP_NAMESPACE_END