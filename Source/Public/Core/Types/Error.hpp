#pragma once

#include "Core.h"

namespace MCP::Types {

// Error codes defined by the JSON-RPC specification.
enum class ErrorCode : int32_t {
  // SDK error codes
  ConnectionClosed = -32000,
  RequestTimeout = -32001,

  // Standard JSON-RPC error codes
  ParseError = -32700,
  InvalidRequest = -32600,
  MethodNotFound = -32601,
  InvalidParams = -32602,
  InternalError = -32603,
}

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