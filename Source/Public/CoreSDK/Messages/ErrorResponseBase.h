#pragma once

#include "CoreSDK/Messages/MessageBase.h"
#include "CoreSDK/Messages/RequestBase.h"

struct ErrorResponseBase;

MCP_NAMESPACE_BEGIN

using ErrorResponseHandler = std::function<void(const ErrorResponseBase& InError)>;

enum class Errors {
    // JSON-RPC
    Ok = 0,
    ParseError = -32700,
    InvalidRequest = -32600,
    MethodNotFound = -32601,
    InvalidParams = -32602,
    InternalError = -32603,

    // Server
    ConnectionClosed = -32000,
    RequestTimeout = -32001,
    InvalidNotification = -32002,
    InternalInputTerminate = -32003,
    InternalInputError = -32004,
    InternalOutputError = -32005,
};

namespace ErrorMessages {
constexpr std::string_view MSG_OK = "OK";
constexpr std::string_view PARSE_ERROR = "Parse error";
constexpr std::string_view INVALID_REQUEST = "Invalid Request";
constexpr std::string_view METHOD_NOT_FOUND = "Method not found";
constexpr std::string_view INVALID_PARAMS = "Invalid params";
constexpr std::string_view INTERNAL_ERROR = "Internal error";

constexpr std::string_view CONNECTION_CLOSED = "Connection closed";
constexpr std::string_view REQUEST_TIMEOUT = "Request timeout";
constexpr std::string_view INVALID_NOTIFICATION = "Invalid notification";
constexpr std::string_view INTERNAL_INPUT_TERMINATE = "Internal input terminate";
constexpr std::string_view INTERNAL_INPUT_ERROR = "Internal input error";
constexpr std::string_view INTERNAL_OUTPUT_ERROR = "Internal output error";
} // namespace ErrorMessages

DEFINE_ENUM_JSON(Errors, {Errors::Ok, ErrorMessages::MSG_OK},
                 {Errors::ParseError, ErrorMessages::PARSE_ERROR},
                 {Errors::InvalidRequest, ErrorMessages::INVALID_REQUEST},
                 {Errors::MethodNotFound, ErrorMessages::METHOD_NOT_FOUND},
                 {Errors::InvalidParams, ErrorMessages::INVALID_PARAMS},
                 {Errors::InternalError, ErrorMessages::INTERNAL_ERROR},
                 {Errors::ConnectionClosed, ErrorMessages::CONNECTION_CLOSED},
                 {Errors::RequestTimeout, ErrorMessages::REQUEST_TIMEOUT},
                 {Errors::InvalidNotification, ErrorMessages::INVALID_NOTIFICATION},
                 {Errors::InternalInputTerminate, ErrorMessages::INTERNAL_INPUT_TERMINATE},
                 {Errors::InternalInputError, ErrorMessages::INTERNAL_INPUT_ERROR},
                 {Errors::InternalOutputError, ErrorMessages::INTERNAL_OUTPUT_ERROR})

struct MCPErrorInfo {
    Errors Code;         // The error type that occurred.
    std::string Message; // A short description of the error. The message SHOULD be limited to a
                         // concise single sentence.
    std::optional<JSONValue>
        Data; // Additional information about the error. The value of this member is defined by
              // the sender (e.g. detailed error information, nested errors etc.)

    JKEY(CODEKEY, Code, "code")
    JKEY(MESSAGEKEY, Message, "message")
    JKEY(DATAKEY, Data, "data")

    DEFINE_TYPE_JSON(MCPErrorInfo, CODEKEY, MESSAGEKEY, DATAKEY)
};

// Error structure
struct ErrorResponseBase : MessageBase {
    RequestID ID;
    MCPErrorInfo Error;

    JKEY(IDKEY, ID, "id")
    JKEY(ERRORKEY, Error, "error")

    DEFINE_TYPE_JSON_DERIVED(ErrorResponseBase, MessageBase, IDKEY, ERRORKEY)

    ErrorResponseBase(RequestID InID, MCPErrorInfo InError,
                      std::optional<ErrorResponseHandler> InHandler = std::nullopt)
        : ID(std::move(InID)), Error(std::move(InError)), Handler(std::move(InHandler)) {}

    std::optional<ErrorResponseHandler> Handler;
};

MCP_NAMESPACE_END