#include "RequestBase.h"

MCP_NAMESPACE_BEGIN

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

MCP_NAMESPACE_END