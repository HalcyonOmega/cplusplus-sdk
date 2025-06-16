#pragma once

#include "Auth/Types/Auth.h"
#include "Communication/Transport/EventStore.h"
#include "Core.h"
#include "Core/Types/Progress.h"
#include "ErrorBase.h"
#include "MessageBase.h"
#include "RequestBase.h"

MCP_NAMESPACE_BEGIN

using StartCallback = function<void()>;
using CloseCallback = function<void()>;
using ErrorCallback = function<void(const ErrorBase&)>;
using MessageCallback = function<void(const MessageBase&, const optional<AuthInfo>&)>;
using ProgressCallback = function<void(const ProgressNotification&)>;

// Transport options
struct TransportOptions {
    optional<string> ResumptionToken;
    optional<string> LastEventID;
    shared_ptr<EventStore> EventStore;
};

// Options for sending a JSON-RPC message.
struct TransportSendOptions {
    // If present, `RelatedRequestID` is used to indicate to the transport which incoming request to
    // associate this outgoing message with.
    optional<RequestID> RelatedRequestID;

    // The resumption token used to continue long-running requests that were interrupted.
    // This allows clients to reconnect and continue from where they left off, if supported by the
    // transport.
    optional<string> ResumptionToken;

    // A callback that is invoked when the resumption token changes, if supported by the transport.
    // This allows clients to persist the latest token for potential reconnection.
    optional<function<void(const string& /* Token */)>> OnResumptionToken;

    // Optional authentication information to forward to the peer transport. This allows
    // in-process tests to exercise authenticated message flows without needing a full
    // authentication pipeline.
    optional<AuthInfo> AuthInfo;
};

// Describes the minimal contract for a MCP transport that a client or server can communicate over.
class Transport {
  public:
    // Virtual destructor for proper cleanup.
    virtual ~Transport() = default;

    // Starts processing messages on the transport, including any connection steps that might need
    // to be taken. This method should only be called after callbacks are installed, or else
    // messages may be lost. NOTE: This method should not be called explicitly when using Client,
    // Server, or Protocol classes, as they will implicitly call Start().
    virtual future<void> Start() = 0;

    // Closes the connection.
    virtual future<void> Close() = 0;

    // Sends a JSON-RPC message (request or response). If present, `relatedRequestID` is used to
    // indicate to the transport which incoming request to associate this outgoing message with.
    // TODO: @HalcyonOmega Should the TransportSendOptions be optional?
    virtual future<void> Send(const MessageBase& InMessage,
                              const TransportSendOptions& InOptions = {}) = 0;

    // SSE event writing
    // TODO: @HalcyonOmega Should this be in the base class?
    virtual void WriteSSEEvent(const string& InEvent, const string& InData) = 0;

    // Note: Resumability is not yet supported by any transport implementation.
    [[deprecated("Not yet implemented - will be supported in a future version")]]
    virtual bool Resume(const string& InResumptionToken) = 0;

    // Getters
    optional<string> GetSessionID() const {
        return m_SessionID;
    }

    optional<StartCallback> OnStart;

    // Callback for when the connection is closed for any reason. This should be invoked when
    // Close() is called as well.
    optional<CloseCallback> OnClose;

    // Callback for when an error occurs. Note that errors are not necessarily fatal; they are used
    // for reporting any kind of exceptional condition out of band.
    optional<ErrorCallback> OnError;

    // Callback for when a message (request or response) is received over the connection. Includes
    // the AuthInfo if the transport is authenticated.
    optional<MessageCallback> OnMessage;

  protected:
    // Helper methods to safely invoke callbacks with proper locking and null checks
    void CallOnStart() const {
        lock_guard<mutex> lock(m_CallbackMutex);
        if (OnStart) { (*OnStart)(); }
    }

    void CallOnClose() const {
        lock_guard<mutex> lock(m_CallbackMutex);
        if (OnClose) { (*OnClose)(); }
    }

    void CallOnError(const ErrorBase& InError) const {
        lock_guard<mutex> lock(m_CallbackMutex);
        if (OnError) { (*OnError)(InError); }
    }

    void CallOnError(const string& InMessage) const {
        ErrorBase Error(Errors::InternalError, InMessage);
        CallOnError(Error);
    }

    void CallOnMessage(const MessageBase& InMessage,
                       const optional<AuthInfo>& InAuthInfo = nullopt) const {
        lock_guard<mutex> lock(m_CallbackMutex);
        if (OnMessage) { (*OnMessage)(InMessage, InAuthInfo); }
    }

  private:
    // The session ID generated for this connection.
    optional<string> m_SessionID;

    mutable mutex m_CallbackMutex; // Protects callback invocation to avoid concurrent access
                                   // between read thread and callers.
};

MCP_NAMESPACE_END
