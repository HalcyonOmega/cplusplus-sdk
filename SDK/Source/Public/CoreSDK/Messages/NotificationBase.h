#pragma once

#include <functional>
#include <optional>
#include <string>

#include "CoreSDK/Messages/MessageBase.h"
#include "JSONProxy.h"

struct NotificationBase;

MCP_NAMESPACE_BEGIN

using NotificationHandler = std::function<void(const NotificationBase& InNotification)>;

struct NotificationParams
{
	struct NotificationParamsMeta
	{
		friend void to_json(JSONData& InJSON, const NotificationParamsMeta& InNotificationParamsMeta)
		{
			InJSON = JSONData::object();
			(void)InNotificationParamsMeta;
		}

		friend void from_json(const JSONData& InJSON, NotificationParamsMeta& InNotificationParamsMeta)
		{
			(void)InJSON;
			InNotificationParamsMeta = NotificationParamsMeta{};
		}

		NotificationParamsMeta() = default;
		virtual ~NotificationParamsMeta() = default;
	};

	const std::optional<NotificationParamsMeta>& Meta{ std::nullopt };

	JSON_KEY(METAKEY, Meta, "_meta")

	DEFINE_TYPE_JSON(NotificationParams, METAKEY)

	virtual ~NotificationParams() = default;
	explicit NotificationParams(const std::optional<NotificationParamsMeta>& InMeta = std::nullopt) : Meta(InMeta) {}
};

// NotificationBase {
//   MSG_DESCRIPTION: "A notification which does not expect a response.",
//     MSG_PROPERTIES: {
//         MSG_JSON_RPC: {MSG_CONST: MSG_JSON_RPC_VERSION, MSG_TYPE: MSG_STRING},
//         MSG_METHOD: {MSG_TYPE: MSG_STRING},
//         MSG_PARAMS: {
//           MSG_ADDITIONAL_PROPERTIES: {},
//           MSG_PROPERTIES: {
//             MSG_META: {
//               MSG_ADDITIONAL_PROPERTIES: {},
//               MSG_DESCRIPTION: "This parameter name is reserved by MCP to allow "
//                               "clients and servers to attach additional "
//                               "metadata to their notifications.",
//               MSG_TYPE: MSG_OBJECT
//             }
//           },
//           MSG_TYPE: MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED: [ MSG_JSON_RPC, MSG_METHOD ],
//                      MSG_TYPE: MSG_OBJECT
// };

// A notification which does not expect a response. Supports JSON-RPC 2.0.
struct NotificationBase : MessageBase
{
	std::string Method{};
	std::optional<std::unique_ptr<NotificationParams>> ParamsData{ std::nullopt };

	JSON_KEY(METHODKEY, Method, "method")
	JSON_KEY(PARAMSKEY, ParamsData, "params")

	DEFINE_TYPE_JSON_DERIVED(NotificationBase, MessageBase, METHODKEY, PARAMSKEY)

	explicit NotificationBase(const std::string_view InMethod,
		std::optional<std::unique_ptr<NotificationParams>> InParams = std::nullopt)
		: MessageBase(),
		  Method(InMethod),
		  ParamsData(std::move(InParams))
	{}

	[[nodiscard]] std::string_view GetNotificationMethod() const { return Method; }

	NotificationBase() = default;
	~NotificationBase() override = default;
	NotificationBase(NotificationBase&&) = default;
	NotificationBase& operator=(NotificationBase&&) = default;
};

template <typename T>
concept ConcreteNotification = std::is_base_of_v<NotificationBase, T>;

// Get typed params - cast the base Params to the derived notification's Params type
template <typename TParamsType, ConcreteNotification T>
[[nodiscard]] std::optional<const TParamsType*> GetNotificationParams(T& InNotification)
{
	if (InNotification.ParamsData)
	{
		return { static_cast<const TParamsType*>(InNotification.ParamsData.value().get()) };
	}
	return std::nullopt;
}

MCP_NAMESPACE_END