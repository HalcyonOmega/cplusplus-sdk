#pragma once

#include "Core.h"
#include "Utilities/ThirdParty/UUID/UUIDLayer.h"

MCP_NAMESPACE_BEGIN

// TODO: Fix External Ref: Server-Sent Events implementation
// TODO: Fix External Ref: Raw body parsing
// TODO: Fix External Ref: Content-Type parsing

// TODO: Fix External Ref: URL parsing and manipulation
// TODO: Consider making a class since it has logic
struct URLHelper {
    static string AddSessionParam(const string& InEndpoint, const string& InSessionID);
};

// TODO: Fix External Ref: Content-Type parsing
struct ContentTypeResult {
    string Type;
    map<string, string> Params;
};

ContentTypeResult ParseContentType(const string& InContentTypeHeader);

// TODO: Fix External Ref: Raw body parsing
string GetRawBodyEquivalent(HTTP_Request* InRequest, const string& InLimit,
                            const string& InEncoding);

const string MAXIMUM_MESSAGE_SIZE = "4mb";

/**
 * Server transport for SSE: this will send messages over an SSE connection and receive messages
 * from HTTP POST requests.
 *
 * This transport is only available in Node.js environments.
 */
class SSEServerTransport {
  private:
    optional<HTTP_Response*> m_SSEResponse;
    string m_SessionID;
    string m_Endpoint;
    HTTP_Response* m_Res;

  public:
    optional<function<void()>> OnClose;
    optional<function<void(const Error&)>> OnError;
    optional<function<void(const MessageBase& InMessage,
                           const optional<map<string, AuthInfo>>& InExtra)>>
        OnMessage;

    /**
     * Creates a new SSE server transport, which will direct the client to POST messages to the
     * relative or absolute URL identified by _endpoint.
     */
    SSEServerTransport(const string& InEndpoint, HTTP_Response* InResParam)
        : m_Endpoint(InEndpoint), m_Res(InResParam), m_SSEResponse(nullopt) {
        m_SessionID = GenerateUUID();
    }

    /**
     * Handles the initial SSE connection request.
     *
     * This should be called when a GET request is made to establish the SSE stream.
     */
    void Start();

    /**
     * Handles incoming POST messages.
     *
     * This should be called when a POST request is made to send a message to the server.
     */
    void HandlePostMessage(HTTP_Request* InRequest, HTTP_Response* InResponse,
                           const optional<JSON>& InParsedBody = nullopt);

    /**
     * Handle a client message, regardless of how it arrived. This can be used to inform the server
     * of messages that arrive via a means different than HTTP POST.
     */
    void HandleMessage(const JSON& InMessage,
                       const optional<map<string, AuthInfo>>& InExtra = nullopt);

    void Close();

    void Send(const MessageBase& InMessage);

    /**
     * Returns the session ID for this transport.
     *
     * This can be used to route incoming POST requests.
     */
    string GetSessionID() const;
};

// ##########################################################
// ##########################################################
// ===================== CLIENT SECTION =====================
// ##########################################################
// ##########################################################

// URL class placeholder
// TODO: Fix External Ref: Implement proper URL class
class URL {
  public:
    string Href;
    string Origin;

    URL(const string& InURLString) : href(InURLString) {
        // TODO: Proper URL parsing
        Origin = InURLString; // Simplified
    }

    URL(const string& InRelative, const URL& InBase) {
        // TODO: Proper relative URL resolution
        Href = InBase.Href + "/" + InRelative;
        Origin = InBase.Origin;
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
    optional<int> Code;
    string Message;
};

class SSEError : public exception {
  public:
    SSEError(optional<int> InCode, const string& InMessage, const ErrorEvent& InEvent)
        : Code_(InCode), Event_(InEvent) {
        Message_ = "SSE error: " + InMessage;
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
    shared_ptr<OAuthClientProvider> AuthProvider = nullptr;

    /**
     * Customizes the initial SSE request to the server (the request that begins the stream).
     *
     * NOTE: Setting this property will prevent an Authorization header from
     * being automatically attached to the SSE request, if an authProvider is
     * also given. This can be worked around by setting the Authorization header
     * manually.
     */
    optional<EventSourceInit> EventSourceInit = nullopt;

    /**
     * Customizes recurring POST requests to the server.
     */
    optional<RequestInit> RequestInit = nullopt;
};

/**
 * Client transport for SSE: this will connect to a server using Server-Sent Events for receiving
 * messages and make separate POST requests for sending messages.
 */
class SSEClientTransport : public Transport {
  public:
    SSEClientTransport(const URL& InURL, const optional<SSEClientTransportOptions>& InOpts)
        : _eventSource(nullptr), _endpoint(nullopt), _abortController(nullptr), _url(InURL),
          _resourceMetadataUrl(nullopt),
          _eventSourceInit(InOpts() ? InOpts.value().EventSourceInit : nullopt),
          _requestInit(InOpts() ? InOpts.value().RequestInit : nullopt),
          _authProvider(InOpts() ? InOpts.value().AuthProvider : nullptr) {}

    // TODO: Identify proper constructor signature
    SSEClientTransport(const URL& InURL,
                       const optional<SSEClientTransportOptions>& InOpts = nullopt);

  private:
    // TODO: Fix External Ref: EventSource equivalent
    void* m_EventSource; // Placeholder for EventSource
    optional<URL> m_Endpoint;
    // TODO: Fix External Ref: AbortController equivalent
    void* m_AbortController; // Placeholder for AbortController
    URL m_URL;
    optional<URL> m_ResourceMetadataUrl;
    optional<EventSourceInit> m_EventSourceInit;
    optional<RequestInit> m_RequestInit;
    shared_ptr<OAuthClientProvider> m_AuthProvider;

  public:
    // Callback function types (exactly matching TypeScript)
    function<void()> OnClose;
    function<void(const exception& error)> OnError;
    function<void(const MessageBase& InMessage)> OnMessage;

    // Public interface methods (async equivalent using futures for Promise pattern)
    future<void> Start();
    future<void> FinishAuth(const string& InAuthorizationCode);
    future<void> Close();
    future<void> Send(const MessageBase& InMessage);

  private:
    // Private methods (exactly matching TypeScript structure)
    future<void> m_AuthThenStart();
    future<HeadersInit> m_CommonHeaders();
    future<void> m_StartOrAuth();

    // TODO: Fix External Ref: Auth function
    future<AuthResult> Auth(shared_ptr<OAuthClientProvider> InAuthProvider,
                            const map<string, variant<URL, string>>& InParams);

    // TODO: Fix External Ref: extractResourceMetadataUrl function
    optional<URL> ExtractResourceMetadataUrl(const HTTP_Response& InResponse);

    // TODO: Fix External Ref: HTTP functionality
    future<HTTP_Response> Fetch(const URL& InURL, const RequestInit& InInit);
};

MCP_NAMESPACE_END
