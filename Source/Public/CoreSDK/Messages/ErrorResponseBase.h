#pragma once

#include <functional>
#include <optional>
#include <string>

#include "CoreSDK/Common/MCPContext.h"
#include "CoreSDK/Messages/MessageBase.h"
#include "CoreSDK/Messages/RequestBase.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

struct MCPError {
    int64_t Code;
    std::string Message;
    std::optional<JSONData> Data;

    JKEY(CODEKEY, Code, "code")
    JKEY(MESSAGEKEY, Message, "message")
    JKEY(DATAKEY, Data, "data")

    DEFINE_TYPE_JSON(MCPError, CODEKEY, MESSAGEKEY, DATAKEY)

    MCPError(int64_t InCode, std::string_view InMessage,
             std::optional<JSONData> InData = std::nullopt)
        : Code(InCode), Message(InMessage), Data(std::move(InData)) {}
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
    MCPError ErrorData;

    JKEY(IDKEY, ID, "id")
    JKEY(ERRORKEY, ErrorData, "error")

    DEFINE_TYPE_JSON_DERIVED(ErrorResponseBase, MessageBase, IDKEY, ERRORKEY)

    ErrorResponseBase(RequestID InID, MCPError InError)
        : MessageBase(), ID(std::move(InID)), ErrorData(std::move(InError)) {}
};

using ErrorResponseHandler =
    std::function<void(const ErrorResponseBase& InError, std::optional<MCPContext*> InContext)>;

template <typename T>
concept ConcreteErrorResponse = std::is_base_of_v<ErrorResponseBase, T>;

MCP_NAMESPACE_END