#include "NotificationBase.h"

MCP_NAMESPACE_BEGIN

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

MCP_NAMESPACE_END