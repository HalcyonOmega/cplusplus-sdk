#pragma once

#include "../ITransport.h"
#include "Core.h"
#include "HTTPProxy.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Sandbox/IMCP.h"
#include "StreamableHTTPBase.h"

// Poco Net includes
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Thread.h>

MCP_NAMESPACE_BEGIN

using IncomingMessage = Poco::Net::HTTPServerRequest;
using ServerResponse = Poco::Net::HTTPServerResponse;
using SessionInitializedCallback = function<MCPTask_Void(const string& InSessionID)>;

class StreamableHTTPServer : public ITransport {
  public:
    // === ITransport Implementation ===
    MCPTask_Void Connect() override;
    MCPTask_Void Disconnect() override;
    MCPTask_Void SendMessage(const MessageBase& InMessage) override;
    // === End ITransport Implementation ===

    MCPTask_Void HandleRequest(IncomingMessage& InRequest, ServerResponse& InResponse,
                               optional<AuthInfo> InAuthInfo = nullopt,
                               optional<any> InParsedBody = nullopt);

    /**
     * Configuration options for StreamableHTTPServerTransport
     */
    struct Options {
        /**
         * Function that generates a session ID for the transport.
         * The session ID SHOULD be globally unique and cryptographically secure (e.g., a securely
         * generated UUID, a JWT, or a cryptographic hash)
         *
         * Return undefined to disable session management.
         */
        function<optional<string>()> SessionIDGenerator;

        /**
         * A callback for session initialization events
         * This is called when the server initializes a new session.
         * Useful in cases when you need to register multiple mcp sessions
         * and need to keep track of them.
         * @param sessionId The generated session ID
         */
        optional<SessionInitializedCallback> OnSessionInitialized;

        /**
         * If true, the server will return JSON responses instead of starting an SSE stream.
         * This can be useful for simple request/response scenarios without streaming.
         * Default is false (SSE streams are preferred).
         */
        optional<bool> EnableJSONResponse;

        /**
         * Event store for resumability support
         * If provided, resumability will be enabled, allowing clients to reconnect and resume
         * messages
         */
        optional<EventStore> EventStore;
    };

    StreamableHTTPServer(const Options& InOptions)
        : m_EnableJSONResponse(InOptions.EnableJSONResponse.value_or(false)),
          m_SessionIDGenerator(InOptions.SessionIDGenerator), m_EventStore(InOptions.EventStore),
          m_OnSessionInitialized(InOptions.OnSessionInitialized) {};

  private:
    MCPTask_Void HandleGetRequest(IncomingMessage& InRequest, ServerResponse& InResponse);
    MCPTask_Void ReplayEvents(string InLastEventID, ServerResponse& InResponse);
    bool WriteSSEEvent(ServerResponse& InResponse, const MessageBase& InMessage,
                       const optional<string>& InEventID = nullopt);
    MCPTask_Void HandleUnsupportedRequest(ServerResponse& InResponse);
    MCPTask_Void HandlePostRequest(IncomingMessage& InRequest, ServerResponse& InResponse,
                                   optional<AuthInfo> InAuthInfo = nullopt,
                                   optional<any> InParsedBody = nullopt);
    MCPTask_Void HandleDeleteRequest(IncomingMessage& InRequest, ServerResponse& InResponse);
    bool ValidateSession(IncomingMessage& InRequest, ServerResponse& InResponse);

    // When SessionID is not set (undefined), it means the transport is in stateless mode
    bool m_Started = false;
    bool m_Initialized = false;
    bool m_EnableJSONResponse = false;
    std::map<string, ServerResponse> m_StreamMapping;
    std::map<RequestID, string> m_RequestToStreamMapping;
    std::map<RequestID, MessageBase> m_RequestResponseMap;
    string m_StandaloneSSEStreamID = "_GET_stream";
    function<optional<string>()> m_SessionIDGenerator;
    optional<IEventStore> m_EventStore;
    optional<SessionInitializedCallback> m_OnSessionInitialized;

    // === ITransport Members ===
    optional<string> m_SessionID;

    // Callbacks
    optional<ConnectCallback> m_ConnectCallback;
    optional<DisconnectCallback> m_DisconnectCallback;
    optional<ErrorCallback> m_ErrorCallback;
    optional<MessageCallback> m_MessageCallback;
};

MCP_NAMESPACE_END