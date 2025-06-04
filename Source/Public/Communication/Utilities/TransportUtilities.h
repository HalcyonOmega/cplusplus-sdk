#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

class TransportUtilities {
  public:
    static bool IsValidUTF8(const std::string& str);
    static bool IsValidJSONRPC(const std::string& str);
    static void Log(const std::string& message);
};

MCP_NAMESPACE_END