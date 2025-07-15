#pragma once

#include <functional>
#include <optional>
#include <string>

#include "CoreSDK/Messages/MessageBase.h"
#include "CoreSDK/Messages/RequestBase.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

// Standard JSON-RPC 2.0 error codes
enum class Errors : int64_t
{
	ParseError = -32700,
	InvalidRequest = -32600,
	MethodNotFound = -32601,
	InvalidParams = -32602,
	InternalError = -32603,
	UnknownError = -32000
};

struct FErrorData
{
	Errors Code{ Errors::UnknownError };
	std::string Message{ "Unknown Error" };
	std::optional<JSONData> Data{ std::nullopt };

	JKEY(CODEKEY, Code, "code")
	JKEY(MESSAGEKEY, Message, "message")
	JKEY(DATAKEY, Data, "data")

	DEFINE_TYPE_JSON(FErrorData, CODEKEY, MESSAGEKEY, DATAKEY)

	FErrorData() = default;

	FErrorData(const Errors InCode,
		const std::string_view InMessage,
		const std::optional<JSONData>& InData = std::nullopt)
		: Code(InCode),
		  Message(InMessage),
		  Data(InData)
	{}
};

struct ErrorResponseBase : ResponseBase
{
	FErrorData ErrorData{};

	JKEY(ERRORKEY, ErrorData, "error")

	DEFINE_TYPE_JSON_DERIVED(ErrorResponseBase, ResponseBase, IDKEY, ERRORKEY)

	ErrorResponseBase() = default;
	~ErrorResponseBase() override = default;

	ErrorResponseBase(RequestID InID, FErrorData InError) : ResponseBase(std::move(InID)), ErrorData(std::move(InError))
	{}

	[[nodiscard]] Errors GetErrorCode() const { return ErrorData.Code; }
	[[nodiscard]] std::string_view GetErrorMessage() const { return ErrorData.Message; }
	[[nodiscard]] std::optional<JSONData> GetErrorData() const { return ErrorData.Data; }
};

using ErrorResponseHandler = std::function<void(const ErrorResponseBase& InError)>;

template <typename T>
concept ConcreteErrorResponse = std::is_base_of_v<ErrorResponseBase, T>;

inline ErrorResponseBase
ErrorParseError(RequestID InID, const std::string_view InMessage, const std::optional<JSONData>& InData = std::nullopt)
{
	return ErrorResponseBase{ std::move(InID), FErrorData(Errors::ParseError, InMessage, InData) };
}

inline ErrorResponseBase ErrorInvalidRequest(RequestID InID,
	const std::string_view InMessage,
	const std::optional<JSONData>& InData = std::nullopt)
{
	return ErrorResponseBase{ std::move(InID), FErrorData(Errors::InvalidRequest, InMessage, InData) };
}

inline ErrorResponseBase ErrorMethodNotFound(RequestID InID,
	const std::string_view InMessage,
	const std::optional<JSONData>& InData = std::nullopt)
{
	return ErrorResponseBase{ std::move(InID), FErrorData(Errors::MethodNotFound, InMessage, InData) };
}

inline ErrorResponseBase ErrorInvalidParams(RequestID InID,
	const std::string_view InMessage,
	const std::optional<JSONData>& InData = std::nullopt)
{
	return ErrorResponseBase{ std::move(InID), FErrorData(Errors::InvalidParams, InMessage, InData) };
}

inline ErrorResponseBase ErrorInternalError(RequestID InID,
	const std::string_view InMessage,
	const std::optional<JSONData>& InData = std::nullopt)
{
	return ErrorResponseBase{ std::move(InID), FErrorData(Errors::InternalError, InMessage, InData) };
}

MCP_NAMESPACE_END