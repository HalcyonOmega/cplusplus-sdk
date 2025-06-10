#pragma once

#include "Macros.h"

MCP_NAMESPACE_BEGIN

// Message Keys
static constexpr const char* MSG_NULL = "";
static constexpr const char* MSG_ID = "id";
static constexpr const char* MSG_METHOD = "method";
static constexpr const char* MSG_PARAMS = "params";
static constexpr const char* MSG_RESULT = "result";
static constexpr const char* MSG_ERROR = "error";
static constexpr const char* MSG_CODE = "code";
static constexpr const char* MSG_MESSAGE = "message";
static constexpr const char* MSG_DATA = "data";
static constexpr const char* MSG_NAME = "name";
static constexpr const char* MSG_VERSION = "version";

// JSON-RPC
static constexpr const char* MSG_JSON_RPC = "jsonrpc";
static constexpr const char* MSG_JSON_RPC_VERSION = "2.0";

// Message Constants
static constexpr const char* MSG_PROTOCOL_VERSION = "protocolVersion";
static constexpr const char* MSG_CLIENT_INFO = "clientInfo";
static constexpr const char* MSG_SERVER_INFO = "serverInfo";
static constexpr const char* MSG_CAPABILITIES = "capabilities";
static constexpr const char* MSG_PROMPTS =
    "prompts"; // TODO: Need extensibility for keys - ex. 'prompts/list'
static constexpr const char* MSG_RESOURCES = "resources"; // TODO: Same as above
static constexpr const char* MSG_TOOLS = "tools";         // TODO: Same as above
static constexpr const char* MSG_LIST_CHANGED = "listChanged";
static constexpr const char* MSG_SUBSCRIBE = "subscribe";
static constexpr const char* MSG_CURSOR = "cursor";
static constexpr const char* MSG_NEXT_CURSOR = "nextCursor";
static constexpr const char* MSG_DESCRIPTION = "description";
static constexpr const char* MSG_INPUT_SCHEMA = "inputSchema";
static constexpr const char* MSG_ARGUMENTS = "arguments";
static constexpr const char* MSG_IS_ERROR = "isError";
static constexpr const char* MSG_CONTENT = "content";
static constexpr const char* MSG_TEXT = "text";
static constexpr const char* MSG_TYPE = "type";
static constexpr const char* MSG_MIME_TYPE = "mimeType";
static constexpr const char* MSG_URI = "uri";
static constexpr const char* MSG_BLOB = "blob";
static constexpr const char* MSG_RESOURCE = "resource";
static constexpr const char* MSG_PROGRESS_TOKEN = "progressToken";
static constexpr const char* MSG_META = "_meta";
static constexpr const char* MSG_PROGRESS = "progress";
static constexpr const char* MSG_TOTAL = "total";
static constexpr const char* MSG_REQUEST_ID = "requestId";
static constexpr const char* MSG_REF_RESOURCE = "ref/resource";
static constexpr const char* MSG_REF_PROMPT = "ref/prompt";

MCP_NAMESPACE_END