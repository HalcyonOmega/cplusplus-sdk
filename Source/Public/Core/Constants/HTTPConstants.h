#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// HTTP Header Constants
static constexpr const char* HTTP_HEADER_CONTENT_TYPE = "Content-Type";
static constexpr const char* HTTP_HEADER_ACCEPT = "Accept";
static constexpr const char* HTTP_HEADER_MCP_SESSION_ID = "Mcp-Session-Id";

// HTTP Header Values
static constexpr const char* HTTP_CONTENT_TYPE_JSON = "application/json";
static constexpr const char* HTTP_ACCEPT_JSON = "application/json";
static constexpr const char* HTTP_ACCEPT_EVENT_STREAM = "text/event-stream";
static constexpr const char* HTTP_ACCEPT_JSON_AND_EVENT_STREAM =
    "application/json, text/event-stream";

// HTTP Methods
static constexpr const char* HTTP_METHOD_POST = "POST";
static constexpr const char* HTTP_METHOD_GET = "GET";
static constexpr const char* HTTP_METHOD_DELETE = "DELETE";

// HTTP Response Codes (for reference)
static constexpr int HTTP_STATUS_OK = 200;
static constexpr int HTTP_STATUS_ACCEPTED = 202;
static constexpr int HTTP_STATUS_UNAUTHORIZED = 401;
static constexpr int HTTP_STATUS_NOT_FOUND = 404;
static constexpr int HTTP_STATUS_METHOD_NOT_ALLOWED = 405;

// Other HTTP-related constants
static constexpr const char* HTTP_HEADER_EVENT_ID = "id";
static constexpr const char* HTTP_HEADER_ORIGIN = "Origin";

// String separators and protocol-specific literals
static constexpr const char* HTTP_HEADER_SEPARATOR = ": ";
static constexpr const char* HTTP_SSE_EVENT_DELIMITER = "\n\n";
static constexpr const char* HTTP_SSE_DATA_PREFIX = "data: ";
static constexpr size_t HTTP_SSE_DATA_PREFIX_LEN = 6;
static constexpr const char* HTTP_TRANSPORT_TYPE = "http";

// Error messages
static constexpr const char* HTTP_ERR_INIT_CURL = "Failed to init curl";

MCP_NAMESPACE_END
