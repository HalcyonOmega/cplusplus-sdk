#include "Communication/Transport/InMemoryTransport.h"

#include <random>
#include <sstream>

#include "Constants.h"
#include "Core.h"
#include "UUID/UUIDLayer.h"

MCP_NAMESPACE_BEGIN

// InMemoryEventStore implementation
void InMemoryEventStore::StoreEvent(const std::string& event) {
    std::lock_guard<std::mutex> Lock(m_Mutex);
    std::string EventID = GenerateEventID();
    m_Events[EventID] = event;
}

std::vector<std::string> InMemoryEventStore::ReplayEventsAfter(const std::string& LastEventID) {
    std::lock_guard<std::mutex> Lock(m_Mutex);
    std::vector<std::string> Events;
    bool FoundLastEvent = LastEventID.empty();

    for (const auto& [ID, Event] : m_Events) {
        if (FoundLastEvent) {
            Events.push_back(Event);
        } else if (ID == LastEventID) {
            FoundLastEvent = true;
        }
    }

    return Events;
}

std::string InMemoryEventStore::GenerateEventID() {
    return std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
}

InMemoryTransport::InMemoryTransport() {
    m_SessionID = GenerateUUID();
}

InMemoryTransport::~InMemoryTransport() {
    Stop();
}

void InMemoryTransport::Start() {
    // Process any messages that were queued before start was called
    std::lock_guard<std::mutex> Lock(m_QueueMutex);
    while (!m_MessageQueue.empty()) {
        const auto& QueuedMessage = m_MessageQueue.front();
        if (m_OnMessage) {
            m_OnMessage(QueuedMessage.Message,
                        QueuedMessage.AuthInfo ? &*QueuedMessage.AuthInfo : nullptr);
        }
        m_MessageQueue.pop();
    }
    if (m_OnStart) { m_OnStart(); }
}

void InMemoryTransport::Stop() {
    if (auto Other = m_OtherTransport.lock()) { Other->m_OtherTransport.reset(); }
    m_OtherTransport.reset();
    if (m_OnStop) { m_OnStop(); }
    if (m_OnClose) { m_OnClose(); }
}

void InMemoryTransport::Send(const std::string& InMessage, const TransportSendOptions& InOptions) {
    if (auto Other = m_OtherTransport.lock()) {
        if (Other->m_OnMessage) {
            Other->m_OnMessage(InMessage, nullptr);
        } else {
            std::lock_guard<std::mutex> Lock(Other->m_QueueMutex);
            Other->m_MessageQueue.push({InMessage, std::nullopt});
        }

        // Handle resumption token if provided
        if (InOptions.OnResumptionToken && InOptions.ResumptionToken) {
            InOptions.OnResumptionToken(*InOptions.ResumptionToken);
        }
    } else {
        if (m_OnError) { m_OnError("Not connected"); }
    }
}

void InMemoryTransport::SetOnMessage(MessageCallback InCallback) {
    m_OnMessage = std::move(InCallback);
}

void InMemoryTransport::SetOnError(ErrorCallback InCallback) {
    m_OnError = std::move(InCallback);
}

void InMemoryTransport::SetOnClose(CloseCallback InCallback) {
    m_OnClose = std::move(InCallback);
}

void InMemoryTransport::SetOnStart(StartCallback InCallback) {
    m_OnStart = std::move(InCallback);
}

void InMemoryTransport::SetOnStop(StopCallback InCallback) {
    m_OnStop = std::move(InCallback);
}

void InMemoryTransport::WriteSSEEvent(const std::string& InEvent, const std::string& InData) {
    std::string SSEMessage = "event: " + InEvent + "\ndata: " + InData + "\n\n";
    Send(SSEMessage);
}

// Note: Resumability is not yet supported by any transport implementation.
[[deprecated("Not yet implemented - will be supported in a future version")]]
bool InMemoryTransport::Resume(const std::string& InResumptionToken) {
    (void)InResumptionToken;
    // In-memory transport does not support resumption
    if (m_OnError) { m_OnError("Resumption not supported by InMemoryTransport"); }
    return false;
}

std::pair<std::shared_ptr<InMemoryTransport>, std::shared_ptr<InMemoryTransport>>
InMemoryTransport::CreateLinkedPair() {
    auto ClientTransport = std::make_shared<InMemoryTransport>();
    auto ServerTransport = std::make_shared<InMemoryTransport>();
    ClientTransport->m_OtherTransport = ServerTransport;
    ServerTransport->m_OtherTransport = ClientTransport;
    return {ClientTransport, ServerTransport};
}

MCP_NAMESPACE_END