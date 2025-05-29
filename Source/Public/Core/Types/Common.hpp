#pragma once

// Standard library includes
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <vector>
#include <optional>
#include <future>
#include <random>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <string_view>
#include <variant>

// Third-party includes
#include "../Utilities/JSON/JSON.hpp"

MCP_NAMESPACE_BEGIN

// Common using declarations
using std::string;
using std::vector;
using std::unordered_map;
using std::optional;
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;
using std::make_unique;
using std::function;
using std::future;
using std::async;
using std::launch;
using std::runtime_error;
using std::exception;
using std::nullopt;
using std::promise;
using std::make_exception_ptr;
using std::variant;
using std::string_view;

// JSON type alias
using JSON = nlohmann::json;

// Common type aliases used across MCP
using StreamID = string;
using EventID = string;
using RequestID = variant<string, int>; // A uniquely identifying ID for a request in JSON-RPC.
using SessionID = string;
using Cursor = string; // An opaque token used to represent a cursor for pagination.


using Passthrough = unordered_map<string, JSON>; // A passthrough property is a property that is not part of the schema, but is used to pass additional information to the server or client.
using ProgressToken = variant<string, int>; // A progress token, used to associate progress notifications with the original request.

} // namespace MCP