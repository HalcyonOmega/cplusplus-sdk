#pragma once

#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <variant>

#include "CoreSDK/Common/Progress.h"
#include "CoreSDK/Messages/MessageBase.h"
#include "JSONProxy.h"
#include "UUIDProxy.h"

struct RequestBase;
class MCPContext;

MCP_NAMESPACE_BEGIN

using RequestHandler = std::function<void(const RequestBase& InRequest)>;

// RequestID {
//   MSG_DESCRIPTION: "A uniquely identifying ID for a request in JSON-RPC.",
//                   MSG_TYPE: [ MSG_STRING, MSG_INTEGER ]
// };

// A uniquely identifying ID for a request in JSON-RPC.
struct RequestID
{
	std::variant<std::string, int64_t> Value;

	RequestID() : Value("") {}
	explicit RequestID(const std::string& InValue) : Value(InValue) {}
	explicit RequestID(int64_t InValue) : Value(InValue) {}
	explicit RequestID(const std::variant<std::string, int64_t>& InValue) : Value(InValue) {}

	[[nodiscard]] std::string ToString() const
	{
		return std::visit(
			[]<typename T>(const T& InValue) -> std::string
			{
				if constexpr (std::is_same_v<std::decay_t<T>, std::string>)
				{
					return InValue;
				}
				else
				{
					return std::to_string(InValue);
				}
			},
			Value);
	}

	friend void to_json(JSONData& InJSON, const RequestID& InRequestID) { InJSON = InRequestID.ToString(); }

	friend void from_json(const JSONData& InJSON, RequestID& InRequestID)
	{
		if (InJSON.is_string())
		{
			InRequestID.Value = InJSON.get<std::string>();
		}
		else if (InJSON.is_number_integer())
		{
			InRequestID.Value = InJSON.get<int64_t>();
		}
		else
		{
			throw std::invalid_argument("RequestID must be string or integer");
		}
	}
};

struct RequestParams
{
	struct RequestParamsMeta
	{
		std::optional<MCP::ProgressToken> ProgressToken{
			std::nullopt
		}; // If specified, the caller is requesting out-of-band progress notifications for this request (as represented
		   // by notifications/progress). The value of this parameter is an opaque token that will be attached to any
		   // subsequent notifications. The receiver is not obligated to provide these notifications.

		JKEY(PROGRESS_TOKENKEY, ProgressToken, "progressToken")

		DEFINE_TYPE_JSON(RequestParamsMeta, PROGRESS_TOKENKEY)

		RequestParamsMeta() = default;
		virtual ~RequestParamsMeta() = default;

		explicit RequestParamsMeta(const std::optional<MCP::ProgressToken>& InProgressToken = std::nullopt)
			: ProgressToken(InProgressToken)
		{}
	};

	std::optional<RequestParamsMeta> Meta{ std::nullopt };

	JKEY(METAKEY, Meta, "_meta")

	DEFINE_TYPE_JSON(RequestParams, METAKEY)

	RequestParams() = default;
	virtual ~RequestParams() = default;

	explicit RequestParams(const std::optional<RequestParamsMeta>& InMeta = std::nullopt) : Meta(InMeta) {}
};

// PaginatedRequest {
//   MSG_PROPERTIES: {
//     MSG_METHOD: {MSG_TYPE: MSG_STRING},
//     MSG_PARAMS: {
//       MSG_PROPERTIES: {
//         MSG_CURSOR: {
//           MSG_DESCRIPTION:
//               "An opaque token representing the current pagination position. If provided, the
//               server should return results starting after this cursor.",
//           MSG_TYPE: MSG_STRING
//         }
//       },
//       MSG_TYPE: MSG_OBJECT
//     }
//   },
//                  MSG_REQUIRED: [MSG_METHOD],
//                               MSG_TYPE: MSG_OBJECT
// };

