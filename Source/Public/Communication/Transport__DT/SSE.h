#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// TODO: Fix External Ref: HTTP Server Response functionality
// TODO: Fix External Ref: HTTP Request/Response handling
// TODO: Fix External Ref: Server-Sent Events implementation
// TODO: Fix External Ref: Raw body parsing
// TODO: Fix External Ref: Content-Type parsing

// Forward declarations for external dependencies
struct IncomingMessage {
    // TODO: Implement HTTP request structure
    map<string, string> headers;
    optional<AuthInfo> auth; // Optional auth info
};

// TODO: Consider making a class since it has logic
struct ServerResponse {
    // TODO: Implement HTTP response structure
    bool is_ended = false;

    void writeHead(int status_code, const optional<map<string, string>>& headers = nullopt);

    void write(const string& data);

    void end(const optional<string>& data = nullopt);

    void on(const string& event, optional<function<void()>> callback);
};

// TODO: Fix External Ref: UUID generation
string generateRandomUUID();

// TODO: Fix External Ref: URL parsing and manipulation
// TODO: Consider making a class since it has logic
struct URLHelper {
    static string addSessionParam(const string& endpoint, const string& session_id);
};

// TODO: Fix External Ref: Content-Type parsing
struct ContentTypeResult {
    string type;
    map<string, string> parameters;
};

ContentTypeResult parseContentType(const string& content_type_header);

// TODO: Fix External Ref: Raw body parsing
string getRawBodyEquivalent(IncomingMessage* req, const string& limit, const string& encoding);

const string MAXIMUM_MESSAGE_SIZE = "4mb";

/**
 * Server transport for SSE: this will send messages over an SSE connection and receive messages
 * from HTTP POST requests.
 *
 * This transport is only available in Node.js environments.
 */
class SSEServerTransport {
  private:
    optional<ServerResponse*> _sseResponse;
    string _sessionId;
    string _endpoint;
    ServerResponse* res;

  public:
    optional<function<void()>> onclose;
    optional<function<void(const Error&)>> onerror;
    optional<function<void(const JSONRPCMessage&, const optional<map<string, AuthInfo>>& extra)>>
        onmessage;

    /**
     * Creates a new SSE server transport, which will direct the client to POST messages to the
     * relative or absolute URL identified by _endpoint.
     */
    SSEServerTransport(const string& endpoint, ServerResponse* res_param)
        : _endpoint(endpoint), res(res_param), _sseResponse(nullopt) {
        _sessionId = generateRandomUUID();
    }

    /**
     * Handles the initial SSE connection request.
     *
     * This should be called when a GET request is made to establish the SSE stream.
     */
    void start();

    /**
     * Handles incoming POST messages.
     *
     * This should be called when a POST request is made to send a message to the server.
     */
    void handlePostMessage(IncomingMessage* req, ServerResponse* res_param,
                           const optional<JSON>& parsedBody = nullopt);

    /**
     * Handle a client message, regardless of how it arrived. This can be used to inform the server
     * of messages that arrive via a means different than HTTP POST.
     */
    void handleMessage(const JSON& message, const optional<map<string, AuthInfo>>& extra = nullopt);

    void close();

    void send(const JSONRPCMessage& message);

    /**
     * Returns the session ID for this transport.
     *
     * This can be used to route incoming POST requests.
     */
    string sessionId() const;
};

// ##########################################################
// ##########################################################
// ===================== CLIENT SECTION =====================
// ##########################################################
// ##########################################################

// HTTP Response struct (moved up for forward declaration)
// TODO: Consider making a class since it has logic
struct HttpResponse {
    int status;
    string body;
    map<string, string> headers;
    bool ok;

    future<string> text() const;
};

// URL class placeholder
// TODO: Fix External Ref: Implement proper URL class
class URL {
  public:
    string href;
    string origin;

    URL(const string& url_string) : href(url_string) {
        // TODO: Proper URL parsing
        origin = url_string; // Simplified
    }

    URL(const string& relative, const URL& base) {
        // TODO: Proper relative URL resolution
        href = base.href + "/" + relative;
        origin = base.origin;
    }
};

// HeadersInit type equivalent
using HeadersInit = map<string, string>;

// EventSourceInit equivalent
using EventSourceInit = map<string, string>;

// RequestInit equivalent - using variant to handle different value types
using RequestInit = map<string, variant<string, HeadersInit, bool>>;

