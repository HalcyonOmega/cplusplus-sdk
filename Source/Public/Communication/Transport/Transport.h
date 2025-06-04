#pragma once

#include <functional>
#include <future>
#include <map>
#include <memory>
#include <optional>
#include <string>

#include "Core.h"
#include "MCP_Message.h"

MCP_NAMESPACE_BEGIN

// Forward declarations
struct AuthInfo;

// Event store interface for resumability support
class MCP_EventStore {
  public:
    virtual ~MCP_EventStore() = default;
    virtual std::string StoreEvent(const std::string& streamId, const MCP_MessageBase& message) = 0;
    virtual std::string
    ReplayEventsAfter(const std::string& lastEventId,
                      std::function<void(const std::string&, const MCP_MessageBase&)> send) = 0;
};

// Transport configuration options
struct MCP_TransportOptions {
    std::function<std::string()> sessionIdGenerator;
    std::function<void(const std::string&)> onSessionInitialized;
    bool enableJsonResponse = false;
    std::shared_ptr<MCP_EventStore> eventStore;
};

// Options for sending a JSON-RPC message
struct MCP_TransportSendOptions {
    // If present, used to indicate which incoming request to associate this outgoing message with
    std::optional<RequestID> relatedRequestId;

    // The resumption token used to continue long-running requests that were interrupted
    std::optional<std::string> resumptionToken;

    // Callback invoked when the resumption token changes
    std::function<void(const std::string&)> onResumptionToken;
};

// Abstract base class for all MCP transport mechanisms (stdio, HTTP, custom, etc.)
// Complies with MCP Spec, supports JSON-RPC 2.0, session management, and extensible callbacks.
class MCP_Transport {
  public:
    static constexpr size_t MAXIMUM_MESSAGE_SIZE = 4 * 1024 * 1024; // 4MB

    virtual ~MCP_Transport() = default;

    // Starts processing messages on the transport, including any connection steps
    // This method should only be called after callbacks are installed, or else messages may be lost
    virtual std::future<void> Start() = 0;

    // Sends a JSON-RPC message (request or response)
    // If present, relatedRequestId is used to indicate which incoming request to associate with
    virtual std::future<void> Send(const MCP_MessageBase& message,
                                   const MCP_TransportSendOptions& options = {}) = 0;

    // Closes the connection
    virtual std::future<void> Close() = 0;

    // Callbacks
    using CloseCallback = std::function<void()>;
    virtual void SetCloseCallback(CloseCallback callback) {
        (void)callback; /* Optional */
    }

    using ErrorCallback = std::function<void(const std::string& errorMsg)>;
    virtual void SetErrorCallback(ErrorCallback callback) {
        (void)callback; /* Optional */
    }

    using MessageCallback = std::function<void(const MCP_MessageBase&, const AuthInfo*)>;
    virtual void SetMessageCallback(MessageCallback callback) {
        (void)callback; /* Optional */
    }

    // Session management
    virtual std::optional<std::string> GetSessionId() const {
        return std::nullopt;
    }

    // Validate session ID for non-initialization requests
    virtual bool ValidateSession(const std::string& sessionId) {
        (void)sessionId;
        return true;
    }

    // Write an event to the SSE stream with proper formatting
    virtual bool WriteSSEEvent(const MCP_MessageBase& message, const std::string& eventId = "") {
        (void)message;
        (void)eventId;
        return false;
    }

    // Optional: Return a string identifying the transport type (e.g., "stdio", "http", etc.)
    virtual std::string GetTransportType() const = 0;

  protected:
    // Stream management
    using StreamId = std::string;
    using EventId = std::string;
    std::map<StreamId, std::shared_ptr<MCP_MessageBase>> streamMapping;
    std::map<RequestID, StreamId> requestToStreamMapping;
    std::map<RequestID, std::shared_ptr<MCP_MessageBase>> requestResponseMap;
};

MCP_NAMESPACE_END
