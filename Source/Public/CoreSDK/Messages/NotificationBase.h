#pragma once

#include <functional>
#include <optional>
#include <string>

#include "CoreSDK/Messages/MessageBase.h"
#include "JSONProxy.h"

struct NotificationBase;

MCP_NAMESPACE_BEGIN

// Forward declare concept from EventSignatures.h
template <typename T>
concept NotificationHandlerConcept = requires(T handler, const NotificationBase& notification) {
    { handler(notification) } -> std::same_as<void>;
};

// Keep type alias for storage compatibility
using NotificationHandler = std::function<void(const NotificationBase& InNotification)>;

struct NotificationParams {
    struct NotificationParamsMeta {
        // Custom JSON serialization to serialize directly as the value, not as an object
        void to_json(JSONValue& InJSON) const {
            (void)InJSON;
        }

        void from_json(const JSONValue& InJSON) {
            (void)InJSON;
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

    // Concept-based constructor
    template <NotificationHandlerConcept T>
    NotificationBase(std::string_view InMethod,
                     std::optional<NotificationParams> InParams = std::nullopt,
                     std::optional<T> InHandler = std::nullopt)
        : MessageBase(), Method(InMethod), Params(std::move(InParams)) {
        if (InHandler.has_value()) { Handler = NotificationHandler(std::move(InHandler.value())); }
    }

    // Legacy constructor for backward compatibility
    NotificationBase(std::string_view InMethod,
                     std::optional<NotificationParams> InParams = std::nullopt,
                     std::optional<NotificationHandler> InHandler = std::nullopt)
        : MessageBase(), Method(InMethod), Params(std::move(InParams)),
          Handler(std::move(InHandler)) {}

    std::optional<NotificationHandler> Handler;
};

MCP_NAMESPACE_END