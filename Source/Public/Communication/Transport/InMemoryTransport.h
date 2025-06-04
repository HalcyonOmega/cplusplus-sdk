#pragma once

#include <condition_variable>
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
    void Start() override {
        _isRunning = true;
        if (_onStart) { _onStart(); }
    }

    void Stop() override {
        _isRunning = false;
        if (_onStop) { _onStop(); }
    }

    void Send(const std::string& message, const TransportSendOptions& options = {}) override {
        std::lock_guard<std::mutex> lock(_mutex);
        _messageQueue.push(message);
        _condition.notify_one();
    }

    void SetOnMessage(MessageCallback callback) override {
        _onMessage = std::move(callback);
    }

    void SetOnError(ErrorCallback callback) override {
        _onError = std::move(callback);
    }

    void SetOnClose(CloseCallback callback) override {
        _onClose = std::move(callback);
    }

    void SetOnStart(StartCallback callback) override {
        _onStart = std::move(callback);
    }

    void SetOnStop(StopCallback callback) override {
        _onStop = std::move(callback);
    }

    void WriteSSEEvent(const std::string& event, const std::string& data) override {
        std::string sseMessage = "event: " + event + "\ndata: " + data + "\n\n";
        Send(sseMessage);
    }

    // InMemoryTransport specific methods
    std::string Receive() {
        std::unique_lock<std::mutex> lock(_mutex);
        _condition.wait(lock, [this] { return !_messageQueue.empty() || !_isRunning; });

        if (!_isRunning) { return ""; }

        std::string message = _messageQueue.front();
        _messageQueue.pop();
        return message;
    }

    bool IsRunning() const {
        return _isRunning;
    }

    std::optional<std::string> GetSessionId() const override {
        return _sessionId;
    }

  private:
    std::weak_ptr<InMemoryTransport> _otherTransport;
    std::queue<std::string> _messageQueue;
    std::mutex _mutex;
    std::condition_variable _condition;
    bool _isRunning = false;

    MessageCallback _onMessage;
    ErrorCallback _onError;
    CloseCallback _onClose;
    StartCallback _onStart;
    StopCallback _onStop;

    // Session
    std::optional<std::string> _sessionId;
};

MCP_NAMESPACE_END