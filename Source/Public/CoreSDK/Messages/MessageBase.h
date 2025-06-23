#pragma once

#include <string_view>

#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

// Base message types
struct MessageBase {
    std::string_view JSONRPCVersion = "2.0";

    JKEY(JSONRPCKEY, JSONRPCVersion, "jsonrpc")

    DEFINE_TYPE_JSON(MessageBase, JSONRPCKEY)
};

MCP_NAMESPACE_END