#include "Communication/Transport/MCP_StdioTransport.h"

#include "Core.h"

MCP_NAMESPACE_BEGIN

MCP_StdioTransport::MCP_StdioTransport() = default;
MCP_StdioTransport::~MCP_StdioTransport() = default;

bool MCP_StdioTransport::SendMessage(const MCP_MessageBase& /*message*/) {
    // TODO: Implement stdio message sending
    return false;
}

std::unique_ptr<MCP_MessageBase> MCP_StdioTransport::ReceiveMessage() {
    // TODO: Implement stdio message receiving
    return nullptr;
}

void MCP_StdioTransport::Start() {
    // TODO: Implement stdio transport start
}

void MCP_StdioTransport::Stop() {
    // TODO: Implement stdio transport stop
}

bool MCP_StdioTransport::IsOpen() const {
    // TODO: Implement stdio transport open state
    return false;
}

std::string MCP_StdioTransport::GetTransportType() const {
    return "stdio";
}

MCP_NAMESPACE_END
