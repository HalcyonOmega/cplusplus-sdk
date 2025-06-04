#pragma once

#include <memory>
#include <mutex>
#include <queue>

#include "Constants.h"
#include "Core.h"
#include "Transport.h"

namespace MCP {

// Forward declarations
struct AuthInfo {
    std::string token;
    std::string type;
};

/**
 * In-memory transport for creating clients and servers that talk to each other within the same
 * process.
 */
class InMemoryTransport : public Transport {
  public:
    InMemoryTransport();
    ~InMemoryTransport() override;

    // Transport interface implementation
    void Start() override;
    void Stop() override;
    void Send(const std::string& message, const TransportSendOptions& options = {}) override;
    void SetOnMessage(MessageCallback callback) override;
    void SetOnError(ErrorCallback callback) override;
    void SetOnClose(CloseCallback callback) override;
    void SetOnStart(StartCallback callback) override;
    void SetOnStop(StopCallback callback) override;
    void WriteSSEEvent(const std::string& event, const std::string& data) override;

    /**
     * Creates a pair of linked in-memory transports that can communicate with each other.
     * One should be passed to a Client and one to a Server.
     */
    static std::pair<std::shared_ptr<InMemoryTransport>, std::shared_ptr<InMemoryTransport>>
    CreateLinkedPair();

  private:
    struct QueuedMessage {
        std::string message;
        std::optional<AuthInfo> authInfo;
    };

    std::weak_ptr<InMemoryTransport> _otherTransport;
    std::queue<QueuedMessage> _messageQueue;
    std::mutex _queueMutex;
    std::string _sessionId;

    // Callbacks
    MessageCallback _onMessage;
    ErrorCallback _onError;
    CloseCallback _onClose;
    StartCallback _onStart;
    StopCallback _onStop;
};

} // namespace MCP