#pragma once

#include "Core/Types/Common.hpp"

namespace MCP::Types {

// Protocol Constants
const string LATEST_PROTOCOL_VERSION = "2025-03-26";
const vector<string> SUPPORTED_PROTOCOL_VERSIONS = [
  LATEST_PROTOCOL_VERSION,
  "2024-11-05",
  "2024-10-07",
];

// JSON-RPC Types
const string JSON_RPC_VERSION = "2.0";

} // namespace MCP::Types