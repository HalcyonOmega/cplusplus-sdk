#pragma once

#include "../Message.h"
#include "Core.h"

MCP_NAMESPACE_BEGIN

// Forward declarations
class EventStore;

// AuthInfo definition
struct AuthInfo {
    std::string Token; // Example member, adjust if different fields are needed
    // Add other relevant authentication information here if necessary
};

// Transport options
struct TransportOptions {
    std::optional<std::string> ResumptionToken;
    std::optional<std::string> LastEventID;
    std::shared_ptr<EventStore> EventStore;
};

// Transport send options
struct TransportSendOptions {
    std::optional<std::string> RelatedRequestID;
    std::optional<std::string> ResumptionToken;
    std::function<void(const std::string&)> OnResumptionToken;
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
    virtual void Send(const std::string& InMessage, const TransportSendOptions& InOptions = {}) = 0;

    // Callback setters
    virtual void SetOnMessage(MessageCallback InCallback) = 0;
    virtual void SetOnError(ErrorCallback InCallback) = 0;
    virtual void SetOnClose(CloseCallback InCallback) = 0;
    virtual void SetOnStart(StartCallback InCallback) = 0;
    virtual void SetOnStop(StopCallback InCallback) = 0;

    // SSE event writing
    virtual void WriteSSEEvent(const std::string& InEvent, const std::string& InData) = 0;

    // Note: Resumability is not yet supported by any transport implementation.
    [[deprecated("Not yet implemented - will be supported in a future version")]]
    virtual bool Resume(const std::string& InResumptionToken) = 0;
};

MCP_NAMESPACE_END
