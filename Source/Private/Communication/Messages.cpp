#include "Communication/Messages.h"

#include <any>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>

#include "Core.h"

// TODO: @HalcyonOmega Split & or implement messagebase methods & other methods as appropriate

MCP_NAMESPACE_BEGIN

/* RequestID */
string_view RequestID::ToString() const {
    return visit(
        [](const auto& VisitID) -> string_view {
            if constexpr (is_same_v<decay_t<decltype(VisitID)>, string>) {
                return VisitID;
            } else {
                static thread_local string Buffer;
                Buffer = to_string(VisitID);
                return Buffer;
            }
        },
        m_RequestID);
}

/* MessageParams */
string MessageParams::Serialize() const {
    // TODO: @HalcyonOmega Implement
    return MSG_EMPTY;
}

/* MessageBase */
string_view MessageBase::GetJSONRPCVersion() const {
    return m_JSONRPC;
}

// MessageBase Helpers
string MessageBase::Serialize() const {
    return ToJSON().dump();
}

/* Request Message */
RequestID RequestBase::GetRequestID() const {
    return m_ID;
};

string_view RequestBase::GetMethod() const {
    return m_Method;
};

optional<const MessageParams*> RequestBase::GetParams() const {
    if (m_Params()) { return m_Params.value().get(); }
    return std::nullopt;
};

JSON RequestBase::ToJSON() const {
    JSON JSON_Object;
    JSON_Object[MSG_JSON_RPC] = GetJSONRPCVersion();
    JSON_Object[MSG_ID] = GetRequestID().ToString();
    JSON_Object[MSG_METHOD] = GetMethod();
    if (m_Params() && m_Params.value() != nullptr) {
        JSON_Object[MSG_PARAMS] = m_Params.value()->Serialize();
    }
    return JSON_Object;
}

unique_ptr<MessageBase> RequestBase::FromJSON(const JSON& InJSON) {
    // Basic validation
    if (!IsRequestBase(InJSON)) {
        throw std::invalid_argument("JSON does not represent a RequestBase");
    }

    // Parse ID
    RequestID ParsedID = [&]() -> RequestID {
        const auto& JSON_ID = InJSON.at(MSG_ID);
        if (JSON_ID.is_string()) { return RequestID(JSON_ID.get<string>()); }
        if (JSON_ID.is_number_integer()) { return RequestID(JSON_ID.get<long long>()); }
        throw std::invalid_argument("Unsupported id type for RequestBase");
    }();

    auto Method = InJSON.at(MSG_METHOD).get<string>();

    // Currently, Params parsing is not implemented – store as nullopt
    auto NewMessage = std::make_unique<RequestBase>(std::move(Method));
    NewMessage->m_ID = std::move(ParsedID);
    return NewMessage;
}

string RequestBase::Serialize() const {
    return MessageBase::Serialize();
}

unique_ptr<MessageBase> RequestBase::Deserialize(string InString) {
    JSON Parsed = JSON::parse(std::move(InString));
    return FromJSON(Parsed);
}

/* Response Message */
RequestID ResponseBase::GetRequestID() const {
    return m_ID;
};

const MessageParams* ResponseBase::GetResult() const {
    if (m_Result) { return m_Result.get(); }
    return nullptr;
}

JSON ResponseBase::ToJSON() const {
    JSON JSON_Object;
    JSON_Object[MSG_JSON_RPC] = GetJSONRPCVersion();
    JSON_Object[MSG_ID] = GetRequestID().ToString();
    if (const MessageParams* ResultOpt = GetResult(); ResultOpt != nullptr) {
        JSON_Object[MSG_RESULT] = ResultOpt->Serialize();
    }
    return JSON_Object;
}

