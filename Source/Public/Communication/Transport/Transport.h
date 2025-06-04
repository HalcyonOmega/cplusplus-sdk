#pragma once

#include <functional>
#include <future>
#include <map>
#include <memory>
#include <optional>
#include <string>

#include "../../Core/Includes/Core.h"
#include "../Message.h"

MCP_NAMESPACE_BEGIN

// Forward declarations
class AuthInfo;

// Event store interface for resumability support
class EventStore {
  public:
    virtual ~EventStore() = default;
    virtual void StoreEvent(const std::string& event) = 0;
    virtual std::vector<std::string> ReplayEventsAfter(const std::string& lastEventId) = 0;
};

// Transport options
struct TransportOptions {
    std::optional<std::string> resumptionToken;
    std::optional<std::string> lastEventId;
    std::shared_ptr<EventStore> eventStore;
};

// Transport send options
struct TransportSendOptions {
    std::optional<std::string> relatedRequestId;
    std::optional<std::string> resumptionToken;
    std::function<void(const std::string&)> onResumptionToken;
};

// Transport callbacks
using MessageCallback = std::function<void(const std::string&, const AuthInfo*)>;
using ErrorCallback = std::function<void(const std::string&)>;
using CloseCallback = std::function<void()>;
using StartCallback = std::function<void()>;
using StopCallback = std::function<void()>;

// Base transport interface
class Transport {
  public:
    virtual ~Transport() = default;

    // Core transport methods
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual void Send(const std::string& message, const TransportSendOptions& options = {}) = 0;

    // Callback setters
    virtual void SetOnMessage(MessageCallback callback) = 0;
    virtual void SetOnError(ErrorCallback callback) = 0;
    virtual void SetOnClose(CloseCallback callback) = 0;
    virtual void SetOnStart(StartCallback callback) = 0;
    virtual void SetOnStop(StopCallback callback) = 0;

    // SSE event writing
    virtual void WriteSSEEvent(const std::string& event, const std::string& data) = 0;
};

MCP_NAMESPACE_END
