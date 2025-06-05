#pragma once

#include "Constants.h"
#include "Core.h"
#include "Transport.hpp"

MCP_NAMESPACE_BEGIN

// TODO: Fix External Ref: WebSocket implementation (platform-specific)
// TODO: Fix External Ref: URL parsing (consider std::string or custom URL class)
// TODO: Fix External Ref: JSON_RPC_Message validation schema

const string SUBPROTOCOL = "mcp";

/**
 * Client transport for WebSocket: this will connect to a server over the WebSocket protocol.
 */
class WebSocket_Client_Transport : public Transport {
  private:
    // TODO: Fix External Ref: WebSocket - replace with actual WebSocket implementation
    optional<void*> Socket_; // Optional WebSocket instance (was _socket?: WebSocket)
    string URL_;             // URL was not optional in TypeScript

    // Helper method to validate JSON-RPC message structure
    bool ValidateJSON_RPC_Message(const JSON& json) const;

    // Helper method to convert JSON to JSON_RPC_Message
    optional<JSONRPCMessage> JSONToMessage(const JSON& json) const;

    // Helper method to convert JSON_RPC_Message to JSON
    JSON MessageToJSON(const JSONRPCMessage& message) const;

  public:
    // Event handlers - optional callbacks matching TypeScript interface
    optional<function<void()>> OnClose;
    optional<function<void(const exception&)>> OnError;
    optional<function<void(const JSONRPCMessage&, const optional<AuthInfo>&)>> OnMessage;

    // Session ID as required by Transport interface
    optional<string> SessionID;

    explicit WebSocket_Client_Transport(const string& Url) : URL_(Url), Socket_(nullopt) {}

    future<void> Start() override;

    future<void> Close() override;

    // Updated Send method to match Transport interface signature
    future<void> Send(const JSONRPCMessage& Message,
                      const optional<TransportSendOptions>& Options = nullopt) override;

    // Legacy Send method for backward compatibility (matching original signature)
    future<void> Send(const JSONRPCMessage& Message);
};

MCP_NAMESPACE_END
