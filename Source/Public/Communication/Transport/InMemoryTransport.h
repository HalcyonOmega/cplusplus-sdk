#pragma once

#include <memory>
#include <mutex>
#include <queue>

#include "Transport.h"

MCP_NAMESPACE_BEGIN

// Forward declarations
struct AuthInfo;

// Structure for queued messages with optional auth info
struct QueuedMessage {
    std::shared_ptr<MessageBase> message;
    std::shared_ptr<AuthInfo> authInfo;
};

/**
 * In-memory transport for creating clients and servers that talk to each other within the same
 * process. Useful for testing and local communication.
 */
class InMemoryTransport : public Transport {
  public:
    InMemoryTransport() = default;
    ~InMemoryTransport() override = default;

    /**
     * Creates a pair of linked in-memory transports that can communicate with each other.
     * One should be passed to a Client and one to a Server.
     */
    static std::pair<std::shared_ptr<InMemoryTransport>, std::shared_ptr<InMemoryTransport>>
    CreateLinkedPair() {
        auto clientTransport = std::make_shared<InMemoryTransport>();
        auto serverTransport = std::make_shared<InMemoryTransport>();
        clientTransport->_otherTransport = serverTransport;
        serverTransport->_otherTransport = clientTransport;
        return {clientTransport, serverTransport};
    }

    // Transport interface implementation
    std::future<void> Start() override {
        std::promise<void> promise;
        auto future = promise.get_future();

        // Process any messages that were queued before start was called
        std::lock_guard<std::mutex> lock(_queueMutex);
        while (!_messageQueue.empty()) {
            auto queuedMessage = std::move(_messageQueue.front());
            _messageQueue.pop();
            if (_messageCallback) {
                _messageCallback(*queuedMessage.message, queuedMessage.authInfo.get());
            }
        }

        promise.set_value();
        return future;
    }

    std::future<void> Send(const MessageBase& message,
                           const TransportSendOptions& options = {}) override {
        std::promise<void> promise;
        auto future = promise.get_future();

        auto other = _otherTransport.lock();
        if (!other) {
            promise.set_exception(std::make_exception_ptr(std::runtime_error("Not connected")));
            return future;
        }

        // Handle resumption token if provided
        if (options.resumptionToken && options.onResumptionToken) {
            options.onResumptionToken(*options.resumptionToken);
        }

        if (other->_messageCallback) {
            other->_messageCallback(message, nullptr);
        } else {
            std::lock_guard<std::mutex> lock(other->_queueMutex);
            other->_messageQueue.push({
                std::make_shared<MessageBase>(message),
                nullptr // authInfo
            });
        }

        promise.set_value();
        return future;
    }

    std::future<void> Close() override {
        std::promise<void> promise;
        auto future = promise.get_future();

        auto other = _otherTransport.lock();
        _otherTransport.reset();

        if (other) { other->Close(); }

        if (_closeCallback) { _closeCallback(); }

        promise.set_value();
        return future;
    }

    // Callback setters
    void SetCloseCallback(CloseCallback callback) override {
        _closeCallback = std::move(callback);
    }

    void SetErrorCallback(ErrorCallback callback) override {
        _errorCallback = std::move(callback);
    }

    void SetMessageCallback(MessageCallback callback) override {
        _messageCallback = std::move(callback);
    }

    std::optional<std::string> GetSessionId() const override {
        return _sessionId;
    }

  private:
    std::weak_ptr<InMemoryTransport> _otherTransport;
    std::queue<QueuedMessage> _messageQueue;
    std::mutex _queueMutex;

    // Callbacks
    CloseCallback _closeCallback;
    ErrorCallback _errorCallback;
    MessageCallback _messageCallback;

    // Session
    std::optional<std::string> _sessionId;
};

MCP_NAMESPACE_END