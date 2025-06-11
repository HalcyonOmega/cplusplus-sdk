#include "Communication/Messages.h"

#include <any>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>

#include "Core.h"

// TODO: @HalcyonOmega Split & or implement messagebase methods & other methods as appropriate

MCP_NAMESPACE_BEGIN

/* MessageID */
string_view MessageID::ToString() const {
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
        m_MessageID);
}

/* MessageParams */
string MessageParams::Serialize() const {
    // TODO: @HalcyonOmega Implement
    return MSG_NULL;
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
MessageID RequestMessage::GetMessageID() const {
    return m_ID;
};

string_view RequestMessage::GetMethod() const {
    return m_Method;
};

optional<const MessageParams*> RequestMessage::GetParams() const {
    if (m_Params.has_value()) { return m_Params.value().get(); }
    return std::nullopt;
};

JSON RequestMessage::ToJSON() const {
    JSON JSON_Object;
    JSON_Object[MSG_JSON_RPC] = GetJSONRPCVersion();
    JSON_Object[MSG_ID] = GetMessageID().ToString();
    JSON_Object[MSG_METHOD] = GetMethod();
    if (m_Params.has_value() && m_Params.value() != nullptr) {
        JSON_Object[MSG_PARAMS] = m_Params.value()->Serialize();
    }
    return JSON_Object;
}

unique_ptr<MessageBase> RequestMessage::FromJSON(const JSON& InJSON) {
    // Basic validation
    if (!IsRequestMessage(InJSON)) {
        throw std::invalid_argument("JSON does not represent a RequestMessage");
    }

    // Parse ID
    MessageID ParsedID = [&]() -> MessageID {
        const auto& JSON_ID = InJSON.at(MSG_ID);
        if (JSON_ID.is_string()) { return MessageID(JSON_ID.get<string>()); }
        if (JSON_ID.is_number_integer()) { return MessageID(JSON_ID.get<long long>()); }
        throw std::invalid_argument("Unsupported id type for RequestMessage");
    }();

    auto Method = InJSON.at(MSG_METHOD).get<string>();

    // Currently, Params parsing is not implemented – store as nullopt
    auto NewMessage = std::make_unique<RequestMessage>(std::move(Method));
    NewMessage->m_ID = std::move(ParsedID);
    return NewMessage;
}

string RequestMessage::Serialize() const {
    return MessageBase::Serialize();
}

unique_ptr<MessageBase> RequestMessage::Deserialize(string InString) {
    JSON Parsed = JSON::parse(std::move(InString));
    return FromJSON(Parsed);
}

/* Response Message */
MessageID ResponseMessage::GetMessageID() const {
    return m_ID;
};

const MessageParams* ResponseMessage::GetResult() const {
    if (m_Result) { return m_Result.get(); }
    return nullptr;
}

JSON ResponseMessage::ToJSON() const {
    JSON JSON_Object;
    JSON_Object[MSG_JSON_RPC] = GetJSONRPCVersion();
    JSON_Object[MSG_ID] = GetMessageID().ToString();
    if (const MessageParams* ResultOpt = GetResult(); ResultOpt != nullptr) {
        JSON_Object[MSG_RESULT] = ResultOpt->Serialize();
    }
    return JSON_Object;
}

unique_ptr<MessageBase> ResponseMessage::FromJSON(const JSON& InJSON) {
    if (!IsResponseMessage(InJSON)) {
        throw std::invalid_argument("JSON does not represent a ResponseMessage");
    }

    MessageID ParsedID = [&]() -> MessageID {
        const auto& JSON_ID = InJSON.at(MSG_ID);
        if (JSON_ID.is_string()) { return MessageID(JSON_ID.get<string>()); }
        if (JSON_ID.is_number_integer()) { return MessageID(JSON_ID.get<long long>()); }
        throw std::invalid_argument("Unsupported id type for ResponseMessage");
    }();

    // Result parsing not implemented – store nullptr
    auto NewMessage = std::make_unique<ResponseMessage>(std::move(ParsedID), nullptr);
    return NewMessage;
}

string ResponseMessage::Serialize() const {
    return MessageBase::Serialize();
}

unique_ptr<MessageBase> ResponseMessage::Deserialize(string InString) {
    JSON Parsed = JSON::parse(std::move(InString));
    return FromJSON(Parsed);
}

/* Notification Message */
string_view NotificationMessage::GetMethod() const {
    return m_Method;
};

optional<const MessageParams*> NotificationMessage::GetParams() const {
    if (m_Params.has_value()) { return m_Params.value().get(); }
    return std::nullopt;
};

JSON NotificationMessage::ToJSON() const {
    JSON JSON_Object;
    JSON_Object[MSG_JSON_RPC] = GetJSONRPCVersion();
    JSON_Object[MSG_METHOD] = GetMethod();
    if (m_Params.has_value() && m_Params.value() != nullptr) {
        JSON_Object[MSG_PARAMS] = m_Params.value()->Serialize();
    }
    return JSON_Object;
}

unique_ptr<MessageBase> NotificationMessage::FromJSON(const JSON& InJSON) {
    if (!IsNotificationMessage(InJSON)) {
        throw std::invalid_argument("JSON does not represent a NotificationMessage");
    }

    auto Method = InJSON.at(MSG_METHOD).get<string>();
    auto NewMessage = std::make_unique<NotificationMessage>(std::move(Method));
    return NewMessage;
}

string NotificationMessage::Serialize() const {
    return MessageBase::Serialize();
}

unique_ptr<MessageBase> NotificationMessage::Deserialize(string InString) {
    JSON Parsed = JSON::parse(std::move(InString));
    return FromJSON(Parsed);
}

/* ErrorMessage */
MessageID ErrorMessage::GetID() const {
    return m_ID;
};

ErrorParams ErrorMessage::GetError() const {
    return m_Error;
};

JSON ErrorMessage::ToJSON() const {
    JSON JSON_Object;
    JSON_Object[MSG_JSON_RPC] = GetJSONRPCVersion();
    JSON_Object[MSG_ID] = GetID().ToString();
    // TODO: Fix Accessor
    JSON_Object[MSG_ERROR] = GetError().Message;
    return JSON_Object;
}

unique_ptr<MessageBase> ErrorMessage::FromJSON(const JSON& InJSON) {
    if (!IsErrorMessage(InJSON)) {
        throw std::invalid_argument("JSON does not represent an ErrorMessage");
    }

    MessageID ParsedID = [&]() -> MessageID {
        const auto& JSON_ID = InJSON.at(MSG_ID);
        if (JSON_ID.is_string()) { return MessageID(JSON_ID.get<string>()); }
        if (JSON_ID.is_number_integer()) { return MessageID(JSON_ID.get<long long>()); }
        throw std::invalid_argument("Unsupported id type for ErrorMessage");
    }();

    Errors Code = InJSON.at(MSG_ERROR).at(MSG_CODE).get<Errors>();
    string Message = InJSON.at(MSG_ERROR).at(MSG_MESSAGE).get<string>();
    optional<any> Data = nullopt;
    if (InJSON.at(MSG_ERROR).contains(MSG_DATA)) { Data = InJSON.at(MSG_ERROR).at(MSG_DATA); }

    auto NewMessage =
        std::make_unique<ErrorMessage>(std::move(ParsedID), Code, std::move(Message), Data);
    return NewMessage;
}

string ErrorMessage::Serialize() const {
    return MessageBase::Serialize();
}

unique_ptr<MessageBase> ErrorMessage::Deserialize(string InString) {
    JSON Parsed = JSON::parse(std::move(InString));
    return FromJSON(Parsed);
}

MCP_NAMESPACE_END