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
struct JSONSchema {
    JSON Value;
};

// Cursor {
//   MSG_DESCRIPTION : "An opaque token used to represent a cursor for
//   pagination.",
//                   MSG_TYPE : MSG_STRING
// };

// An opaque token used to represent a cursor for pagination.
using Cursor = string;

// TODO: @HalcyonOmega Implement proper URL class
class URL {
  public:
    string Href;
    string Origin;

    URL(const string& InURLString) : Href(InURLString) {
        // TODO: Proper URL parsing
        Origin = InURLString; // Simplified
    }

    URL(const string& InRelative, const URL& InBase) {
        // TODO: Proper relative URL resolution
        Href = InBase.Href + "/" + InRelative;
        Origin = InBase.Origin;
    }
};

// Platform-specific environment variables to inherit by default
#ifdef _WIN32
const vector<string> DEFAULT_INHERITED_ENV_VARS = {
    "APPDATA",     "HOMEDRIVE",  "HOMEPATH", "LOCALAPPDATA", "PATH",       "PROCESSOR_ARCHITECTURE",
    "SYSTEMDRIVE", "SYSTEMROOT", "TEMP",     "USERNAME",     "USERPROFILE"};
#else
const vector<string> DEFAULT_INHERITED_ENV_VARS = {"HOME",  "LOGNAME", "PATH",
                                                   "SHELL", "TERM",    "USER"};
#endif

// Returns a default environment object including only environment variables deemed safe to inherit.
inline unordered_map<string, string> GetDefaultEnvironment() {
    unordered_map<string, string> Env;

    for (const auto& key : DEFAULT_INHERITED_ENV_VARS) {
        const char* value = getenv(key.c_str());
        if (value == nullptr) { continue; }

        string valueStr(value);
        if (valueStr.starts_with("()")) {
            // Skip functions, which are a security risk.
            continue;
        }

        Env[key] = valueStr;
    }

    return Env;
}

MCP_NAMESPACE_END