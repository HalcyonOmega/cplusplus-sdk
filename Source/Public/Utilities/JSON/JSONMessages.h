#pragma once

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Messages/RequestBase.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

enum class MessageType { Request, Response, Error, Notification };

[[nodiscard]] std::optional<JSONData> ParseJSONMessage(const std::string& InRawMessage);
[[nodiscard]] std::string ExtractMethod(const JSONData& InMessage);
[[nodiscard]] std::optional<RequestID> ExtractRequestID(const JSONData& InMessage);
[[nodiscard]] JSONData ExtractParams(const JSONData& InMessage);
[[nodiscard]] JSONData ExtractResult(const JSONData& InMessage);
[[nodiscard]] JSONData ExtractError(const JSONData& InMessage);
[[nodiscard]] bool IsValidJSONRPC(const JSONData& InMessage);
[[nodiscard]] std::optional<MessageType> GetValidMessageType(const JSONData& InMessage);

MCP_NAMESPACE_END