#pragma once

#include "../Message.h"
#include "Core.h"

MCP_NAMESPACE_BEGIN

// Forward declarations
class EventStore;

// AuthInfo definition
struct AuthInfo {
    std::string token; // Example member, adjust if different fields are needed
    // Add other relevant authentication information here if necessary
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

    // Note: Resumability is not yet supported by any transport implementation.
    [[deprecated("Not yet implemented - will be supported in a future version")]]
    virtual bool Resume(const std::string& resumptionToken) = 0;
};

MCP_NAMESPACE_END
