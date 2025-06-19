#include "ErrorBase.h"

MCP_NAMESPACE_BEGIN

RequestID ErrorBase::GetID() const {
    return m_ID;
};

ErrorBase::ErrorParams ErrorBase::GetError() const {
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