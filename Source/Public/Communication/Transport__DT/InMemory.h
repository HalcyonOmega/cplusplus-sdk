#pragma once

#include "Communication/Messages.h"
#include "Core.h"

MCP_NAMESPACE_BEGIN

// TODO: Fix External Ref: Transport interface
// TODO: Fix External Ref: RequestID type
// TODO: Fix External Ref: AuthInfo type

struct QueuedMessage {
    MessageBase Message;
    optional<AuthInfo> AuthInfo;
};

/**
 * In-memory transport for creating clients and servers that talk to each other within the same
 * process.
 */
class InMemoryTransport : public Transport {
  private:
    InMemoryTransport* m_OtherTransport = nullptr;
    vector<QueuedMessage> m_MessageQueue;

  public:
    optional<function<void()>> OnClose;
    optional<function<void(const string&)>> OnError;
    optional<function<void(const MessageBase&, const optional<AuthInfo>&)>> OnMessage;
    optional<string> SessionID;

    InMemoryTransport() = default;

    // Disable copy constructor and assignment operator to prevent issues with linked pairs
    InMemoryTransport(const InMemoryTransport&) = delete;
    InMemoryTransport& operator=(const InMemoryTransport&) = delete;

    // Enable move constructor and assignment operator
    InMemoryTransport(InMemoryTransport&& InOther) noexcept
        : m_OtherTransport(InOther.m_OtherTransport), m_MessageQueue(move(InOther.m_MessageQueue)),
          OnClose(move(InOther.OnClose)), OnError(move(InOther.OnError)),
          OnMessage(move(InOther.OnMessage)), SessionID(move(InOther.SessionID)) {
        InOther.m_OtherTransport = nullptr;
        if (m_OtherTransport) { m_OtherTransport->m_OtherTransport = this; }
    }

    InMemoryTransport& operator=(InMemoryTransport&& InOther) noexcept {
        if (this != &InOther) {
            m_OtherTransport = InOther.m_OtherTransport;
            m_MessageQueue = move(InOther.m_MessageQueue);
            OnClose(move(InOther.OnClose)), OnError(move(InOther.OnError)),
                OnMessage(move(InOther.OnMessage)), SessionID(move(InOther.SessionID)) {
                InOther.m_OtherTransport = nullptr;
                if (m_OtherTransport) { m_OtherTransport->m_OtherTransport = this; }
            }
            return *this;
        }

        /**
         * Creates a pair of linked in-memory transports that can communicate with each other.
         * One should be passed to a Client and one to a Server.
         */
        static pair<InMemoryTransport, InMemoryTransport> CreateLinkedPair() {
            InMemoryTransport ClientTransport;
            InMemoryTransport ServerTransport;

            ClientTransport.m_OtherTransport = &ServerTransport;
            ServerTransport.m_OtherTransport = &ClientTransport;

            return make_pair(move(ClientTransport), move(ServerTransport));
        }
        void Start();

        void close();

        /**
         * Sends a message with optional auth info.
         * This is useful for testing authentication scenarios.
         */
        void Send(const MessageBase& InMessage, const optional<{
                                                    optional<RequestID> RelatedRequestID;
                                                    optional<AuthInfo> AuthInfo;
                                                }>& InOptions = nullopt);

        ~InMemoryTransport();
    }
};

MCP_NAMESPACE_END
