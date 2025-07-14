#include "JSONMessages.h"

MCP_NAMESPACE_BEGIN

std::optional<JSONData> ParseJSONMessage(const std::string& InRawMessage)
{
	try
	{
		return JSONData::parse(InRawMessage);
	}
	catch (const JSONData::parse_error&)
	{
		return std::nullopt;
	}
}

std::string ExtractMethod(const JSONData& InMessage)
{
	if (InMessage.contains("method") && InMessage["method"].is_string())
	{
		return InMessage["method"].get<std::string>();
	}
	return "";
}

std::optional<RequestID> ExtractRequestID(const JSONData& InMessage)
{
	if (InMessage.contains("id"))
	{
		if (InMessage["id"].is_string())
		{
			return RequestID{ InMessage["id"].get<std::string>() };
		}
		if (InMessage["id"].is_number())
		{
			return RequestID{ InMessage["id"].get<int64_t>() };
		}
	}
	return std::nullopt;
}

JSONData ExtractParams(const JSONData& InMessage)
{
	if (InMessage.contains("params"))
	{
		return InMessage["params"];
	}
	return JSONData::object();
}

bool IsValidJSONRPC(const JSONData& InMessage)
{
	if (!InMessage.is_object())
	{
		return false;
	}

	// Must have jsonrpc field with value "2.0"
	if (!InMessage.contains("jsonrpc") || InMessage["jsonrpc"] != "2.0")
	{
		return false;
	}

	// Must be either request, response, or notification
	const bool HasMethod = InMessage.contains("method");
	const bool HasId = InMessage.contains("id");
	const bool HasResult = InMessage.contains("result");
	const bool HasError = InMessage.contains("error");

	// Request: method + id + optional params
	if (HasMethod && HasId && !HasResult && !HasError)
	{
		return true;
	}

	// Response: id + (result XOR error)
	if (!HasMethod && HasId && HasResult != HasError)
	{
		return true;
	}

	// Notification: method + no id
	if (HasMethod && !HasId && !HasResult && !HasError)
	{
		return true;
	}

	return false;
}

std::optional<MessageType> GetValidMessageType(const JSONData& InMessage)
{
	if (InMessage.contains("id") && InMessage.contains("method"))
	{
		return MessageType::Request;
	}
	if (InMessage.contains("id") && InMessage.contains("result") && !InMessage.contains("error"))
	{
		return MessageType::Response;
	}
	if (InMessage.contains("id") && InMessage.contains("error"))
	{
		return MessageType::Error;
	}
	if (InMessage.contains("method") && !InMessage.contains("id"))
	{
		return MessageType::Notification;
	}
	return std::nullopt;
}

MCP_NAMESPACE_END