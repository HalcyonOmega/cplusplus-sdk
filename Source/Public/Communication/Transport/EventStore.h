#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

/**
 * Event store interface for resumability support
 */
class EventStore {
  public:
    virtual ~EventStore() = default;
    virtual void StoreEvent(const std::string& event) = 0;
    virtual std::vector<std::string> ReplayEventsAfter(const std::string& lastEventId) = 0;
};

MCP_NAMESPACE_END