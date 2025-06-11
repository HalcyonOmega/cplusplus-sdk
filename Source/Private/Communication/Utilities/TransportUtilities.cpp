#include "Communication/Utilities/TransportUtilities.h"

#include "Core.h"
#include "Core/Constants/MessageConstants.h"
#include "Core/Constants/TransportConstants.h"
#include "json.hpp"

MCP_NAMESPACE_BEGIN

bool IsValidJSONRPC(const std::string& message) {
    try {
        auto json = JSON::parse(message);

        // Check for required JSON-RPC fields
        if (!json.contains(MSG_JSON_RPC) || !json.contains(MSG_METHOD)) { return false; }

        // Validate jsonrpc version
        if (json[MSG_JSON_RPC] != MSG_JSON_RPC_VERSION) { return false; }

        return true;
    } catch (const JSON::parse_error&) { return false; }
}

bool IsValidUTF8(const std::string& message) {
    // Simple UTF-8 validation
    // This is a basic check - in production, use a proper UTF-8 validation library
    for (size_t i = 0; i < message.length(); i++) {
        unsigned char c = message[i];
        if (c <= 0x7F) {
            // Single byte character
            continue;
        } else if (c <= 0xDF && i + 1 < message.length()) {
            // Two byte character
            if ((message[i + 1] & 0xC0) != 0x80) return false;
            i++;
        } else if (c <= 0xEF && i + 2 < message.length()) {
            // Three byte character
            if ((message[i + 1] & 0xC0) != 0x80 || (message[i + 2] & 0xC0) != 0x80) return false;
            i += 2;
        } else if (c <= 0xF7 && i + 3 < message.length()) {
            // Four byte character
            if ((message[i + 1] & 0xC0) != 0x80 || (message[i + 2] & 0xC0) != 0x80
                || (message[i + 3] & 0xC0) != 0x80)
                return false;
            i += 3;
        } else {
            return false;
        }
    }
    return true;
}

bool IsValidProtocolVersion(const std::string& version) {
    return version == TRANSPORT_PROTOCOL_VERSION;
}

bool IsValidState(bool isRunning, bool isConnected) {
    // Basic state validation
    // - Can't be connected if not running
    // - Can be running but not connected (e.g., during connection)
    return !isConnected || isRunning;
}

MCP_NAMESPACE_END