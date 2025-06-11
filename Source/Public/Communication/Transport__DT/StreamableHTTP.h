#pragma once

// TODO: Fix External Ref: HTTP server implementation (IncomingMessage, ServerResponse equivalents)
// TODO: Fix External Ref: Transport base class
// TODO: Fix External Ref: JSON-RPC message types and validation
// TODO: Fix External Ref: AuthInfo type

#include "Communication/Messages.h"
#include "Core.h"

MCP_NAMESPACE_BEGIN

using IncomingMessage = HTTP_Request;
using ServerResponse = HTTP_Response;

const string MAXIMUM_MESSAGE_SIZE = "4mb";

/**
 * Interface for resumability support via event storage
 */
class EventStore {
  public:
    virtual ~EventStore() = default;

    /**
     * Stores an event for later retrieval
     * @param InStreamID ID of the stream the event belongs to
     * @param InMessage The JSON-RPC message to store
     * @returns The generated event ID for the stored event
     */
    virtual future<EventID> StoreEvent(const StreamID& InStreamID,
                                       const MessageBase& InMessage) = 0;

    virtual future<StreamID>
    ReplayEventsAfter(const EventID& InLastEventID,
                      function<future<void>(const EventID&, const MessageBase&)> InSend) = 0;
};

/**
 * Configuration options for StreamableHTTPServerTransport
 */
struct StreamableHTTPServerTransportOptions {
    /**
     * Function that generates a session ID for the transport.
     * The session ID SHOULD be globally unique and cryptographically secure (e.g., a securely
     * generated UUID, a JWT, or a cryptographic hash)
     *
     * Return empty optional to disable session management.
     */
    optional<function<string()>> SessionIDGenerator;

    /**
     * A callback for session initialization events
     * This is called when the server initializes a new session.
     * Useful in cases when you need to register multiple mcp sessions
     * and need to keep track of them.
     * @param SessionID The generated session ID
     */
    optional<function<void(const string&)>> OnSessionInitialized;

    /**
     * If true, the server will return JSON responses instead of starting an SSE stream.
     * This can be useful for simple request/response scenarios without streaming.
     * Default is false (SSE streams are preferred).
     */
    bool EnableJsonResponse = false;

    /**
     * Event store for resumability support
     * If provided, resumability will be enabled, allowing clients to reconnect and resume messages
     */
    shared_ptr<EventStore> EventStore;
};

/**
 * Server transport for Streamable HTTP: this implements the MCP Streamable HTTP transport
 * specification. It supports both SSE streaming and direct HTTP responses.
 *
 * Usage example:
 *
 * ```cpp
 * // Stateful mode - server sets the session ID
 * StreamableHTTPServerTransportOptions statefulOptions;
 * statefulOptions.SessionIDGenerator = []() { return generateUUID(); };
 * auto statefulTransport = make_unique<StreamableHTTPServerTransport>(statefulOptions);
 *
 * // Stateless mode - explicitly set session ID generator to nullopt
 * StreamableHTTPServerTransportOptions statelessOptions;
 * statelessOptions.SessionIDGenerator = nullopt;
 * auto statelessTransport = make_unique<StreamableHTTPServerTransport>(statelessOptions);
 * ```
 *
 * In stateful mode:
 * - Session ID is generated and included in response headers
 * - Session ID is always included in initialization responses
 * - Requests with invalid session IDs are rejected with HTTPStatus::NotFound Not Found
 * - Non-initialization requests without a session ID are rejected with HTTPStatus::BadRequest Bad
 * Request
 * - State is maintained in-memory (connections, message history)
 *
 * In stateless mode:
 * - No Session ID is included in any responses
 * - No session validation is performed
 */
class StreamableHTTPServerTransport : public Transport {
  private:
    // when SessionID is not set (nullopt), it means the transport is in stateless mode
    optional<function<string()>> m_SessionIDGenerator;
    bool m_Started = false;
    unordered_map<string, shared_ptr<ServerResponse>> m_StreamMapping;
    unordered_map<RequestID, string> m_RequestToStreamMapping;
    unordered_map<RequestID, MessageBase> m_RequestResponseMap;
    bool m_Initialized = false;
    bool m_EnableJSONResponse = false;
    string m_StandaloneSSEStreamID = "_GET_stream";
    shared_ptr<EventStore> m_EventStore;
    optional<function<void(const string&)>> m_OnSessionInitialized;

  public:
    optional<string> SessionID;

    explicit StreamableHTTPServerTransport(const StreamableHTTPServerTransportOptions& InOptions)
        : m_SessionIDGenerator(InOptions.SessionIDGenerator),
          m_EnableJSONResponse(InOptions.EnableJSONResponse), m_EventStore(InOptions.EventStore),
          m_OnSessionInitialized(InOptions.OnSessionInitialized) {}

    /**
     * Starts the transport. This is required by the Transport interface but is a no-op
     * for the Streamable HTTP transport as connections are managed per-request.
     */
    future<void> Start() override;

    /**
     * Handles an incoming HTTP request, whether GET or POST
     */
    future<void> HandleRequest(const IncomingMessage& InRequest,
                               shared_ptr<ServerResponse> InResponse,
                               const optional<JSON>& InParsedBody = nullopt);

  private:
    /**
     * Handles GET requests for SSE stream
     */
    future<void> HandleGetRequest(const IncomingMessage& InRequest,
                                  shared_ptr<ServerResponse> InResponse);

