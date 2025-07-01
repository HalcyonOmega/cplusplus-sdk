#pragma once

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Messages/RequestBase.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

enum class MessageType { Request, Response, Error, Notification };

[[nodiscard]] std::optional<JSONValue> ParseJSONMessage(const std::string& InRawMessage);
[[nodiscard]] std::string ExtractMethod(const JSONValue& InMessage);
[[nodiscard]] std::optional<RequestID> ExtractRequestID(const JSONValue& InMessage);
[[nodiscard]] JSONValue ExtractParams(const JSONValue& InMessage);
[[nodiscard]] JSONValue ExtractResult(const JSONValue& InMessage);
[[nodiscard]] JSONValue ExtractError(const JSONValue& InMessage);
[[nodiscard]] bool IsValidJSONRPC(const JSONValue& InMessage);
[[nodiscard]] std::optional<MessageType> GetValidMessageType(const JSONValue& InMessage);

MCP_NAMESPACE_END