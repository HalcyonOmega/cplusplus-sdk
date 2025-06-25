#pragma once

#include <optional>
#include <string>
#include <utility>
#include <variant>

#include "CoreSDK/Common/Progress.h"
#include "CoreSDK/Messages/MessageBase.h"
#include "JSONProxy.h"
#include "UUIDProxy.h"

MCP_NAMESPACE_BEGIN

using RequestID = std::variant<std::string, int64_t>;

struct RequestParams {
    struct RequestParamsMeta {
        std::optional<ProgressToken>
            ProgressToken; // If specified, the caller is requesting out-of-band
                           // progress notifications for this request (as represented by
                           // notifications/progress). The value of this parameter is an opaque
                           // token that will be attached to any subsequent notifications. The
                           // receiver is not obligated to provide these notifications.

        JKEY(PROGRESS_TOKENKEY, ProgressToken, "progressToken")

        DEFINE_TYPE_JSON(RequestParamsMeta, PROGRESS_TOKENKEY)
    };

    std::optional<RequestParamsMeta> Meta;

    JKEY(METAKEY, Meta, "_meta")

    DEFINE_TYPE_JSON(RequestParams, METAKEY)
};

struct PaginatedRequestParams : RequestParams {
    std::optional<std::string> Cursor; // An opaque token representing the current pagination
                                       // position. If provided, the server should return
                                       // results starting after this cursor.
    JKEY(CURSORKEY, Cursor, "cursor")

    DEFINE_TYPE_JSON_DERIVED(PaginatedRequestParams, RequestParams, CURSORKEY)
};

struct RequestBase : MessageBase {
    RequestID ID;
    std::string Method;
    std::optional<RequestParams> Params;

    JKEY(IDKEY, ID, "id")
    JKEY(METHODKEY, Method, "method")
    JKEY(PARAMSKEY, Params, "params")

    DEFINE_TYPE_JSON_DERIVED(RequestBase, MessageBase, IDKEY, METHODKEY, PARAMSKEY)

    RequestBase(std::string_view InMethod, std::optional<RequestParams> InParams = std::nullopt)
        : ID(GenerateUUID()), Method(InMethod), Params(std::move(InParams)) {}

    RequestBase(RequestID InID, std::string_view InMethod,
                std::optional<RequestParams> InParams = std::nullopt)
        : ID(std::move(InID)), Method(InMethod), Params(std::move(InParams)) {}
};

MCP_NAMESPACE_END