#pragma once

#include <cstdint>

#include "Macros.h"

// TODO: @HalcyonOmega - Is this used? Does this facilitate the MCP spec?

MCP_NAMESPACE_BEGIN

// MCP Standard Error Codes
// Following JSON-RPC 2.0 and MCP Specification (2025-03-26)
namespace MCPErrorCodes {
// JSON-RPC standard errors
constexpr int64_t PARSE_ERROR = -32700;
constexpr int64_t INVALID_REQUEST = -32600;
constexpr int64_t METHOD_NOT_FOUND = -32601;
constexpr int64_t INVALID_PARAMS = -32602;
constexpr int64_t INTERNAL_ERROR = -32603;

// MCP-specific errors (2025-03-26 specification)
constexpr int64_t TOOL_NOT_FOUND = -32001;
constexpr int64_t TOOL_EXECUTION_ERROR = -32002;
constexpr int64_t RESOURCE_NOT_FOUND = -32003;
constexpr int64_t RESOURCE_ACCESS_ERROR = -32004;
constexpr int64_t PROMPT_NOT_FOUND = -32005;
constexpr int64_t PROMPT_EXECUTION_ERROR = -32006;
constexpr int64_t INITIALIZATION_ERROR = -32007;
constexpr int64_t CAPABILITY_MISMATCH = -32008;
constexpr int64_t TRANSPORT_ERROR = -32009;
constexpr int64_t PROTOCOL_VERSION_MISMATCH = -32010;
constexpr int64_t AUTHENTICATION_ERROR = -32011;
constexpr int64_t AUTHORIZATION_ERROR = -32012;
constexpr int64_t RATE_LIMIT_EXCEEDED = -32013;
constexpr int64_t SCHEMA_VALIDATION_ERROR = -32014;
constexpr int64_t CANCELLED = -32015;
} // namespace MCPErrorCodes

// Error message constants
namespace MCPErrorMessages {
constexpr const char* PARSE_ERROR = "Parse error";
constexpr const char* INVALID_REQUEST = "Invalid Request";
constexpr const char* METHOD_NOT_FOUND = "Method not found";
constexpr const char* INVALID_PARAMS = "Invalid params";
constexpr const char* INTERNAL_ERROR = "Internal error";

constexpr const char* TOOL_NOT_FOUND = "Tool not found";
constexpr const char* TOOL_EXECUTION_ERROR = "Tool execution error";
constexpr const char* RESOURCE_NOT_FOUND = "Resource not found";
constexpr const char* RESOURCE_ACCESS_ERROR = "Resource access error";
constexpr const char* PROMPT_NOT_FOUND = "Prompt not found";
constexpr const char* PROMPT_EXECUTION_ERROR = "Prompt execution error";
constexpr const char* INITIALIZATION_ERROR = "Initialization error";
constexpr const char* CAPABILITY_MISMATCH = "Capability mismatch";
constexpr const char* TRANSPORT_ERROR = "Transport error";
constexpr const char* PROTOCOL_VERSION_MISMATCH = "Protocol version mismatch";
constexpr const char* AUTHENTICATION_ERROR = "Authentication error";
constexpr const char* AUTHORIZATION_ERROR = "Authorization error";
constexpr const char* RATE_LIMIT_EXCEEDED = "Rate limit exceeded";
constexpr const char* SCHEMA_VALIDATION_ERROR = "Schema validation error";
constexpr const char* CANCELLED = "Request cancelled";
} // namespace MCPErrorMessages

MCP_NAMESPACE_END