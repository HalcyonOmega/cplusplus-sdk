#include "Communication/Messages.h"

#include <string_view>

#include "Core.h"

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
optional<unique_ptr<MessageParams>> RequestMessage::GetParams() const {
    if (m_Params.has_value()) { return m_Params.value(); }
    return nullopt;
};

JSON RequestMessage::ToJSON() const {
    JSON JSON_Object;
    JSON_Object[MSG_JSON_RPC] = GetJSONRPCVersion();
    JSON_Object[MSG_ID] = GetMessageID().ToString();
    JSON_Object[MSG_METHOD] = GetMethod();
    if (m_Params.has_value()) { JSON_Object[MSG_PARAMS] = m_Params.value()->Serialize(); }
    return JSON_Object;
}

/* Response Message */
MessageID ResponseMessage::GetMessageID() const {
    return m_ID;
};

MessageParams& ResponseMessage::GetResult() const {
    return *m_Result;
};

JSON ResponseMessage::ToJSON() const {
    JSON JSON_Object;
    JSON_Object[MSG_JSON_RPC] = GetJSONRPCVersion();
    JSON_Object[MSG_ID] = GetMessageID().ToString();
    JSON_Object[MSG_RESULT] = GetResult().Serialize();
    return JSON_Object;
}

/* Notification Message */
string_view NotificationMessage::GetMethod() const {
    return m_Method;
};

optional<MessageParams> NotificationMessage::GetParams() const {
    if (m_Params.has_value()) { return m_Params.value(); }
    return nullopt;
};

JSON NotificationMessage::ToJSON() const {
    JSON JSON_Object;
    JSON_Object[MSG_JSON_RPC] = GetJSONRPCVersion();
    JSON_Object[MSG_METHOD] = GetMethod();
    if (m_Params.has_value()) { JSON_Object[MSG_PARAMS] = m_Params.value()->Serialize(); }
    return JSON_Object;
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

ErrorMessage ErrorMessage::FromJSON(const JSON& json) {
    if (!json.contains(MSG_CODE) || !json.contains(MSG_MESSAGE)) {
        throw std::invalid_argument("Invalid error JSON: missing required fields");
    }

    optional<JSON> data;
    if (json.contains(MSG_DATA)) { data = json[MSG_DATA]; }

    return ErrorMessage(json[MSG_CODE].get<Errors>(), json[MSG_MESSAGE].get<string>(), data);
}

MCP_NAMESPACE_END