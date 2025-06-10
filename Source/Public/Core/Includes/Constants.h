#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// MCP Constants
static constexpr const char* LATEST_PROTOCOL_VERSION = "2025-03-26";
static constexpr const array<const char*, 3> SUPPORTED_PROTOCOL_VERSIONS = {
    LATEST_PROTOCOL_VERSION,
    "2024-11-05",
    "2024-10-07",
};

// URI Template Constants
static constexpr const size_t MAX_TEMPLATE_LENGTH = 1000000; // 1MB
static constexpr const size_t MAX_VARIABLE_LENGTH = 1000000; // 1MB
static constexpr const size_t MAX_TEMPLATE_EXPRESSIONS = 10000;
static constexpr const size_t MAX_REGEX_LENGTH = 1000000; // 1MB

// The default request timeout, in milliseconds.
static constexpr const int64_t DEFAULT_REQUEST_TIMEOUT_MSEC = 60000;

static constexpr const char* CONST_TEXT = "text";
static constexpr const char* CONST_IMAGE = "image";
static constexpr const char* CONST_AUDIO = "audio";
static constexpr const char* CONST_RESOURCE = "resource";

MCP_NAMESPACE_END