#pragma once

#include "Core.h"
#include "MCP_Message.h"

MCP_NAMESPACE_BEGIN

// Abstract base class for all MCP transport mechanisms (stdio, HTTP, custom, etc.)
// Complies with MCP Spec, supports JSON-RPC 2.0, session management, and extensible callbacks.
class MCP_Transport {
  public:
    virtual ~MCP_Transport() = default;

    // Send a message (request, response, or notification) over the transport.
    // Returns true if the message was successfully queued for sending.
    virtual bool SendMessage(const MCP_MessageBase& message) = 0;

    // Receive the next message from the transport (blocking or non-blocking depending on
    // implementation). Returns nullptr if no message is available or on error.
    virtual std::unique_ptr<MCP_MessageBase> ReceiveMessage() = 0;

    // Start the transport (e.g., open connection, begin listening, etc.).
    virtual void Start() = 0;

    // Stop the transport (e.g., close connection, cleanup resources).
    virtual void Stop() = 0;

    // Returns true if the transport is currently open/active.
    virtual bool IsOpen() const = 0;

    // Session management (optional, for transports that support sessions)
    // Set or get the current session ID (for HTTP, SSE, etc.)
    virtual void SetSessionID(const SessionID& sessionId) {
        (void)sessionId; /* Optional */
    }
    virtual SessionID GetSessionID() const {
        return {};
    }

    // Register a callback to be invoked when a message is received (for async/event-driven
    // transports) The callback receives a reference to the received MCP_MessageBase.
    using MessageCallback = std::function<void(const MCP_MessageBase&)>;
    virtual void SetMessageCallback(MessageCallback callback) {
        (void)callback; /* Optional */
    }

    // Optional: Register a callback for transport errors
    using ErrorCallback = std::function<void(const std::string& errorMsg)>;
    virtual void SetErrorCallback(ErrorCallback callback) {
        (void)callback; /* Optional */
    }

    // Optional: Register a callback for transport state changes (open/close, etc.)
    using StateCallback = std::function<void(bool isOpen)>;
    virtual void SetStateCallback(StateCallback callback) {
        (void)callback; /* Optional */
    }

    // Optional: Return a string identifying the transport type (e.g., "stdio", "http", etc.)
    virtual std::string GetTransportType() const = 0;
};

MCP_NAMESPACE_END
