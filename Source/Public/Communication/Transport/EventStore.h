#pragma once

#include "Core.h"
#include "MessageBase.h"

MCP_NAMESPACE_BEGIN

// Event store interface for resumability support
class EventStore {
  public:
    virtual ~EventStore() = default;

    /**
     * Stores an event for later retrieval
     * @param InStreamID ID of the stream the event belongs to
     * @param InMessage The JSON-RPC message to store
     * @returns The generated event ID for the stored event
     */
    virtual future<EventID> StoreEvent(const StreamID& InStreamID,
                                       const MessageBase& InMessage) = 0;
    virtual future<StreamID>
    ReplayEventsAfter(const EventID& InLastEventID,
                      function<future<void>(const EventID&, const MessageBase&)> InSend) = 0;
};

MCP_NAMESPACE_END