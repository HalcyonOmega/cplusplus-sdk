#pragma once

#include "Constants.h"
#include "Core.h"
#include "Transport.hpp"

MCP_NAMESPACE_BEGIN

// TODO: Fix External Ref: WebSocket implementation (platform-specific)
// TODO: Fix External Ref: URL parsing (consider std::string or custom URL class)
// TODO: Fix External Ref: MessageBase validation schema

const string SUBPROTOCOL = "mcp";

/**
 * Client transport for WebSocket: this will connect to a server over the WebSocket protocol.
 */
class WebSocket_Client_Transport : public Transport {
  private:
    // TODO: Fix External Ref: WebSocket - replace with actual WebSocket implementation
    optional<void*> m_Socket; // Optional WebSocket instance (was _socket?: WebSocket)
    string m_URL;             // URL was not optional in TypeScript

    // Helper method to validate JSON-RPC message structure
    bool ValidateMessage(const JSON& InJSON) const;

    // Helper method to convert JSON to MessageBase
    optional<MessageBase> JSONToMessage(const JSON& InJSON) const;

    // Helper method to convert MessageBase to JSON
    JSON MessageToJSON(const MessageBase& InMessage) const;

  public:
    // Event handlers - optional callbacks matching TypeScript interface
    optional<function<void()>> OnClose;
    optional<function<void(const exception&)>> OnError;
    optional<function<void(const MessageBase&, const optional<AuthInfo>&)>> OnMessage;

    // Session ID as required by Transport interface
    optional<string> SessionID;

    explicit WebSocket_Client_Transport(const string& InURL) : m_URL(InURL), m_Socket(nullopt) {}

    future<void> Start() override;

    future<void> Close() override;

    // Updated Send method to match Transport interface signature
    future<void> Send(const MessageBase& InMessage,
                      const optional<TransportSendOptions>& InOptions = nullopt) override;

    // Legacy Send method for backward compatibility (matching original signature)
    future<void> Send(const MessageBase& InMessage);
};

MCP_NAMESPACE_END