unique_ptr<MessageBase> ResponseBase::FromJSON(const JSON& InJSON) {
    if (!IsResponseBase(InJSON)) {
        throw std::invalid_argument("JSON does not represent a ResponseBase");
    }

    RequestID ParsedID = [&]() -> RequestID {
        const auto& JSON_ID = InJSON.at(MSG_ID);
        if (JSON_ID.is_string()) { return RequestID(JSON_ID.get<string>()); }
        if (JSON_ID.is_number_integer()) { return RequestID(JSON_ID.get<long long>()); }
        throw std::invalid_argument("Unsupported id type for ResponseBase");
    }();

    // Result parsing not implemented – store nullptr
    auto NewMessage = std::make_unique<ResponseBase>(std::move(ParsedID), nullptr);
    return NewMessage;
}

string ResponseBase::Serialize() const {
    return MessageBase::Serialize();
}

unique_ptr<MessageBase> ResponseBase::Deserialize(string InString) {
    JSON Parsed = JSON::parse(std::move(InString));
    return FromJSON(Parsed);
}

/* Notification Message */
string_view NotificationBase::GetMethod() const {
    return m_Method;
};

optional<const MessageParams*> NotificationBase::GetParams() const {
    if (m_Params()) { return m_Params.value().get(); }
    return std::nullopt;
};

JSON NotificationBase::ToJSON() const {
    JSON JSON_Object;
    JSON_Object[MSG_JSON_RPC] = GetJSONRPCVersion();
    JSON_Object[MSG_METHOD] = GetMethod();
    if (m_Params() && m_Params.value() != nullptr) {
        JSON_Object[MSG_PARAMS] = m_Params.value()->Serialize();
    }
    return JSON_Object;
}

unique_ptr<MessageBase> NotificationBase::FromJSON(const JSON& InJSON) {
    if (!IsNotificationBase(InJSON)) {
        throw std::invalid_argument("JSON does not represent a NotificationBase");
    }

    auto Method = InJSON.at(MSG_METHOD).get<string>();
    auto NewMessage = std::make_unique<NotificationBase>(std::move(Method));
    return NewMessage;
}

string NotificationBase::Serialize() const {
    return MessageBase::Serialize();
}

unique_ptr<MessageBase> NotificationBase::Deserialize(string InString) {
    JSON Parsed = JSON::parse(std::move(InString));
    return FromJSON(Parsed);
}

/* ErrorBase */
RequestID ErrorBase::GetID() const {
    return m_ID;
};

ErrorParams ErrorBase::GetError() const {
    return m_Error;
};

JSON ErrorBase::ToJSON() const {
    JSON JSON_Object;
    JSON_Object[MSG_JSON_RPC] = GetJSONRPCVersion();
    JSON_Object[MSG_ID] = GetID().ToString();
    // TODO: Fix Accessor
    JSON_Object[MSG_ERROR] = GetError().Message;
    return JSON_Object;
}

unique_ptr<MessageBase> ErrorBase::FromJSON(const JSON& InJSON) {
    if (!IsErrorBase(InJSON)) {
        throw std::invalid_argument("JSON does not represent an ErrorBase");
    }

    RequestID ParsedID = [&]() -> RequestID {
        const auto& JSON_ID = InJSON.at(MSG_ID);
        if (JSON_ID.is_string()) { return RequestID(JSON_ID.get<string>()); }
        if (JSON_ID.is_number_integer()) { return RequestID(JSON_ID.get<long long>()); }
        throw std::invalid_argument("Unsupported id type for ErrorBase");
    }();

    Errors Code = InJSON.at(MSG_ERROR).at(MSG_CODE).get<Errors>();
    string Message = InJSON.at(MSG_ERROR).at(MSG_MESSAGE).get<string>();
    optional<any> Data = nullopt;
    if (InJSON.at(MSG_ERROR).contains(MSG_DATA)) { Data = InJSON.at(MSG_ERROR).at(MSG_DATA); }

    auto NewMessage =
        std::make_unique<ErrorBase>(std::move(ParsedID), Code, std::move(Message), Data);
    return NewMessage;
}

string ErrorBase::Serialize() const {
    return MessageBase::Serialize();
}

unique_ptr<MessageBase> ErrorBase::Deserialize(string InString) {
    JSON Parsed = JSON::parse(std::move(InString));
    return FromJSON(Parsed);
}

MCP_NAMESPACE_END