    /**
     * Replays events that would have been sent after the specified event ID
     * Only used when resumability is enabled
     */
    future<void> ReplayEvents(const string& InLastEventID, shared_ptr<ServerResponse> InResponse);

    /**
     * Writes an event to the SSE stream with proper formatting
     */
    bool WriteSSEEvent(shared_ptr<ServerResponse> InResponse, const MessageBase& InMessage,
                       const optional<string>& InEventID = nullopt);

    /**
     * Handles unsupported requests (PUT, PATCH, etc.)
     */
    future<void> HandleUnsupportedRequest(shared_ptr<ServerResponse> InResponse);

    /**
     * Handles POST requests containing JSON-RPC messages
     */
    future<void> HandlePostRequest(const IncomingMessage& InRequest,
                                   shared_ptr<ServerResponse> InResponse,
                                   const optional<JSON>& InParsedBody = nullopt);

    /**
     * Handles DELETE requests to terminate sessions
     */
    future<void> HandleDeleteRequest(const IncomingMessage& InRequest,
                                     shared_ptr<ServerResponse> InResponse);

    /**
     * Validates session ID for non-initialization requests
     * Returns true if the session is valid, false otherwise
     */
    bool ValidateSession(const IncomingMessage& InRequest, shared_ptr<ServerResponse> InResponse);

  public:
    future<void> Close() override;

    future<void> Send(const MessageBase& InMessage,
                      const optional<RequestID>& InRelatedRequestID = nullopt) override;

  private:
    /**
     * Generates a UUID-like string for request/stream IDs
     */
    string GenerateUUID();
};

// ##########################################################
// ##########################################################
// ===================== CLIENT SECTION =====================
// ##########################################################
// ##########################################################

// Default reconnection options for StreamableHTTP connections
struct StreamableHTTPReconnectionOptions {
    int MaxReconnectionDelay = 30000;    // 30 seconds
    int InitialReconnectionDelay = 1000; // 1 second
    double ReconnectionDelayGrowFactor = 1.5;
    int MaxRetries = 2;
};

const StreamableHTTPReconnectionOptions DEFAULT_STREAMABLE_HTTP_RECONNECTION_OPTIONS = {
    1000,  // initialReconnectionDelay
    30000, // maxReconnectionDelay
    1.5,   // reconnectionDelayGrowFactor
    2      // maxRetries
};

class StreamableHTTPError : public exception {
  private:
    optional<int> m_Code;
    string m_Message;
    string m_FullMessage;

  public:
    const char* what() const noexcept override;

    optional<int> GetCode() const;
    const string& GetMessage() const;
};

/**
 * Options for starting or authenticating an SSE connection
 */
struct StartSSEOptions {
    optional<string> ResumptionToken;
    function<void(const string&)> OnResumptionToken;
    optional<string> ReplayMessageID; // Can be string or number
};

/**
 * Configuration options for the `StreamableHTTPClientTransport`.
 */
struct StreamableHTTPClientTransportOptions {
    shared_ptr<OAuthClientProvider> AuthProvider;
    map<string, string> RequestHeaders; // RequestInit equivalent
    StreamableHTTPReconnectionOptions ReconnectionOptions =
        DEFAULT_STREAMABLE_HTTP_RECONNECTION_OPTIONS;
    optional<string> SessionID;
};

/**
 * Client transport for Streamable HTTP: this implements the MCP Streamable HTTP transport
 * specification. It will connect to a server using HTTP POST for sending messages and HTTP GET with
 * Server-Sent Events for receiving messages.
 */
class StreamableHTTPClientTransport {
  private:
    atomic<bool> m_AbortRequested;
    string m_URL;
    optional<string> m_ResourceMetadataURL;
    map<string, string> m_RequestHeaders;
    shared_ptr<OAuthClientProvider> m_AuthProvider;
    optional<string> m_SessionID;
    StreamableHTTPReconnectionOptions m_ReconnectionOptions;

  public:
    function<void()> OnClose;
    function<void(const exception&)> OnError;
    function<void(const MessageBase&)> OnMessage;

    StreamableHTTPClientTransport(const string& InURL,
                                  const StreamableHTTPClientTransportOptions& InOptions = {})
        : m_AbortRequested(false), m_URL(InURL), m_RequestHeaders(InOptions.RequestHeaders),
          m_AuthProvider(InOptions.AuthProvider), m_SessionID(InOptions.SessionID),
          m_ReconnectionOptions(InOptions.ReconnectionOptions) {}

  private:
    future<void> AuthThenStart();

    future<map<string, string>> CommonHeaders();

    future<void> StartOrAuthSSE(const StartSSEOptions& InOptions);

    int GetNextReconnectionDelay(int InAttempt);

    void ScheduleReconnection(const StartSSEOptions& InOptions, int InAttemptCount = 0);

    void HandleSSEStream(const string& InStreamData, const StartSSEOptions& InOptions);

  public:
    future<void> Start();

    /**
     * Call this method after the user has finished authorizing via their user agent and is
     * redirected back to the MCP client application.
     */
    future<void> FinishAuth(const string& InAuthorizationCode);

    future<void> Close();

    struct SendOptions {
        optional<string> ResumptionToken;
        function<void(const string&)> OnResumptionToken;
    };

    future<void> Send(const MessageBase& InMessage, const SendOptions& InOptions = {});

    future<void> Send(const vector<MessageBase>& InMessages, const SendOptions& InOptions = {});

    optional<string> GetSessionID() const;

    /**
     * Terminates the current session by sending a DELETE request to the server.
     */
    future<void> TerminateSession();
};

MCP_NAMESPACE_END