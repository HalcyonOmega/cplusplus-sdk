#pragma once

#include "Communication/Transport/EventStore.h"
#include "Core.h"
#include "Transport.h"

MCP_NAMESPACE_BEGIN

// In-memory implementation of the EventStore interface.
// Stores events in memory with their IDs for later retrieval.
class InMemoryEventStore : public EventStore {
  public:
    InMemoryEventStore() = default;
    ~InMemoryEventStore() override = default;

    void StoreEvent(const string& InEvent) override;
    vector<string> ReplayEventsAfter(const string& InLastEventID) override;

  private:
    string GenerateEventID();
    map<string, string> m_Events;
    mutex m_Mutex;
};

// In-memory transport for creating clients and servers that talk to each other within the same
// process.
class InMemoryTransport : public Transport {
  public:
    InMemoryTransport() = default;
    ~InMemoryTransport() override;

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

        // Creates a pair of linked in-memory transports that can communicate with each other.
        // One should be passed to a Client and one to a Server.
        static pair<InMemoryTransport, InMemoryTransport> CreateLinkedPair() {
            InMemoryTransport ClientTransport;
            InMemoryTransport ServerTransport;

            ClientTransport.m_OtherTransport = &ServerTransport;
            ServerTransport.m_OtherTransport = &ClientTransport;

            return make_pair(move(ClientTransport), move(ServerTransport));
        }

        // Sends a message with optional auth info. This is useful for testing authentication
        // scenarios.
        void Send(const MessageBase& InMessage, const optional<{
                                                    optional<RequestID> RelatedRequestID;
                                                    optional<AuthInfo> AuthInfo;
                                                }>& InOptions = nullopt);

        ~InMemoryTransport();
    }
    // Transport interface implementation
    future<void> Start() override;
    future<void> Close() override;
    future<void> Send(const MessageBase& InMessage,
                      const TransportSendOptions& InOptions = {}) override;
    void WriteSSEEvent(const string& InEvent, const string& InData) override;

    // New method for resumability support
    bool Resume(const string& InResumptionToken) override;

    // Creates a pair of linked in-memory transports that can communicate with each other.
    // One should be passed to a Client and one to a Server.
    static pair<shared_ptr<InMemoryTransport>, shared_ptr<InMemoryTransport>> CreateLinkedPair();

  private:
    struct QueuedMessage {
        shared_ptr<MessageBase> Message;
        optional<AuthInfo> AuthInfo;
    };

    weak_ptr<InMemoryTransport> m_OtherTransport;
    queue<QueuedMessage> m_MessageQueue;
    mutex m_QueueMutex;
};

MCP_NAMESPACE_END