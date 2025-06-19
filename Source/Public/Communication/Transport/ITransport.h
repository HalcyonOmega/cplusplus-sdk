#pragma once

#include <functional>
#include <memory>
#include <optional>

#include "Auth/Types/Auth.h"
#include "Communication/Utilities/AbortController.h"
#include "Macros.h"
#include "RequestID.h"
#include "Utilities/Async/MCPTask.h"

class IEventStore;
struct ProgressNotification;
class MessageBase;
class ErrorBase;

using std::function;
using std::mutex;
using std::nullopt;
using std::optional;
using std::shared_ptr;
using std::string;

MCP_NAMESPACE_BEGIN

// TODO: @HalcyonOmega Begin Direct Translated Code
using ConnectCallback = function<void()>;
using DisconnectCallback = function<void()>;
using ErrorCallback = function<void(const ErrorBase&)>;
using MessageCallback = function<void(const MessageBase&, const optional<AuthInfo>&)>;
using ProgressCallback = function<void(const ProgressNotification&)>;

// Transport options
struct TransportOptions {
    optional<string> ResumptionToken;
    optional<string> LastEventID;
    shared_ptr<IEventStore> EventStore;
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

// Options that can be given per request.
struct RequestOptions : public TransportSendOptions {
    // If set, requests progress notifications from the remote end (if supported). When progress
    // notifications are received, this callback will be invoked.
    optional<ProgressCallback> OnProgress;

    // Can be used to cancel an in-flight request. This will cause an AbortError to be raised from
    // request().
    optional<AbortSignal> Signal;

    // A timeout (in milliseconds) for this request. If exceeded, an MCP_Error with code
    // `RequestTimeout` will be raised from request().
    //
    // If not specified, `DEFAULT_REQUEST_TIMEOUT_MSEC` will be used as the timeout.
    optional<int64_t> Timeout;

    // If true, receiving a progress notification will reset the request timeout.
    // This is useful for long-running operations that send periodic progress updates.
    // Default: false
    optional<bool> ResetTimeoutOnProgress;

    // Maximum total time (in milliseconds) to wait for a response.
    // If exceeded, an MCP_Error with code `RequestTimeout` will be raised, regardless of progress
    // notifications. If not specified, there is no maximum total timeout.
    optional<int64_t> MaxTotalTimeout;
};

// TODO: @HalcyonOmega End Direct Translated Code

// Transport types for easy selection
enum class TransportType {
    Stdio,   // Standard input/output
    HTTP,    // HTTP with Server-Sent Events
    InMemory // In-memory transport (for testing)
};

/**
 * @brief Abstract base class for MCP Transport.
 *
 * This class defines the interface for sending and receiving JSON-RPC messages
 * as per the Model Context Protocol (MCP) specification.
 */
class ITransport {
  public:
    /**
     * Starts processing messages on the transport, including any connection steps that might need
     * to be taken.
     *
     * This method should only be called after callbacks are installed, or else messages may be
     * lost.
     *
     * NOTE: This method should not be called explicitly when using Client, Server, or Protocol
     * classes, as they will implicitly call start().
     */
    virtual MCPTask_Void Connect() = 0;

    /**
     * Closes the connection.
     */
    virtual MCPTask_Void Disconnect() = 0;

    /**
     * Sends a JSON-RPC message (request or response).
     *
     * If present, `relatedRequestId` is used to indicate to the transport which incoming request to
     * associate this outgoing message with.
     */
    virtual MCPTask_Void SendMessage(const MessageBase& InMessage) = 0;

    // State management
    virtual bool IsConnected() const = 0;

    // Note: Resumability is not yet supported by any transport implementation.
    [[deprecated("Not yet implemented - will be supported in a future version")]]
    virtual bool Resume(const string& InResumptionToken) = 0;

    /**
     * The session ID generated for this connection.
     */
    optional<string> SessionID;

    optional<ConnectCallback> OnConnect;

    // Callback for when the connection is closed for any reason. This should be invoked when
    // Close() is called as well.
    optional<DisconnectCallback> OnDisconnect;

    // Callback for when an error occurs. Note that errors are not necessarily fatal; they are used
    // for reporting any kind of exceptional condition out of band.
    optional<ErrorCallback> OnError;

    // Callback for when a message (request or response) is received over the connection. Includes
    // the AuthInfo if the transport is authenticated.
    optional<MessageCallback> OnMessage;

  protected:
    // Helper methods to safely invoke callbacks with proper locking and null checks
    void CallOnConnect() const;
    void CallOnDisconnect() const;
    void CallOnError(const ErrorBase& InError) const;
    void CallOnError(const string& InMessage) const;
    void CallOnMessage(const MessageBase& InMessage,
                       const optional<AuthInfo>& InAuthInfo = nullopt) const;

  private:
    mutable mutex m_CallbackMutex; // Protects callback invocation to avoid concurrent access

  public:
    // Disallow copy and move operations to prevent slicing
    ITransport(const ITransport&) = delete;
    ITransport& operator=(const ITransport&) = delete;
    ITransport(ITransport&&) = delete;
    ITransport& operator=(ITransport&&) = delete;
    virtual ~ITransport() = default;

  protected:
    ITransport() = default;
};

MCP_NAMESPACE_END