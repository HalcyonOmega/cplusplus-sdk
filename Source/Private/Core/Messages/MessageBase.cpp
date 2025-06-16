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

MCP_NAMESPACE_END