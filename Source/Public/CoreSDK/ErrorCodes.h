#pragma once

#include <cstdint>
#include <string_view>

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
constexpr std::string_view PARSE_ERROR = "Parse error";
constexpr std::string_view INVALID_REQUEST = "Invalid Request";
constexpr std::string_view METHOD_NOT_FOUND = "Method not found";
constexpr std::string_view INVALID_PARAMS = "Invalid params";
constexpr std::string_view INTERNAL_ERROR = "Internal error";

constexpr std::string_view TOOL_NOT_FOUND = "Tool not found";
constexpr std::string_view TOOL_EXECUTION_ERROR = "Tool execution error";
constexpr std::string_view RESOURCE_NOT_FOUND = "Resource not found";
constexpr std::string_view RESOURCE_ACCESS_ERROR = "Resource access error";
constexpr std::string_view PROMPT_NOT_FOUND = "Prompt not found";
constexpr std::string_view PROMPT_EXECUTION_ERROR = "Prompt execution error";
constexpr std::string_view INITIALIZATION_ERROR = "Initialization error";
constexpr std::string_view CAPABILITY_MISMATCH = "Capability mismatch";
constexpr std::string_view TRANSPORT_ERROR = "Transport error";
constexpr std::string_view PROTOCOL_VERSION_MISMATCH = "Protocol version mismatch";
constexpr std::string_view AUTHENTICATION_ERROR = "Authentication error";
constexpr std::string_view AUTHORIZATION_ERROR = "Authorization error";
constexpr std::string_view RATE_LIMIT_EXCEEDED = "Rate limit exceeded";
constexpr std::string_view SCHEMA_VALIDATION_ERROR = "Schema validation error";
constexpr std::string_view CANCELLED = "Request cancelled";
} // namespace MCPErrorMessages

MCP_NAMESPACE_END