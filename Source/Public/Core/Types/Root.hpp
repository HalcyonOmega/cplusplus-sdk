#pragma once

#include "Core.h"

namespace MCP::Types {

// Represents a root directory or file that the server can operate on.
struct Root {
    // The URI identifying the root. This *must* start with file:// for now.
    string.startsWith("file://") URI;

    // An optional name for the root.
    optional<string> Name;
    Passthrough Additional;
};

MCP_NAMESPACE_END