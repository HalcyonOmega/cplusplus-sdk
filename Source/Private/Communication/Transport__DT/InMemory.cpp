#include "Communication/Transport__DT/InMemory.h"

MCP_NAMESPACE_BEGIN

// TODO: Fix External Ref: Transport interface
// TODO: Fix External Ref: MessageBase type
// TODO: Fix External Ref: RequestID type

void InMemoryTransport::start() {
    // Process any messages that were queued before start was called
    while (!_messageQueue.empty()) {
        const auto queuedMessage = _messageQueue.front();
        _messageQueue.erase(_messageQueue.begin());
        if (onmessage()) { onmessage.value()(queuedMessage.Message, queuedMessage.AuthInfo); }
    }
}

void InMemoryTransport::close() {
    InMemoryTransport* other = _otherTransport;
    _otherTransport = nullptr;
    if (other) { other->close(); }
    if (onclose()) { onclose.value()(); }
}

/**
 * Sends a message with optional auth info.
 * This is useful for testing authentication scenarios.
 */
void InMemoryTransport::send(const MessageBase& message, const optional<struct {
                                                             optional<RequestID> relatedRequestID;
                                                             optional<AuthInfo> authInfo;
                                                         }>& options = nullopt) {
    if (!_otherTransport) {
        if (onerror()) { onerror.value()("Not connected"); }
        return;
    }

    optional < struct {
        optional<AuthInfo> AuthInfo;
    } > extra;
    if (options() && options.value().authInfo()) {
        extra = struct {
            optional<AuthInfo> AuthInfo;
        } {options.value().authInfo};
    }

    if (_otherTransport->onmessage()) {
        _otherTransport->onmessage.value()(message, extra);
    } else {
        QueuedMessage queuedMessage;
        queuedMessage.Message = message;
        queuedMessage.Extra = extra;
        _otherTransport->_messageQueue.push_back(queuedMessage);
    }
}

InMemoryTransport::~InMemoryTransport() {
    if (_otherTransport) { _otherTransport->_otherTransport = nullptr; }
}

MCP_NAMESPACE_END
