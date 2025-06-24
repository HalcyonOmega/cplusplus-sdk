#pragma once

#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

[[nodiscard]] std::optional<JSONValue> ParseJSONMessage(const std::string& InRawMessage);
[[nodiscard]] bool IsRequest(const JSONValue& InMessage);
[[nodiscard]] bool IsResponse(const JSONValue& InMessage);
[[nodiscard]] bool IsNotification(const JSONValue& InMessage);
[[nodiscard]] std::string ExtractMethod(const JSONValue& InMessage);
[[nodiscard]] std::string ExtractRequestID(const JSONValue& InMessage);
[[nodiscard]] JSONValue ExtractParams(const JSONValue& InMessage);
[[nodiscard]] JSONValue ExtractResult(const JSONValue& InMessage);
[[nodiscard]] JSONValue ExtractError(const JSONValue& InMessage);
[[nodiscard]] bool IsValidJSONRPC(const JSONValue& InMessage);

MCP_NAMESPACE_END