#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// Message Keys
static constexpr const char* MSG_NULL = "";
static constexpr const char* MSG_ID = "id";
static constexpr const char* MSG_METHOD = "method";
static constexpr const char* MSG_PARAMS = "params";
static constexpr const char* MSG_RESULT = "result";
static constexpr const char* MSG_ERROR = "error";

// JSON-RPC
static constexpr const char* MSG_JSON_RPC = "jsonrpc";
static constexpr const char* MSG_JSON_RPC_VERSION = "2.0";

MCP_NAMESPACE_END