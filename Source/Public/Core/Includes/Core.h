#pragma once

// Macros
#include "Macros.h"

// Standard library includes
#include <array>
#include <atomic>
#include <codecvt>
#include <condition_variable>
#include <functional>
#include <future>
#include <locale>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <variant>
#include <vector>

// Third-party includes
// #include "httplib.h"
#include "json.hpp"

MCP_NAMESPACE_BEGIN

using namespace std;

// MCP Constants
static constexpr const char* LATEST_PROTOCOL_VERSION = "2025-03-26";
static constexpr const array<const char*, 3> SUPPORTED_PROTOCOL_VERSIONS = {
    LATEST_PROTOCOL_VERSION,
    "2024-11-05",
    "2024-10-07",
};

// JSON type alias
using JSON = nlohmann::json;

// Common type aliases used across MCP
using StreamID = string;
using EventID = string;
using SessionID = string;

using AdditionalProperties = unordered_map<string, any>;
using AdditionalStrings = unordered_map<string, string>;
using AdditionalObjects = unordered_map<string,
                                        JSON>; // A passthrough property is a property that is not
                                               // part of the schema, but is used to pass
                                               // additional information to the server or client.

// TODO: @HalcyonOmega create URI, URIFile, & URITemplate classes
struct URI {
    string Value;
};

struct URITemplate {
    string Value;
};

struct URIFile {
    static constexpr const char* URI_FILE_PREFIX = "file://";
    string Value = URI_FILE_PREFIX; // This *must* start with "file://" for now.
};

// TODO: @HalcyonOmega create JSONSchema class. Needed to define ToolInputSchema and
// ToolOutputSchema.
class JSONSchema;

// Cursor {
//   MSG_DESCRIPTION : "An opaque token used to represent a cursor for
//   pagination.",
//                   MSG_TYPE : MSG_STRING
// };

// An opaque token used to represent a cursor for pagination.
using Cursor = string;

MCP_NAMESPACE_END