#pragma once

#include <string>

#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

// Base message types
struct MessageBase {
    std::string JSONRPC = "2.0";

    JKEY(JSONRPCKEY, JSONRPC, "jsonrpc")

    DEFINE_TYPE_JSON(MessageBase, JSONRPCKEY)
};

template <typename T>
concept ConcreteMessage = std::is_base_of_v<MessageBase, T>;

MCP_NAMESPACE_END