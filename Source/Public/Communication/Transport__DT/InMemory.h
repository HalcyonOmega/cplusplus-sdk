#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// TODO: Fix External Ref: Transport interface
// TODO: Fix External Ref: JSON_RPC_Message type
// TODO: Fix External Ref: RequestID type
// TODO: Fix External Ref: AuthInfo type

struct QueuedMessage {
    JSONRPCMessage Message;
    optional<AuthInfo> AuthInfo;
};

/**
 * In-memory transport for creating clients and servers that talk to each other within the same
 * process.
 */
class InMemoryTransport : public Transport {
  private:
    InMemoryTransport* _otherTransport = nullptr;
    vector<QueuedMessage> _messageQueue;

  public:
    optional<function<void()>> onclose;
    optional<function<void(const string&)>> onerror;
    optional<function<void(const JSONRPCMessage&, const optional<AuthInfo>&)>> onmessage;
    optional<string> SessionID;

    InMemoryTransport() = default;

    // Disable copy constructor and assignment operator to prevent issues with linked pairs
    InMemoryTransport(const InMemoryTransport&) = delete;
    InMemoryTransport& operator=(const InMemoryTransport&) = delete;

    // Enable move constructor and assignment operator
    InMemoryTransport(InMemoryTransport&& other) noexcept
        : _otherTransport(other._otherTransport), _messageQueue(move(other._messageQueue)),
          onclose(move(other.onclose)), onerror(move(other.onerror)),
          onmessage(move(other.onmessage)), SessionID(move(other.SessionID)) {
        other._otherTransport = nullptr;
        if (_otherTransport) { _otherTransport->_otherTransport = this; }
    }

    InMemoryTransport& operator=(InMemoryTransport&& other) noexcept {
        if (this != &other) {
            _otherTransport = other._otherTransport;
            _messageQueue = move(other._messageQueue);
            onclose = move(other.onclose);
            onerror = move(other.onerror);
            onmessage = move(other.onmessage);
            SessionID = move(other.SessionID);

            other._otherTransport = nullptr;
            if (_otherTransport) { _otherTransport->_otherTransport = this; }
        }
        return *this;
    }

    /**
     * Creates a pair of linked in-memory transports that can communicate with each other.
     * One should be passed to a Client and one to a Server.
     */
    static pair<InMemoryTransport, InMemoryTransport> createLinkedPair() {
        InMemoryTransport clientTransport;
        InMemoryTransport serverTransport;

        clientTransport._otherTransport = &serverTransport;
        serverTransport._otherTransport = &clientTransport;

        return make_pair(move(clientTransport), move(serverTransport));
    }
    void start();

    void close();

    /**
     * Sends a message with optional auth info.
     * This is useful for testing authentication scenarios.
     */
    void send(const JSONRPCMessage& message, const optional<{
                                                 optional<RequestID> relatedRequestID;
                                                 optional<AuthInfo> authInfo;
                                             }>& options = nullopt);

    ~InMemoryTransport();
};

MCP_NAMESPACE_END
