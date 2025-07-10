#pragma once

#include <functional>
#include <optional>
#include <string>

#include "CoreSDK/Messages/MessageBase.h"
#include "CoreSDK/Messages/RequestBase.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

struct FErrorData
{
	int64_t Code;
	std::string Message;
	std::optional<JSONData> Data;

	JKEY(CODEKEY, Code, "code")
	JKEY(MESSAGEKEY, Message, "message")
	JKEY(DATAKEY, Data, "data")

	DEFINE_TYPE_JSON(FErrorData, CODEKEY, MESSAGEKEY, DATAKEY)

	FErrorData(int64_t InCode, std::string_view InMessage, std::optional<JSONData> InData = std::nullopt)
		: Code(std::move(InCode)),
		  Message(std::move(InMessage)),
		  Data(std::move(InData))
	{}
};

// Standard JSON-RPC 2.0 error codes
namespace ErrorCodes
{
	static constexpr int64_t PARSE_ERROR = -32700;
	static constexpr int64_t INVALID_REQUEST = -32600;
	static constexpr int64_t METHOD_NOT_FOUND = -32601;
	static constexpr int64_t INVALID_PARAMS = -32602;
	static constexpr int64_t INTERNAL_ERROR = -32603;
} // namespace ErrorCodes

struct ErrorResponseBase : ResponseBase
{
	FErrorData ErrorData;

	JKEY(ERRORKEY, ErrorData, "error")

	DEFINE_TYPE_JSON_DERIVED(ErrorResponseBase, ResponseBase, IDKEY, ERRORKEY)

	ErrorResponseBase(RequestID InID, FErrorData InError) : ResponseBase(std::move(InID)), ErrorData(std::move(InError))
	{}

	[[nodiscard]] int64_t GetErrorCode() const { return ErrorData.Code; }

	[[nodiscard]] std::string_view GetErrorMessage() const { return ErrorData.Message; }

	[[nodiscard]] std::optional<JSONData> GetErrorData() const { return ErrorData.Data; }
};

using ErrorResponseHandler = std::function<void(const ErrorResponseBase& InError)>;

template <typename T>
concept ConcreteErrorResponse = std::is_base_of_v<ErrorResponseBase, T>;

inline ErrorResponseBase
ErrorParseError(RequestID InID, std::string_view InMessage, std::optional<JSONData> InData = std::nullopt)
{
	return ErrorResponseBase(std::move(InID),
		FErrorData(ErrorCodes::PARSE_ERROR, std::move(InMessage), std::move(InData)));
}
inline ErrorResponseBase
ErrorInvalidRequest(RequestID InID, std::string_view InMessage, std::optional<JSONData> InData = std::nullopt)
{
	return ErrorResponseBase(std::move(InID),
		FErrorData(ErrorCodes::INVALID_REQUEST, std::move(InMessage), std::move(InData)));
}

inline ErrorResponseBase
ErrorMethodNotFound(RequestID InID, std::string_view InMessage, std::optional<JSONData> InData = std::nullopt)
{
	return ErrorResponseBase(std::move(InID),
		FErrorData(ErrorCodes::METHOD_NOT_FOUND, std::move(InMessage), std::move(InData)));
}
inline ErrorResponseBase
ErrorInvalidParams(RequestID InID, std::string_view InMessage, std::optional<JSONData> InData = std::nullopt)
{
	return ErrorResponseBase(std::move(InID),
		FErrorData(ErrorCodes::INVALID_PARAMS, std::move(InMessage), std::move(InData)));
}

inline ErrorResponseBase
ErrorInternalError(RequestID InID, std::string_view InMessage, std::optional<JSONData> InData = std::nullopt)
{
	return ErrorResponseBase(std::move(InID),
		FErrorData(ErrorCodes::INTERNAL_ERROR, std::move(InMessage), std::move(InData)));
}

MCP_NAMESPACE_END