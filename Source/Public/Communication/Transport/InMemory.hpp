#pragma once

#include "../Core/Common.hpp"

namespace MCP::Transport {

// TODO: Fix External Ref: Transport interface
// TODO: Fix External Ref: JSON_RPC_Message type
// TODO: Fix External Ref: RequestID type
// TODO: Fix External Ref: AuthInfo type

struct QueuedMessage {
  JSON_RPC_Message Message;
  optional<struct {optional<AuthInfo> AuthInfo;}> Extra;
};

/**
 * In-memory transport for creating clients and servers that talk to each other within the same process.
 */
class InMemoryTransport : public Transport {
private:
    InMemoryTransport* _otherTransport = nullptr;
    vector<QueuedMessage> _messageQueue;

public:
    optional<function<void()>> onclose;
    optional<function<void(const string&)>> onerror;
    optional<function<void(const JSON_RPC_Message&, const optional<struct { optional<AuthInfo> AuthInfo; }>&)>> onmessage;
    optional<string> SessionID;

    InMemoryTransport() = default;

    // Disable copy constructor and assignment operator to prevent issues with linked pairs
    InMemoryTransport(const InMemoryTransport&) = delete;
    InMemoryTransport& operator=(const InMemoryTransport&) = delete;

    // Enable move constructor and assignment operator
    InMemoryTransport(InMemoryTransport&& other) noexcept
        : _otherTransport(other._otherTransport)
        , _messageQueue(move(other._messageQueue))
        , onclose(move(other.onclose))
        , onerror(move(other.onerror))
        , onmessage(move(other.onmessage))
        , SessionID(move(other.SessionID))
    {
        other._otherTransport = nullptr;
        if (_otherTransport) {
            _otherTransport->_otherTransport = this;
        }
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
            if (_otherTransport) {
                _otherTransport->_otherTransport = this;
            }
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

    void start() {
        // Process any messages that were queued before start was called
        while (!_messageQueue.empty()) {
            const auto queuedMessage = _messageQueue.front();
            _messageQueue.erase(_messageQueue.begin());
            if (onmessage.has_value()) {
                onmessage.value()(queuedMessage.Message, queuedMessage.Extra);
            }
        }
    }

    void close() {
        InMemoryTransport* other = _otherTransport;
        _otherTransport = nullptr;
        if (other) {
            other->close();
        }
        if (onclose.has_value()) {
            onclose.value()();
        }
    }

    /**
     * Sends a message with optional auth info.
     * This is useful for testing authentication scenarios.
     */
    void send(const JSON_RPC_Message& message, const optional<struct { optional<RequestID> relatedRequestID; optional<AuthInfo> authInfo; }>& options = nullopt) {
        if (!_otherTransport) {
            if (onerror.has_value()) {
                onerror.value()("Not connected");
            }
            return;
        }

        optional<struct { optional<AuthInfo> AuthInfo; }> extra;
        if (options.has_value() && options.value().authInfo.has_value()) {
            extra = struct { optional<AuthInfo> AuthInfo; }{ options.value().authInfo };
        }

        if (_otherTransport->onmessage.has_value()) {
            _otherTransport->onmessage.value()(message, extra);
        } else {
            QueuedMessage queuedMessage;
            queuedMessage.Message = message;
            queuedMessage.Extra = extra;
            _otherTransport->_messageQueue.push_back(queuedMessage);
        }
    }

    ~InMemoryTransport() {
        if (_otherTransport) {
            _otherTransport->_otherTransport = nullptr;
        }
    }
};

} // namespace MCP::Transport
