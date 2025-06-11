#pragma once

#include "Communication/Transport/EventStore.h"
#include "Constants.h"
#include "Core.h"
#include "Transport.h"

MCP_NAMESPACE_BEGIN

/**
 * In-memory implementation of the EventStore interface.
 * Stores events in memory with their IDs for later retrieval.
 */
class InMemoryEventStore : public EventStore {
  public:
    InMemoryEventStore() = default;
    ~InMemoryEventStore() override = default;

    void StoreEvent(const std::string& InEvent) override;
    std::vector<std::string> ReplayEventsAfter(const std::string& InLastEventID) override;

  private:
    std::string GenerateEventID();
    std::map<std::string, std::string> m_Events;
    std::mutex m_Mutex;
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
    void Send(const std::string& InMessage, const TransportSendOptions& InOptions = {}) override;
    void SetOnMessage(MessageCallback InCallback) override;
    void SetOnError(ErrorCallback InCallback) override;
    void SetOnClose(CloseCallback InCallback) override;
    void SetOnStart(StartCallback InCallback) override;
    void SetOnStop(StopCallback InCallback) override;
    void WriteSSEEvent(const std::string& InEvent, const std::string& InData) override;

    // New method for resumability support
    bool Resume(const std::string& InResumptionToken) override;

    /**
     * Creates a pair of linked in-memory transports that can communicate with each other.
     * One should be passed to a Client and one to a Server.
     */
    static std::pair<std::shared_ptr<InMemoryTransport>, std::shared_ptr<InMemoryTransport>>
    CreateLinkedPair();

  private:
    struct QueuedMessage {
        std::string Message;
        std::optional<AuthInfo> AuthInfo;
    };

    std::weak_ptr<InMemoryTransport> m_OtherTransport;
    std::queue<QueuedMessage> m_MessageQueue;
    std::mutex m_QueueMutex;
    std::string m_SessionID;

    // Callbacks
    MessageCallback m_OnMessage;
    ErrorCallback m_OnError;
    CloseCallback m_OnClose;
    StartCallback m_OnStart;
    StopCallback m_OnStop;
};

MCP_NAMESPACE_END