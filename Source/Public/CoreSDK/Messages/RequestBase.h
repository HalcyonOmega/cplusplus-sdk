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

MCP_NAMESPACE_BEGIN

// Forward declare concept from EventSignatures.h
template <typename T>
concept RequestHandlerConcept = requires(T handler, const RequestBase& request) {
    { handler(request) } -> std::same_as<void>;
};

// Keep type alias for storage - concepts can't be used as types directly
using RequestHandler = std::function<void(const RequestBase& InRequest)>;

struct RequestID {
    std::variant<std::string, int64_t> Value;

    RequestID() = default;
    RequestID(const std::string& InValue) : Value(InValue) {}
    RequestID(int64_t InValue) : Value(InValue) {}
    RequestID(const std::variant<std::string, int64_t>& InValue) : Value(InValue) {}

    std::string ToString() const {
        return std::visit(
            [](const auto& InValue) -> std::string {
                if constexpr (std::is_same_v<std::decay_t<decltype(InValue)>, std::string>) {
                    return InValue;
                } else {
                    return std::to_string(InValue);
                }
            },
            Value);
    }

    // Custom JSON serialization to serialize directly as the value, not as an object
    void to_json(JSONValue& InJSON) const {
        std::visit([&InJSON](const auto& InValue) { InJSON = InValue; }, Value);
    }

    void from_json(const JSONValue& InJSON) {
        if (InJSON.is_string()) {
            Value = InJSON.get<std::string>();
        } else if (InJSON.is_number_integer()) {
            Value = InJSON.get<int64_t>();
        } else {
            throw std::invalid_argument("RequestID must be string or integer");
        }
    }
};

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

    // Concept-based constructors that accept any callable matching the concept
    template <RequestHandlerConcept T>
    RequestBase(std::string_view InMethod, std::optional<RequestParams> InParams = std::nullopt,
                std::optional<T> InHandler = std::nullopt)
        : MessageBase(), ID(GenerateUUID()), Method(InMethod), Params(std::move(InParams)) {
        if (InHandler.has_value()) { Handler = RequestHandler(std::move(InHandler.value())); }
    }

    template <RequestHandlerConcept T>
    RequestBase(RequestID InID, std::string_view InMethod,
                std::optional<RequestParams> InParams = std::nullopt,
                std::optional<T> InHandler = std::nullopt)
        : MessageBase(), ID(std::move(InID)), Method(InMethod), Params(std::move(InParams)) {
        if (InHandler.has_value()) { Handler = RequestHandler(std::move(InHandler.value())); }
    }

    // Legacy constructors for backward compatibility
    RequestBase(std::string_view InMethod, std::optional<RequestParams> InParams = std::nullopt,
                std::optional<RequestHandler> InHandler = std::nullopt)
        : MessageBase(), ID(GenerateUUID()), Method(InMethod), Params(std::move(InParams)),
          Handler(std::move(InHandler)) {}

    RequestBase(RequestID InID, std::string_view InMethod,
                std::optional<RequestParams> InParams = std::nullopt,
                std::optional<RequestHandler> InHandler = std::nullopt)
        : MessageBase(), ID(std::move(InID)), Method(InMethod), Params(std::move(InParams)),
          Handler(std::move(InHandler)) {}

    // Use std::function for storage, not the concept directly
    std::optional<RequestHandler> Handler;
};

MCP_NAMESPACE_END