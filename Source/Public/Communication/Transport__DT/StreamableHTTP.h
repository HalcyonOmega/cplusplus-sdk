#pragma once

// TODO: Fix External Ref: HTTP server implementation (IncomingMessage, ServerResponse equivalents)
// TODO: Fix External Ref: Transport base class
// TODO: Fix External Ref: JSON-RPC message types and validation
// TODO: Fix External Ref: AuthInfo type

#include "Core.h"

MCP_NAMESPACE_BEGIN

// TODO: Fix External Ref: HTTP request/response types

struct IncomingMessage {
    string method;
    unordered_map<string, string> headers;
    string body;
    optional<AuthInfo> auth;
};

struct ServerResponse {
    function<void(int, const unordered_map<string, string>&)> writeHead;
    function<void(const string&)> end;
    function<bool(const string&)> write;
    function<void()> flushHeaders;
    function<void(function<void()>)> on;
    bool closed = false;
};

const string MAXIMUM_MESSAGE_SIZE = "4mb";

/**
 * Interface for resumability support via event storage
 */
class EventStore {
  public:
    virtual ~EventStore() = default;

    /**
     * Stores an event for later retrieval
     * @param StreamID ID of the stream the event belongs to
     * @param message The JSON-RPC message to store
     * @returns The generated event ID for the stored event
     */
    virtual future<EventID> storeEvent(const StreamID& StreamID, const JSONRPCMessage& message) = 0;

    virtual future<StreamID>
    replayEventsAfter(const EventID& lastEventID,
                      function<future<void>(const EventID&, const JSONRPCMessage&)> send) = 0;
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
    optional<function<void(const string&)>> onsessioninitialized;

    /**
     * If true, the server will return JSON responses instead of starting an SSE stream.
     * This can be useful for simple request/response scenarios without streaming.
     * Default is false (SSE streams are preferred).
     */
    bool enableJsonResponse = false;

    /**
     * Event store for resumability support
     * If provided, resumability will be enabled, allowing clients to reconnect and resume messages
     */
    shared_ptr<EventStore> eventStore;
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
 * - Requests with invalid session IDs are rejected with 404 Not Found
 * - Non-initialization requests without a session ID are rejected with 400 Bad Request
 * - State is maintained in-memory (connections, message history)
 *
 * In stateless mode:
 * - No Session ID is included in any responses
 * - No session validation is performed
 */
class StreamableHTTPServerTransport : public Transport {
  private:
    // when SessionID is not set (nullopt), it means the transport is in stateless mode
    optional<function<string()>> SessionIDGenerator;
    bool _started = false;
    unordered_map<string, shared_ptr<ServerResponse>> _streamMapping;
    unordered_map<RequestID, string> _requestToStreamMapping;
    unordered_map<RequestID, JSONRPCMessage> _requestResponseMap;
    bool _initialized = false;
    bool _enableJsonResponse = false;
    string _standaloneSseStreamID = "_GET_stream";
    shared_ptr<EventStore> _eventStore;
    optional<function<void(const string&)>> _onsessioninitialized;

  public:
    optional<string> SessionID;

    explicit StreamableHTTPServerTransport(const StreamableHTTPServerTransportOptions& options)
        : SessionIDGenerator(options.SessionIDGenerator),
          _enableJsonResponse(options.enableJsonResponse), _eventStore(options.eventStore),
          _onsessioninitialized(options.onsessioninitialized) {}

    /**
     * Starts the transport. This is required by the Transport interface but is a no-op
     * for the Streamable HTTP transport as connections are managed per-request.
     */
    future<void> start() override;

    /**
     * Handles an incoming HTTP request, whether GET or POST
     */
    future<void> handleRequest(const IncomingMessage& req, shared_ptr<ServerResponse> res,
                               const optional<JSON>& parsedBody = nullopt);

  private:
    /**
     * Handles GET requests for SSE stream
     */
    future<void> handleGetRequest(const IncomingMessage& req, shared_ptr<ServerResponse> res);

    /**
     * Replays events that would have been sent after the specified event ID
     * Only used when resumability is enabled
     */
    future<void> replayEvents(const string& lastEventID, shared_ptr<ServerResponse> res);

    /**
     * Writes an event to the SSE stream with proper formatting
     */
    bool writeSSEEvent(shared_ptr<ServerResponse> res, const JSONRPCMessage& message,
                       const optional<string>& EventID = nullopt);

