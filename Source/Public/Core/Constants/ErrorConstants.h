#pragma once

#include "Macros.h"

MCP_NAMESPACE_BEGIN

enum class Errors {
    // JSON-RPC
    OK = 0,
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

MCP_NAMESPACE_END