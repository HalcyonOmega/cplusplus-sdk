#pragma once

#include "Core/Types/Common.hpp"

namespace MCP::Types {

// Describes the name and version of an MCP implementation.
struct Implementation {
  string Name;
  string Version;
};

MCP_NAMESPACE_END