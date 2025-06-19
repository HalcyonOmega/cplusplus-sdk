#include "ResponseBase.h"

MCP_NAMESPACE_BEGIN

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

MCP_NAMESPACE_END