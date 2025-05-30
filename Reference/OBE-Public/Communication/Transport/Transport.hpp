#pragma once

#include <functional>
#include <future>
#include <optional>
#include <string>

#include "../Auth/Auth.hpp"
#include "../Core/Types/General.hpp"
#include "../Core/Types/Request.hpp"

MCP_NAMESPACE_BEGIN

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
};

// Describes the minimal contract for a MCP transport that a client or server can communicate over.
class Transport {
  public:
    // Starts processing messages on the transport, including any connection steps that might need
    // to be taken. This method should only be called after callbacks are installed, or else
    // messages may be lost. NOTE: This method should not be called explicitly when using Client,
    // Server, or Protocol classes, as they will implicitly call start().
    virtual future<void> Start() = 0;

    // Sends a JSON-RPC message (request or response). If present, `relatedRequestID` is used to
    // indicate to the transport which incoming request to associate this outgoing message with.
    virtual future<void> Send(const JSON_RPC_Message& Message,
                              const optional<TransportSendOptions>& Options = nullopt) = 0;

    // Closes the connection.
    virtual future<void> Close() = 0;

    // Virtual destructor for proper cleanup.
    virtual ~Transport() = default;

    // Callback for when the connection is closed for any reason. This should be invoked when
    // close() is called as well.
    optional<function<void()>> OnClose;

    // Callback for when an error occurs. Note that errors are not necessarily fatal; they are used
    // for reporting any kind of exceptional condition out of band.
    optional<function<void(const exception&)>> OnError;

    // Callback for when a message (request or response) is received over the connection. Includes
    // the authInfo if the transport is authenticated.
    optional<function<void(const JSON_RPC_Message&, const optional<AuthInfo>&)>> OnMessage;

    // The session ID generated for this connection.
    optional<string> SessionID;
};

MCP_NAMESPACE_END
