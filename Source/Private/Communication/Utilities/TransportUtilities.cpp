#include "Communication/Utilities/TransportUtilities.h"

#include "Core.h"

MCP_NAMESPACE_BEGIN

bool TransportUtilities::IsValidUTF8(const std::string& str) {
    // TODO: Implement UTF-8 validation
    // For now, just return true
    return true;
}

bool TransportUtilities::IsValidJSONRPC(const std::string& str) {
    try {
        auto json = JSON::parse(str);
        return json.is_object() && json.contains("jsonrpc") && json["jsonrpc"] == "2.0";
    } catch (const std::exception&) { return false; }
}

void TransportUtilities::Log(const std::string& message) {
    std::cerr << "[MCP] " << message << std::endl;
}

MCP_NAMESPACE_END