// ErrorEvent equivalent
// TODO: Fix External Ref: Implement proper ErrorEvent
struct ErrorEvent {
    optional<int> code;
    string message;
};

class SseError : public exception {
  public:
    SseError(optional<int> code, const string& message, const ErrorEvent& event)
        : code_(code), event_(event) {
        message_ = "SSE error: " + message;
    }

    const char* what() const noexcept;

    optional<int> GetCode() const;

    const ErrorEvent& GetEvent() const;

  private:
    optional<int> code_;
    string message_;
    ErrorEvent event_;
};

/**
 * Configuration options for the SSEClientTransport.
 */
struct SSEClientTransportOptions {
    /**
     * An OAuth client provider to use for authentication.
     *
     * When an authProvider is specified and the SSE connection is started:
     * 1. The connection is attempted with any existing access token from the authProvider.
     * 2. If the access token has expired, the authProvider is used to refresh the token.
     * 3. If token refresh fails or no access token exists, and auth is required,
     * OAuthClientProvider.redirectToAuthorization is called, and an UnauthorizedError will be
     * thrown from connect/start.
     *
     * After the user has finished authorizing via their user agent, and is redirected back to the
     * MCP client application, call SSEClientTransport.finishAuth with the authorization code before
     * retrying the connection.
     *
     * If an authProvider is not provided, and auth is required, an UnauthorizedError will be
     * thrown.
     *
     * UnauthorizedError might also be thrown when sending any message over the SSE transport,
     * indicating that the session has expired, and needs to be re-authed and reconnected.
     */
    shared_ptr<OAuthClientProvider> authProvider = nullptr;

    /**
     * Customizes the initial SSE request to the server (the request that begins the stream).
     *
     * NOTE: Setting this property will prevent an Authorization header from
     * being automatically attached to the SSE request, if an authProvider is
     * also given. This can be worked around by setting the Authorization header
     * manually.
     */
    optional<EventSourceInit> eventSourceInit = nullopt;

    /**
     * Customizes recurring POST requests to the server.
     */
    optional<RequestInit> requestInit = nullopt;
};

/**
 * Client transport for SSE: this will connect to a server using Server-Sent Events for receiving
 * messages and make separate POST requests for sending messages.
 */
class SSEClientTransport : public Transport {
  public:
    SSEClientTransport(const URL& url, const optional<SSEClientTransportOptions>& opts)
        : _eventSource(nullptr), _endpoint(nullopt), _abortController(nullptr), _url(url),
          _resourceMetadataUrl(nullopt),
          _eventSourceInit(opts.has_value() ? opts.value().eventSourceInit : nullopt),
          _requestInit(opts.has_value() ? opts.value().requestInit : nullopt),
          _authProvider(opts.has_value() ? opts.value().authProvider : nullptr) {}

    // TODO: Identify proper constructor signature
    SSEClientTransport(const URL& url, const optional<SSEClientTransportOptions>& opts = nullopt);

  private:
    // TODO: Fix External Ref: EventSource equivalent
    void* _eventSource; // Placeholder for EventSource
    optional<URL> _endpoint;
    // TODO: Fix External Ref: AbortController equivalent
    void* _abortController; // Placeholder for AbortController
    URL _url;
    optional<URL> _resourceMetadataUrl;
    optional<EventSourceInit> _eventSourceInit;
    optional<RequestInit> _requestInit;
    shared_ptr<OAuthClientProvider> _authProvider;

  public:
    // Callback function types (exactly matching TypeScript)
    function<void()> onclose;
    function<void(const exception& error)> onerror;
    function<void(const JSONRPCMessage& message)> onmessage;

    // Public interface methods (async equivalent using futures for Promise pattern)
    future<void> start();
    future<void> finishAuth(const string& authorizationCode);
    future<void> close();
    future<void> send(const JSONRPCMessage& message);

  private:
    // Private methods (exactly matching TypeScript structure)
    future<void> _authThenStart();
    future<HeadersInit> _commonHeaders();
    future<void> _startOrAuth();

    // TODO: Fix External Ref: Auth function
    future<AuthResult> auth(shared_ptr<OAuthClientProvider> authProvider,
                            const map<string, variant<URL, string>>& params);

    // TODO: Fix External Ref: extractResourceMetadataUrl function
    optional<URL> extractResourceMetadataUrl(const HttpResponse& response);

    // TODO: Fix External Ref: HTTP functionality
    future<HttpResponse> fetch(const URL& url, const RequestInit& init);
};

MCP_NAMESPACE_END
