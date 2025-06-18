#pragma once

#include "Macros.h"
#include "MessageBase.h"
#include "Utilities/Async/MCPTask.h"

MCP_NAMESPACE_BEGIN

/**
 * Interface for resumability support via event storage
 */
class IEventStore {
    /**
     * Stores an event for later retrieval
     * @param InStreamID ID of the stream the event belongs to
     * @param InMessage The JSON-RPC message to store
     * @returns The generated event ID for the stored event
     */
    virtual MCPTask<string> StoreEvent(const string& InStreamID, const MessageBase& InMessage) = 0;

    virtual MCPTask<string> ReplayEventsAfter(
        string LastEventID,
        function<MCPTask_Void(const string& InEventID, const MessageBase& InMessage)> InSend) = 0;
};

MCP_NAMESPACE_END