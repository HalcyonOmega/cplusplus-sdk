#pragma once

// Macros
#include "Macros.h"

// Standard library includes
#include <array>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

// Third-party includes
#include "Communication/Serialization/JSON.hpp"

MCP_NAMESPACE_BEGIN

using namespace std;

// Common using declarations
// using std::array;
// using std::async;
// using std::exception;
// using std::function;
// using std::future;
// using std::launch;
// using std::make_exception_ptr;
// using std::make_shared;
// using std::make_unique;
// using std::nullopt;
// using std::optional;
// using std::promise;
// using std::runtime_error;
// using std::shared_ptr;
// using std::string;
// using std::string_view;
// using std::unique_ptr;
// using std::unordered_map;
// using std::variant;
// using std::vector;

// JSON type alias
using JSON = nlohmann::JSON;

// Common type aliases used across MCP
using StreamID = string;
using EventID = string;
using RequestID = variant<string, int>; // A uniquely identifying ID for a
                                        // request in JSON-RPC.
using SessionID = string;
using Cursor = string; // An opaque token used to represent a cursor for pagination.

using Passthrough = unordered_map<string,
                                  JSON>; // A passthrough property is a property that is not
                                         // part of the schema, but is used to pass
                                         // additional information to the server or client.
// using ProgressToken = variant<string, int>; // A progress token, used to associate progress
// notifications with the original request.

MCP_NAMESPACE_END