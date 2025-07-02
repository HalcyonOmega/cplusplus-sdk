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
        friend void to_json(JSONData& InJSON,
                            const NotificationParamsMeta& InNotificationParamsMeta) {
            InJSON = JSONData::object();
            (void)InNotificationParamsMeta;
        }

        friend void from_json(const JSONData& InJSON,
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
    std::optional<NotificationParams> ParamsData;

    JKEY(METHODKEY, Method, "method")
    JKEY(PARAMSKEY, ParamsData, "params")

    DEFINE_TYPE_JSON_DERIVED(NotificationBase, MessageBase, METHODKEY, PARAMSKEY)

    NotificationBase() = default;
    NotificationBase(std::string_view InMethod,
                     std::optional<NotificationParams> InParams = std::nullopt)
        : MessageBase(), Method(InMethod), ParamsData(std::move(InParams)) {}

    [[nodiscard]] std::string_view GetNotificationMethod() const {
        return Method;
    }
};

template <typename T>
concept ConcreteNotification = std::is_base_of_v<NotificationBase, T>;

// Get typed params - cast the base Params to the derived notification's Params type
template <typename TParamsType, ConcreteNotification T>
[[nodiscard]] std::optional<TParamsType> GetNotificationParams(T& InNotification) {
    if (InNotification.ParamsData) {
        return static_cast<const TParamsType&>(*InNotification.ParamsData);
    }
    return std::nullopt;
}

MCP_NAMESPACE_END