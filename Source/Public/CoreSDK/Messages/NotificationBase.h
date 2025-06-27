#pragma once

#include <functional>
#include <optional>
#include <string>

#include "CoreSDK/Messages/MessageBase.h"
#include "JSONProxy.h"

struct NotificationBase;

MCP_NAMESPACE_BEGIN

using NotificationHandler = std::function<void(const NotificationBase& InNotification)>;

struct NotificationParams {
    struct NotificationParamsMeta {
        friend void to_json(JSONValue& InJSON,
                            const NotificationParamsMeta& InNotificationParamsMeta) {
            InJSON = JSONValue::object();
            (void)InNotificationParamsMeta;
        }

        friend void from_json(const JSONValue& InJSON,
                              NotificationParamsMeta& InNotificationParamsMeta) {
            (void)InJSON;
            InNotificationParamsMeta = NotificationParamsMeta{};
        }
    };

    std::optional<NotificationParamsMeta> Meta;

    JKEY(METAKEY, Meta, "_meta")

    DEFINE_TYPE_JSON(NotificationParams, METAKEY)
};

struct NotificationBase : MessageBase {
    std::string Method;
    std::optional<NotificationParams> Params;

    JKEY(METHODKEY, Method, "method")
    JKEY(PARAMSKEY, Params, "params")

    DEFINE_TYPE_JSON_DERIVED(NotificationBase, MessageBase, METHODKEY, PARAMSKEY)

    NotificationBase(std::string_view InMethod,
                     std::optional<NotificationParams> InParams = std::nullopt,
                     std::optional<NotificationHandler> InHandler = std::nullopt)
        : MessageBase(), Method(InMethod), Params(std::move(InParams)),
          Handler(std::move(InHandler)) {}

    std::optional<NotificationHandler> Handler;

    // Get typed params - cast the base Params to the derived notification's Params type
    template <typename TParamsType> [[nodiscard]] std::optional<TParamsType> GetParams() const {
        if (Params) { return static_cast<const TParamsType&>(*Params); }
        return std::nullopt;
    }
};

MCP_NAMESPACE_END