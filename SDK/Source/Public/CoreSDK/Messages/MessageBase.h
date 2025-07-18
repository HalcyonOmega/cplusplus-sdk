#pragma once

#include <string>

#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

// Base message types
struct MessageBase
{
	std::string JSONRPC{ "2.0" };

	JSON_KEY(JSONRPCKEY, JSONRPC, "jsonrpc")

	DEFINE_TYPE_JSON(MessageBase, JSONRPCKEY)

	virtual ~MessageBase() = default;
	MessageBase() = default;

	MessageBase(const MessageBase&) = default;
	MessageBase(MessageBase&&) = default;
	MessageBase& operator=(const MessageBase&) = default;
	MessageBase& operator=(MessageBase&&) = default;
};

template <typename T>
concept ConcreteMessage = std::is_base_of_v<MessageBase, T>;

MCP_NAMESPACE_END