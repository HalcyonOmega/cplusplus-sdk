#pragma once

#include <string>

#include "Macros.h"

MCP_NAMESPACE_BEGIN
std::string REGEXSearch; // Just this part => "[^\\"]*"

static constexpr const char* MSG_URITEMPLATE = "uri-template"; // TODO: Check duplicates with above

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
static constexpr const char* MSG_DESCRIPTION = "description";
static constexpr const char* MSG_REQUIRED = "required";
static constexpr const char* MSG_TYPE = "type";
static constexpr const char* MSG_PROPERTIES = "properties";
static constexpr const char* MSG_ADDITIONAL_PROPERTIES = "additionalProperties";
static constexpr const char* MSG_OBJECT = "object";
static constexpr const char* MSG_STRING = "string";
static constexpr const char* MSG_ARRAY = "array";
static constexpr const char* MSG_CONST = "const";
static constexpr const char* MSG_ITEMS = "items";
static constexpr const char* MSG_PROGRESS_TOKEN = "progressToken";
static constexpr const char* MSG_ANNOTATIONS = "annotations";
static constexpr const char* MSG_URI_TEMPLATE = "uriTemplate";
static constexpr const char* MSG_FORMAT = "format";
static constexpr const char* MSG_RESOURCE_TEMPLATES = "resourceTemplates";
static constexpr const char* MSG_URI = "uri";
static constexpr const char* MSG_BLOB = "blob";
static constexpr const char* MSG_RESOURCE = "resource";
static constexpr const char* MSG_PROGRESS = "progress";
static constexpr const char* MSG_CONTENTS = "contents";
static constexpr const char* MSG_BYTE = "byte";
static constexpr const char* MSG_PRIORITY = "priority";
static constexpr const char* MSG_MAXIMUM = "maximum";
static constexpr const char* MSG_MINIMUM = "minimum";
static constexpr const char* MSG_AUDIENCE = "audience";
static constexpr const char* MSG_NUMBER = "number";
static constexpr const char* MSG_CLIENT_ID = "client_id";
static constexpr const char* MSG_REDIRECT_URI = "redirect_uri";
static constexpr const char* MSG_RESPONSE_TYPE = "response_type";
static constexpr const char* MSG_CODE_CHALLENGE = "code_challenge";
static constexpr const char* MSG_CODE_CHALLENGE_METHOD = "code_challenge_method";
static constexpr const char* MSG_SCOPE = "scope";
static constexpr const char* MSG_STATE = "state";
static constexpr const char* MSG_HEADERS = "headers";
static constexpr const char* MSG_BODY = "body";
static constexpr const char* MSG_STATUS = "status";
static constexpr const char* MSG_LEVEL = "level";
static constexpr const char* MSG_LOGGER = "logger";

// JSON-RPC
static constexpr const char* MSG_JSON_RPC = "jsonrpc";
static constexpr const char* MSG_JSON_RPC_VERSION = "2.0";

// Message Constants
static constexpr const char* MSG_PROTOCOL_VERSION = "protocolVersion";
static constexpr const char* MSG_CLIENT_INFO = "clientInfo";
static constexpr const char* MSG_SERVER_INFO = "serverInfo";
static constexpr const char* MSG_CAPABILITIES = "capabilities";
static constexpr const char* MSG_PROMPTS = "prompts";
static constexpr const char* MSG_RESOURCES = "resources";
static constexpr const char* MSG_TOOLS = "tools";
static constexpr const char* MSG_LIST_CHANGED = "listChanged";
static constexpr const char* MSG_SUBSCRIBE = "subscribe";
static constexpr const char* MSG_CURSOR = "cursor";
static constexpr const char* MSG_NEXT_CURSOR = "nextCursor";
static constexpr const char* MSG_INPUT_SCHEMA = "inputSchema";
static constexpr const char* MSG_ARGUMENTS = "arguments";
static constexpr const char* MSG_IS_ERROR = "isError";
static constexpr const char* MSG_CONTENT = "content";
static constexpr const char* MSG_TEXT = "text";
static constexpr const char* MSG_IMAGE = "image";
static constexpr const char* MSG_AUDIO = "audio";
static constexpr const char* MSG_MIME_TYPE = "mimeType";
static constexpr const char* MSG_META = "_meta";
static constexpr const char* MSG_TOTAL = "total";
static constexpr const char* MSG_REQUEST_ID = "requestId";
static constexpr const char* MSG_REF_RESOURCE = "ref/resource";
static constexpr const char* MSG_REF_PROMPT = "ref/prompt";
static constexpr const char* MSG_MAXIMUM_SIZE = "4mb";

MCP_NAMESPACE_END