#include "MessageBase.h"

MCP_NAMESPACE_BEGIN

string MessageParams::Serialize() const {
    // TODO: @HalcyonOmega Implement
    return MSG_EMPTY;
}

string_view MessageBase::GetJSONRPCVersion() const {
    return m_JSONRPC;
}

string MessageBase::Serialize() const {
    return ToJSON().dump();
}

JSON MessageBase::ToJSON() const {
    // Empty message with just the JSON-RPC version
    JSON result;
    result[MSG_JSON_RPC] = m_JSONRPC;
    return result;
}

unique_ptr<MessageBase> MessageBase::FromJSON(const JSON& /*InJSON*/) {
    // Create a new empty MessageBase from JSON
    auto message = make_unique<MessageBase>();
    // Could validate JSON-RPC version here if needed
    return message;
}

unique_ptr<MessageBase> MessageBase::Deserialize(string InString) {
    try {
        JSON json = JSON::parse(InString);
        return FromJSON(json);
    } catch (...) {
        return nullptr; // Return null on parse error following MCP SDK pattern
    }
}

MCP_NAMESPACE_END