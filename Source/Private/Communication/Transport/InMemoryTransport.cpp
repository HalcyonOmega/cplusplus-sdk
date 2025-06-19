#include "Communication/Transport/InMemoryTransport.h"

#include <random>
#include <sstream>

#include "Constants.h"
#include "Core.h"
#include "ErrorBase.h"
#include "UUID/UUIDLayer.h"

MCP_NAMESPACE_BEGIN

// InMemoryEventStore implementation
void InMemoryEventStore::StoreEvent(const string& InEvent) {
    lock_guard<mutex> Lock(m_Mutex);
    string EventID = GenerateEventID();
    m_Events[EventID] = InEvent;
}

vector<string> InMemoryEventStore::ReplayEventsAfter(const string& LastEventID) {
    lock_guard<mutex> Lock(m_Mutex);
    vector<string> Events;
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

string InMemoryEventStore::GenerateEventID() {
    return to_string(chrono::system_clock::now().time_since_epoch().count());
}

InMemoryTransport::InMemoryTransport() : m_OtherTransport(nullptr), m_SessionID(GenerateUUID()) {}

InMemoryTransport::~InMemoryTransport() {
    if (m_OtherTransport) {
        m_OtherTransport->Close();
        m_OtherTransport->m_OtherTransport = nullptr;
    }
    Close();
}

future<void> InMemoryTransport::Start() {
    // Process any messages that were queued before start was called
    lock_guard<mutex> Lock(m_QueueMutex);
    while (!m_MessageQueue.empty()) {
        const auto& QueuedMessage = m_MessageQueue.front();
        CallOnMessage(*QueuedMessage.Message, QueuedMessage.AuthInfo);
        m_MessageQueue.pop();
    }
    CallOnStart();
}

future<void> InMemoryTransport::Close() {
    if (auto Other = m_OtherTransport) { Other->m_OtherTransport.reset(); }
    m_OtherTransport.reset();
    CallOnClose();
}

future<void> InMemoryTransport::Send(const MessageBase& InMessage,
                                     const TransportSendOptions& InOptions) {
    if (auto Other = m_OtherTransport) {
        // Build optional auth info to forward
        optional<AuthInfo> ForwardAuth = InOptions.AuthInfo;

        if (Other->OnMessage) {
            // Deliver immediately if the peer has an OnMessage handler installed
            Other->OnMessage.value()(InMessage, ForwardAuth);
        } else {
            // Otherwise enqueue the message until the peer starts the transport
            lock_guard<mutex> Lock(Other->m_QueueMutex);
            Other->m_MessageQueue.push({make_shared<MessageBase>(InMessage), ForwardAuth});
        }

        // Handle resumption token if provided
        if (InOptions.OnResumptionToken && InOptions.ResumptionToken) {
            InOptions.OnResumptionToken.value()(*InOptions.ResumptionToken);
        }
    } else {
        CallOnError("Not connected");
    }
}

void InMemoryTransport::WriteSSEEvent(const string& InEvent, const string& InData) {
    string SSEMessage = "event: " + InEvent + "\ndata: " + InData + "\n\n";
    Send(SSEMessage);
}

// Note: Resumability is not yet supported by any transport implementation.
[[deprecated("Not yet implemented - will be supported in a future version")]]
bool InMemoryTransport::Resume(const string& InResumptionToken) {
    (void)InResumptionToken;
    // In-memory transport does not support resumption
    CallOnError("Resumption not supported by InMemoryTransport");
    return false;
}

pair<shared_ptr<InMemoryTransport>, shared_ptr<InMemoryTransport>>
InMemoryTransport::CreateLinkedPair() {
    auto ClientTransport = make_shared<InMemoryTransport>();
    auto ServerTransport = make_shared<InMemoryTransport>();
    ClientTransport->m_OtherTransport = ServerTransport;
    ServerTransport->m_OtherTransport = ClientTransport;
    return {ClientTransport, ServerTransport};
}

MCP_NAMESPACE_END