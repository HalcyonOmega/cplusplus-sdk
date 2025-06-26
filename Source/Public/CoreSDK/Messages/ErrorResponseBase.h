#pragma once

#include <functional>
#include <optional>
#include <string>
#include <variant>

#include "CoreSDK/Messages/MessageBase.h"
#include "CoreSDK/Messages/RequestBase.h"
#include "JSONProxy.h"

struct ErrorResponseBase;

MCP_NAMESPACE_BEGIN

// Forward declare concept from EventSignatures.h
template <typename T>
concept ErrorResponseHandlerConcept = requires(T handler, const ErrorResponseBase& error) {
    { handler(error) } -> std::same_as<void>;
};

// Keep type alias for storage compatibility
using ErrorResponseHandler = std::function<void(const ErrorResponseBase& InError)>;

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

    // Concept-based constructors that accept any callable matching the concept
    template <ErrorResponseHandlerConcept T>
    ErrorResponseBase(RequestID InID, MCPError InError, std::optional<T> InHandler = std::nullopt)
        : MessageBase(), ID(std::move(InID)), Error(std::move(InError)) {
        if (InHandler.has_value()) { Handler = ErrorResponseHandler(std::move(InHandler.value())); }
    }

    // Legacy constructor for backward compatibility
    ErrorResponseBase(RequestID InID, MCPError InError,
                      std::optional<ErrorResponseHandler> InHandler = std::nullopt)
        : MessageBase(), ID(std::move(InID)), Error(std::move(InError)),
          Handler(std::move(InHandler)) {}

    std::optional<ErrorResponseHandler> Handler;
};

MCP_NAMESPACE_END