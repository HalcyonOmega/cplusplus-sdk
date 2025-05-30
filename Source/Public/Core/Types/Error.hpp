#pragma once

#include "Constants.h"
#include "Core.h"

MCP_NAMESPACE_BEGIN

// Error codes defined by the JSON-RPC specification.
enum class ErrorCode : int32_t {
    // SDK error codes
    ConnectionClosed = ERRCODE_CONNECTION_CLOSED,
    RequestTimeout = ERRCODE_REQUEST_TIMEOUT,

    // Standard JSON-RPC error codes
    ParseError = ERRCODE_PARSE_ERROR,
    InvalidRequest = ERRCODE_INVALID_REQUEST,
    MethodNotFound = ERRCODE_METHOD_NOT_FOUND,
    InvalidParams = ERRCODE_INVALID_PARAMS,
    InternalError = ERRCODE_INTERNAL_ERROR,
};

class Error {
  private:
    ErrorCode code_;
    string message_;
    optional<JSON> data_;
    string full_message_;

  public:
    Error(ErrorCode code, const string& message, optional<JSON> data = nullopt)
        : code_(code), message_(message), data_(data) {
        full_message_ = "MCP error " + to_string(code) + ": " + message;
    }

    const char* what() const noexcept override {
        return full_message_.c_str();
    }

    ErrorCode getCode() const {
        return code_;
    }
    const string& getMessage() const {
        return message_;
    }
    const optional<JSON>& getData() const {
        return data_;
    }
};

MCP_NAMESPACE_END