#pragma once

#include "Communication/Transport/ITransport.h"
#include "Macros.h"

MCP_NAMESPACE_BEGIN

// In-memory transport for creating clients and servers that talk to each other within the same
// process.
class InMemoryTransport : public ITransport {
  public:
    // === ITransport Implementation ===
    MCPTask_Void Connect() override;
    MCPTask_Void Disconnect() override;
    MCPTask_Void SendMessage(const MessageBase& InMessage) override;
    // === End ITransport Implementation ===

    // Creates a pair of linked in-memory transports that can communicate with each other.
    // One should be passed to a Client and one to a Server.
    static pair<shared_ptr<InMemoryTransport>, shared_ptr<InMemoryTransport>> CreateLinkedPair();

    struct QueuedMessage {
        shared_ptr<MessageBase> Message;
        optional<AuthInfo> AuthInfo;
    };

  private:
    shared_ptr<InMemoryTransport> m_OtherTransport;
    queue<QueuedMessage> m_MessageQueue;
    mutable mutex m_QueueMutex;

  public:
    // Disable copy constructor and assignment operator to prevent issues with linked pairs
    InMemoryTransport(const InMemoryTransport&) = delete;
    InMemoryTransport& operator=(const InMemoryTransport&) = delete;
};

MCP_NAMESPACE_END