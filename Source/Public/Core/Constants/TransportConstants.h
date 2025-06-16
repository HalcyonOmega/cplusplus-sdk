#pragma once

#include "Macros.h"

MCP_NAMESPACE_BEGIN

// Transport Types
static constexpr const char* TSPT_STDIO = "stdio";
static constexpr const char* TSPT_HTTP = "http";

// Transport Headers
static constexpr const char* TSPT_SESSION_ID = "x-session-id";
static constexpr const char* TSPT_CONTENT_TYPE = "content-type";
static constexpr const char* TSPT_ACCEPT = "accept";

// Transport Content Types
static constexpr const char* TSPT_APP_JSON = "application/json";
static constexpr const char* TSPT_TEXT_EVENT_STREAM = "text/event-stream";
static constexpr const char* TSPT_APP_JSON_AND_EVENT_STREAM = "application/json, text/event-stream";

// Transport Event Stream Constants
static constexpr const char* TSPT_EVENT_DELIMITER = "\n\n";
static constexpr const char* TSPT_EVENT_PREFIX = "event: ";
static constexpr const char* TSPT_EVENT_DATA_PREFIX = "data: ";
static constexpr int TSPT_EVENT_PREFIX_LEN = 6;      // Length of "event: "
static constexpr int TSPT_EVENT_DATA_PREFIX_LEN = 6; // Length of "data: "

// Transport Error Messages
static constexpr const char* TRANSPORT_ERR_INVALID_UTF8 = "Invalid UTF-8 encoding in message";
static constexpr const char* TRANSPORT_ERR_INVALID_JSON_RPC = "Invalid JSON-RPC message format";
static constexpr const char* TRANSPORT_ERR_HTTP_REQUEST_FAILED = "HTTP request failed: ";
static constexpr const char* TRANSPORT_ERR_NOT_RUNNING = "Transport is not running";

// Transport error codes
constexpr const char* TRANSPORT_ERR_NOT_CONNECTED = "TRANSPORT_ERR_NOT_CONNECTED";
constexpr const char* TRANSPORT_ERR_CONNECTION_FAILED = "TRANSPORT_ERR_CONNECTION_FAILED";
constexpr const char* TRANSPORT_ERR_ALREADY_CONNECTED = "TRANSPORT_ERR_ALREADY_CONNECTED";
constexpr const char* TRANSPORT_ERR_INVALID_MESSAGE = "TRANSPORT_ERR_INVALID_MESSAGE";
constexpr const char* TRANSPORT_ERR_INVALID_PROTOCOL_VERSION =
    "TRANSPORT_ERR_INVALID_PROTOCOL_VERSION";
constexpr const char* TRANSPORT_ERR_UNSUPPORTED_CAPABILITY = "TRANSPORT_ERR_UNSUPPORTED_CAPABILITY";
constexpr const char* TRANSPORT_ERR_INVALID_STATE = "TRANSPORT_ERR_INVALID_STATE";
constexpr const char* TRANSPORT_ERR_RESUMPTION_FAILED = "TRANSPORT_ERR_RESUMPTION_FAILED";
constexpr const char* TRANSPORT_ERR_INVALID_RESUMPTION_TOKEN =
    "TRANSPORT_ERR_INVALID_RESUMPTION_TOKEN";

MCP_NAMESPACE_END