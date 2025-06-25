#pragma once

#include "CoreSDK/Messages/MessageBase.h"

MCP_NAMESPACE_BEGIN

struct NotificationParams {
    struct NotificationParamsMeta {};

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
                     std::optional<NotificationParams> InParams = std::nullopt)
        : Method(InMethod), Params(std::move(InParams)) {}
};

MCP_NAMESPACE_END