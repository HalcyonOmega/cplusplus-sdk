#pragma once

#include <functional>
#include <optional>
#include <string>

#include "CoreSDK/Messages/MessageBase.h"
#include "CoreSDK/Messages/RequestBase.h"
#include "JSONProxy.h"

struct ResponseBase;

MCP_NAMESPACE_BEGIN

using ResponseHandler = std::function<void(const ResponseBase& InResponse)>;

// Result {
//   MSG_ADDITIONAL_PROPERTIES: {},
//         MSG_PROPERTIES: {
//         MSG_META: {
//           MSG_ADDITIONAL_PROPERTIES: {},
//           MSG_DESCRIPTION: "This result property is reserved by the protocol to allow clients
//           and servers to attach additional metadata to their responses.", MSG_TYPE: MSG_OBJECT
//         }
//       },
//         MSG_TYPE: MSG_OBJECT
// };

struct ResultParams
{
	std::optional<JSONData> Meta{ std::nullopt }; // The protocol reserves this result property to allow clients and
												  // servers to attach additional metadata to their responses.

	JKEY(METAKEY, Meta, "_meta")

	DEFINE_TYPE_JSON(ResultParams, METAKEY)

	ResultParams() = default;
	virtual ~ResultParams() = default;

	explicit ResultParams(const std::optional<JSONData>& InMeta = std::nullopt) : Meta(InMeta) {}
};

// PaginatedResult {
//   MSG_PROPERTIES: {
//     MSG_META: {
//       MSG_ADDITIONAL_PROPERTIES: {},
//       MSG_DESCRIPTION: "This result property is reserved by the protocol to allow clients and servers to attach
//       additional metadata to their responses.",
//       MSG_TYPE: MSG_OBJECT
//					},
//     MSG_NEXT_CURSOR: {
//       MSG_DESCRIPTION: "An opaque token representing the pagination position after the last returned result.
//       If present, there may be more results available.",
//       MSG_TYPE: MSG_STRING
//     }
//   },
//   MSG_TYPE: MSG_OBJECT
// };

struct PaginatedResultParams : ResultParams
{
	std::optional<std::string> NextCursor{ std::nullopt }; // An opaque token representing the next pagination
														   // position. If provided, the client should use this
														   // cursor to fetch the next page of results.

	JKEY(NEXT_CURSORKEY, NextCursor, "nextCursor")

	DEFINE_TYPE_JSON_DERIVED(PaginatedResultParams, ResultParams, NEXT_CURSORKEY)

	PaginatedResultParams() = default;
	~PaginatedResultParams() override = default;

	explicit PaginatedResultParams(const std::optional<std::string>& InNextCursor = std::nullopt,
		const std::optional<JSONData>& InMeta = std::nullopt)
		: ResultParams(InMeta),
		  NextCursor(InNextCursor)
	{}
};

// ResponseBase {
//   MSG_DESCRIPTION: "A successful (non-error) response to a request.",
//                   MSG_PROPERTIES: {
//                     MSG_ID: {"$ref": "#/definitions/RequestID"},
//                     MSG_JSON_RPC: {MSG_CONST: MSG_JSON_RPC_VERSION, MSG_TYPE: MSG_STRING},
//                     MSG_RESULT: {"$ref": "#/definitions/Result"}
//                   },
//                   MSG_REQUIRED: [ MSG_ID, MSG_JSON_RPC, MSG_RESULT ],
//                   MSG_TYPE: MSG_OBJECT
// };

// A successful (non-error) response to a request. Supports JSON-RPC 2.0.
struct ResponseBase : MessageBase
{
	RequestID ID{};
	std::unique_ptr<ResultParams> ResultData{ std::make_unique<ResultParams>(std::nullopt) };

	JKEY(IDKEY, ID, "id")
	JKEY(RESULTKEY, ResultData, "result")

	DEFINE_TYPE_JSON_DERIVED(ResponseBase, MessageBase, IDKEY, RESULTKEY)

	ResponseBase() = default;
	~ResponseBase() override = default;

	explicit ResponseBase(RequestID InID) : MessageBase(), ID(std::move(InID)) {}
	explicit ResponseBase(RequestID InID, std::unique_ptr<ResultParams> InResult)
		: MessageBase(),
		  ID(std::move(InID)),
		  ResultData(std::move(InResult))
	{}

	[[nodiscard]] RequestID GetRequestID() const { return ID; }
};

template <typename T>
concept ConcreteResponse = std::is_base_of_v<ResponseBase, T>;

template <typename F, typename T>
concept ExpectedResponseFunction
	= ConcreteResponse<T> && std::invocable<F, const T&> && std::same_as<std::invoke_result_t<F, const T&>, void>;

template <typename F>
concept UnexpectedResponseFunction
	= std::invocable<F, const JSONData&> && std::same_as<std::invoke_result_t<F, const JSONData&>, void>;

// Get typed result - cast the base Result to the derived response's Result type
template <typename TResultType, ConcreteResponse T> [[nodiscard]] TResultType* GetResponseResult(const T& InResponse)
{
	return static_cast<const TResultType*>(InResponse.ResultData.get());
}

MCP_NAMESPACE_END