struct PaginatedRequestParams : RequestParams
{
	std::optional<std::string> Cursor{ std::nullopt }; // An opaque token representing the current pagination
													   // position. If provided, the server should return
													   // results starting after this cursor.
	JKEY(CURSORKEY, Cursor, "cursor")

	DEFINE_TYPE_JSON_DERIVED(PaginatedRequestParams, RequestParams, CURSORKEY)

	PaginatedRequestParams() = default;
	~PaginatedRequestParams() override = default;

	explicit PaginatedRequestParams(const std::optional<std::string>& InCursor = std::nullopt,
		const std::optional<RequestParamsMeta>& InMeta = std::nullopt)
		: RequestParams(InMeta),
		  Cursor(InCursor)
	{}
};

// RequestBase {
//   MSG_DESCRIPTION : "A request that expects a response.",
//   MSG_PROPERTIES  : {
//         MSG_ID : {"$ref" : "#/definitions/RequestID"},
//         MSG_JSON_RPC : {MSG_CONST : MSG_JSON_RPC_VERSION, MSG_TYPE : MSG_STRING},
//         MSG_METHOD : {MSG_TYPE : MSG_STRING},
//         MSG_PARAMS : {
//           MSG_ADDITIONAL_PROPERTIES : {},
//           MSG_PROPERTIES : {
//             MSG_META : {
//               MSG_PROPERTIES : {
//                 MSG_PROGRESS_TOKEN : {
//                   "$ref" : "#/definitions/ProgressToken",
//                   MSG_DESCRIPTION:
//                       "If specified, the caller is requesting out-of-band progress notifications for this request (as
//                       represented by notifications/progress). The value of this parameter is an opaque token that
//                       will be attached to any subsequent notifications. The receiver is not obligated to
//                       provide these notifications."
//                 }
//               },
//               MSG_TYPE: MSG_OBJECT
//             }
//           },
//           MSG_TYPE: MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED: [ MSG_ID, MSG_JSON_RPC, MSG_METHOD ],
//                      MSG_TYPE: MSG_OBJECT
// };

// A request that expects a response. Supports JSON-RPC 2.0.
struct RequestBase : MessageBase
{
	RequestID ID{};
	std::string Method{ "DefaultRequest" };
	std::optional<std::unique_ptr<RequestParams>> ParamsData{ std::nullopt };

	JKEY(IDKEY, ID, "id")
	JKEY(METHODKEY, Method, "method")
	JKEY(PARAMSKEY, ParamsData, "params")

	DEFINE_TYPE_JSON_DERIVED(RequestBase, MessageBase, IDKEY, METHODKEY, PARAMSKEY)

	RequestBase() = default;
	~RequestBase() override = default;

	explicit RequestBase(const std::string_view InMethod,
		std::optional<std::unique_ptr<RequestParams>> InParams = std::nullopt)
		: MessageBase(),
		  ID(GenerateUUID()),
		  Method(InMethod),
		  ParamsData(std::move(InParams))
	{}

	RequestBase(RequestID InID,
		const std::string_view InMethod,
		std::optional<std::unique_ptr<RequestParams>> InParams = std::nullopt)
		: MessageBase(),
		  ID(std::move(InID)),
		  Method(InMethod),
		  ParamsData(std::move(InParams))
	{}

	[[nodiscard]] RequestID GetRequestID() const { return ID; }

	[[nodiscard]] std::string_view GetRequestMethod() const { return Method; }
};

template <typename T>
concept ConcreteRequest = std::is_base_of_v<RequestBase, T>;

// Get typed params - cast the base Params to the derived request's Params type
template <typename TParamsType, ConcreteRequest T>
[[nodiscard]] std::optional<const TParamsType*> GetRequestParams(const T& InRequest)
{
	if (InRequest.ParamsData)
	{
		return { static_cast<const TParamsType*>(InRequest.ParamsData.value().get()) };
	}
	return std::nullopt;
}

MCP_NAMESPACE_END