    /**
     * Handles unsupported requests (PUT, PATCH, etc.)
     */
    future<void> handleUnsupportedRequest(shared_ptr<ServerResponse> res);

    /**
     * Handles POST requests containing JSON-RPC messages
     */
    future<void> handlePostRequest(const IncomingMessage& req, shared_ptr<ServerResponse> res,
                                   const optional<JSON>& parsedBody = nullopt);

    /**
     * Handles DELETE requests to terminate sessions
     */
    future<void> handleDeleteRequest(const IncomingMessage& req, shared_ptr<ServerResponse> res);

    /**
     * Validates session ID for non-initialization requests
     * Returns true if the session is valid, false otherwise
     */
    bool validateSession(const IncomingMessage& req, shared_ptr<ServerResponse> res);

  public:
    future<void> close() override;

    future<void> send(const JSONRPCMessage& message,
                      const optional<RequestID>& relatedRequestID = nullopt) override;

  private:
    /**
     * Generates a UUID-like string for request/stream IDs
     */
    string generateUUID();
};

// ##########################################################
// ##########################################################
// ===================== CLIENT SECTION =====================
// ##########################################################
// ##########################################################

// Default reconnection options for StreamableHTTP connections
struct StreamableHTTPReconnectionOptions {
    int maxReconnectionDelay = 30000;    // 30 seconds
    int initialReconnectionDelay = 1000; // 1 second
    double reconnectionDelayGrowFactor = 1.5;
    int maxRetries = 2;
};

const StreamableHTTPReconnectionOptions DEFAULT_STREAMABLE_HTTP_RECONNECTION_OPTIONS = {
    1000,  // initialReconnectionDelay
    30000, // maxReconnectionDelay
    1.5,   // reconnectionDelayGrowFactor
    2      // maxRetries
};

class StreamableHTTPError : public exception {
  private:
    optional<int> code_;
    string message_;
    string full_message_;

  public:
    const char* what() const noexcept override;

    optional<int> getCode() const;
    const string& getMessage() const;
};

/**
 * Options for starting or authenticating an SSE connection
 */
struct StartSSEOptions {
    optional<string> resumptionToken;
    function<void(const string&)> onresumptiontoken;
    optional<string> replayMessageId; // Can be string or number
};

/**
 * Configuration options for the `StreamableHTTPClientTransport`.
 */
struct StreamableHTTPClientTransportOptions {
    shared_ptr<OAuthClientProvider> authProvider;
    map<string, string> requestHeaders; // RequestInit equivalent
    StreamableHTTPReconnectionOptions reconnectionOptions =
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
    atomic<bool> abort_requested_;
    string url_;
    optional<string> resource_metadata_url_;
    map<string, string> request_headers_;
    shared_ptr<OAuthClientProvider> auth_provider_;
    optional<string> session_id_;
    StreamableHTTPReconnectionOptions reconnection_options_;

  public:
    function<void()> onclose;
    function<void(const exception&)> onerror;
    function<void(const JSONRPCMessage&)> onmessage;

    StreamableHTTPClientTransport(const string& url,
                                  const StreamableHTTPClientTransportOptions& opts = {})
        : abort_requested_(false), url_(url), request_headers_(opts.requestHeaders),
          auth_provider_(opts.authProvider), session_id_(opts.SessionID),
          reconnection_options_(opts.reconnectionOptions) {}

  private:
    future<void> authThenStart();

    future<map<string, string>> commonHeaders();

    future<void> startOrAuthSse(const StartSSEOptions& options);

    int getNextReconnectionDelay(int attempt);

    void scheduleReconnection(const StartSSEOptions& options, int attemptCount = 0);

    void handleSseStream(const string& streamData, const StartSSEOptions& options);

  public:
    future<void> start();

    /**
     * Call this method after the user has finished authorizing via their user agent and is
     * redirected back to the MCP client application.
     */
    future<void> finishAuth(const string& authorizationCode);

    future<void> close();

    struct SendOptions {
        optional<string> resumptionToken;
        function<void(const string&)> onresumptiontoken;
    };

    future<void> send(const JSONRPCMessage& message, const SendOptions& options = {});

    future<void> send(const vector<JSONRPCMessage>& messages, const SendOptions& options = {});

    optional<string> getSessionID() const;

    /**
     * Terminates the current session by sending a DELETE request to the server.
     */
    future<void> terminateSession();
};

MCP_NAMESPACE_END