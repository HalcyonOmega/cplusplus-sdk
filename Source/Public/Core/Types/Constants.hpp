#pragma once

#include <array>

#include "Core/Types/Common.hpp"

MCP_NAMESPACE_BEGIN

// MCP Constants
static constexpr const char* LATEST_PROTOCOL_VERSION = "2025-03-26";
static constexpr const std::array<const char*, 3> SUPPORTED_PROTOCOL_VERSIONS = {
  LATEST_PROTOCOL_VERSION,
  "2024-11-05",
  "2024-10-07",
};

// Message Constants
static constexpr const char* JSON_RPC_VERSION = "2.0";
static constexpr const char* MSG_KEY_JSONRPC = "jsonrpc";
static constexpr const char* MSG_KEY_ID = "id";
static constexpr const char* MSG_KEY_METHOD = "method";
static constexpr const char* MSG_KEY_PARAMS = "params";
static constexpr const char* MSG_KEY_RESULT = "result";
static constexpr const char* MSG_KEY_ERROR = "error";
static constexpr const char* MSG_KEY_CODE = "code";
static constexpr const char* MSG_KEY_MESSAGE = "message";
static constexpr const char* MSG_KEY_DATA = "data";
static constexpr const char* MSG_KEY_PROTOCOL_VERSION = "protocolVersion";	
static constexpr const char* MSG_KEY_CLIENT_INFO = "clientInfo";
static constexpr const char* MSG_KEY_NAME = "name";
static constexpr const char* MSG_KEY_VERSION = "version";
static constexpr const char* MSG_KEY_SERVER_INFO = "serverInfo";
static constexpr const char* MSG_KEY_CAPABILITIES = "capabilities";
static constexpr const char* MSG_KEY_PROMPTS = "prompts";
static constexpr const char* MSG_KEY_RESOURCES = "resources";
static constexpr const char* MSG_KEY_TOOLS = "tools";
static constexpr const char* MSG_KEY_LIST_CHANGED = "listChanged";
static constexpr const char* MSG_KEY_SUBSCRIBE = "subscribe";
static constexpr const char* MSG_KEY_CURSOR = "cursor";
static constexpr const char* MSG_KEY_NEXT_CURSOR = "nextCursor";
static constexpr const char* MSG_KEY_DESCRIPTION = "description";
static constexpr const char* MSG_KEY_INPUT_SCHEMA = "inputSchema";
static constexpr const char* MSG_KEY_ARGUMENTS = "arguments";
static constexpr const char* MSG_KEY_IS_ERROR = "isError";
static constexpr const char* MSG_KEY_CONTENT = "content";
static constexpr const char* MSG_KEY_TEXT = "text";
static constexpr const char* MSG_KEY_TYPE = "type";
static constexpr const char* MSG_KEY_MIME_TYPE = "mimeType";
static constexpr const char* MSG_KEY_URI = "uri";
static constexpr const char* MSG_KEY_BLOB = "blob";
static constexpr const char* MSG_KEY_RESOURCE = "resource";
static constexpr const char* MSG_KEY_PROGRESS_TOKEN = "progressToken";
static constexpr const char* MSG_KEY_META = "_meta";
static constexpr const char* MSG_KEY_PROGRESS = "progress";
static constexpr const char* MSG_KEY_TOTAL = "total";
static constexpr const char* MSG_KEY_REQUEST_ID = "requestId";

static constexpr const char* MTHD_INITIALIZE = "initialize";
static constexpr const char* MTHD_NOTIFICATION_INITIALIZED = "notifications/initialized";
static constexpr const char* MTHD_NOTIFICATION_CANCELLED = "notifications/cancelled";
static constexpr const char* MTHD_NOTIFICATION_PROGRESS = "notifications/progress";
static constexpr const char* MTHD_TOOLS_LIST = "tools/list";
static constexpr const char* MTHD_TOOLS_CALL = "tools/call";

static constexpr const char* CONST_TEXT = "text";
static constexpr const char* CONST_IMAGE = "image";
static constexpr const char* CONST_RESOURCE = "resource";


/* Errors */
static constexpr const char* ERRMSG_PARSE_ERROR = u8"parse error";
static constexpr const char* ERRMSG_INVALID_REQUEST = u8"invalid request";
static constexpr const char* ERRMSG_METHOD_NOT_FOUND = u8"method not found";
static constexpr const char* ERRMSG_INVALID_PARAMS = u8"invalid params";
static constexpr const char* ERRMSG_INTERNAL_ERROR = u8"internal error";

// JSON-RPC Error Codes
static constexpr const int ERRCODE_OK = 0;
static constexpr const int ERRCODE_PARSE_ERROR = -32700;
static constexpr const int ERRCODE_INVALID_REQUEST = -32600;
static constexpr const int ERRCODE_METHOD_NOT_FOUND = -32601;
static constexpr const int ERRCODE_INVALID_PARAMS = -32602;
static constexpr const int ERRCODE_INTERNAL_ERROR = -32603;

// Server Error Codes
static constexpr const int ERRCODE_SERVER_ERROR_FIRST = -32000;
static constexpr const int ERRCODE_INVALID_RESPONSE = -32001;
static constexpr const int ERRCODE_INVALID_NOTIFICATION = -32002;
static constexpr const int ERRCODE_INTERNAL_INPUT_TERMINATE = -32003;
static constexpr const int ERRCODE_INTERNAL_INPUT_ERROR = -32004;
static constexpr const int ERRCODE_INTERNAL_OUTPUT_ERROR = -32005;
static constexpr const int ERRCODE_SERVER_ERROR_LAST = -32099;

MCP_NAMESPACE_END