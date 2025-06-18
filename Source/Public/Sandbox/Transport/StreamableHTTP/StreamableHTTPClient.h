#pragma once

#include "../ITransport.h"
#include "Auth/Providers/Provider.h"
#include "Core.h"
#include "Proxies/URIProxy.h"
#include "Sandbox/IMCP.h"
#include "StreamableHTTPBase.h"

// Poco Net includes
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/StreamCopier.h>
#include <Poco/Thread.h>

MCP_NAMESPACE_BEGIN

// TODO: @HalcyonOmega Begin Direct Translated Code
/**
 * Options for starting or authenticating an SSE connection
 */
struct StartSSEOptions {
    using ResumptionTokenCallback = function<void(const string& /* Token */)>;

    /**
     * The resumption token used to continue long-running requests that were interrupted.
     *
     * This allows clients to reconnect and continue from where they left off.
     */
    optional<string> ResumptionToken;

    /**
     * A callback that is invoked when the resumption token changes.
     *
     * This allows clients to persist the latest token for potential reconnection.
     */
    optional<ResumptionTokenCallback> OnResumptionToken;

    /**
     * Override Message ID to associate with the replay message
     * so that response can be associate with the new resumed request.
     */
    optional<variant<string, int>> ReplayMessageID;
};

/**
 * Configuration options for reconnection behavior of the StreamableHTTPClientTransport.
 */
struct StreamableHTTPReconnectionOptions {
    /**
     * Maximum backoff time between reconnection attempts in milliseconds.
     * Default is 30000 (30 seconds).
     */
    int MaxReconnectionDelay;

    /**
     * Initial backoff time between reconnection attempts in milliseconds.
     * Default is 1000 (1 second).
     */
    int InitialReconnectionDelay;

    /**
     * The factor by which the reconnection delay increases after each attempt.
     * Default is 1.5.
     */
    double ReconnectionDelayGrowFactor;

    /**
     * Maximum number of reconnection attempts before giving up.
     * Default is 2.
     */
    int MaxRetries;
};

/**
 * Configuration options for the `StreamableHTTPClientTransport`.
 */
struct StreamableHTTPClientTransportOptions {
    /**
     * An OAuth client provider to use for authentication.
     *
     * When an `authProvider` is specified and the connection is started:
     * 1. The connection is attempted with any existing access token from the `authProvider`.
     * 2. If the access token has expired, the `authProvider` is used to refresh the token.
     * 3. If token refresh fails or no access token exists, and auth is required,
     * `OAuthClientProvider.redirectToAuthorization` is called, and an `UnauthorizedError` will be
     * thrown from `connect`/`start`.
     *
     * After the user has finished authorizing via their user agent, and is redirected back to the
     * MCP client application, call `StreamableHTTPClientTransport.finishAuth` with the
     * authorization code before retrying the connection.
     *
     * If an `authProvider` is not provided, and auth is required, an `UnauthorizedError` will be
     * thrown.
     *
     * `UnauthorizedError` might also be thrown when sending any message over the transport,
     * indicating that the session has expired, and needs to be re-authed and reconnected.
     */
    optional<OAuthClientProvider> AuthProvider;

    /**
     * Customizes HTTP requests to the server.
     */
    optional<RequestInit> RequestInit;

    /**
     * Options to configure the reconnection behavior.
     */
    optional<StreamableHTTPReconnectionOptions> ReconnectionOptions;

    /**
     * Session ID for the connection. This is used to identify the session on the server.
     * When not provided and connecting to a server that supports session IDs, the server will
     * generate a new session ID.
     */
    optional<string> SessionID;
};

/**
 * Client transport for Streamable HTTP: this implements the MCP Streamable HTTP transport
 * specification. It will connect to a server using HTTP POST for sending messages and HTTP GET with
 * Server-Sent Events for receiving messages.
 */
class StreamableHTTPClientTransport : public ITransport {
  private:
    URL m_URL;
    StreamableHTTPReconnectionOptions m_ReconnectionOptions;
    optional<AbortController> m_AbortController;
    optional<URL> m_ResourceMetadataURL;
    optional<RequestInit> m_RequestInit;
    optional<OAuthClientProvider> m_AuthProvider;
    optional<string> m_SessionID;

    MCPTask_Void AuthThenStart();
    MCPTask<HTTP::Headers> CommonHeaders();
    MCPTask_Void StartOrAuthSSE(const StartSSEOptions& InOptions);
    int GetNextReconnectionDelay(int InAttemptCount);
    void ScheduleReconnection(const StartSSEOptions& InOptions, int InAttemptCount = 0);
    void HandleSSEStream(std::istream* InStream, const StartSSEOptions& InOptions);

  public:
    // === ITransport Implementation ===
    MCPTask_Void Connect() override;
    MCPTask_Void Disconnect() override;
    MCPTask_Void SendMessage(const MessageBase& InMessage) override;
    // === End ITransport Implementation ===

    MCPTask_Void FinishAuth(const string& InAuthorizationCode);
    MCPTask_Void TerminateSession();
    optional<string> GetSessionID() const;
};

// TODO: @HalcyonOmega End Direct Translated Code

// Configuration options for HTTP client transport
struct HTTPClientConfig {
    string Host;
    int Port{80};
    bool UseHTTPS{false};
    string BasePath{"/"};
    bool EnableResumability{false};
    std::chrono::milliseconds RequestTimeout{30000};
    std::chrono::milliseconds ConnectionTimeout{10000};
    int MaxReconnectionAttempts{5};
    bool EnableStatefulMode{false};
    vector<string> AllowedOrigins;
    shared_ptr<OAuthServerProvider> AuthProvider;
    shared_ptr<EventStore> EventStore;
};

// Client implementation of Streamable HTTP transport
// Handles client-specific transport behavior only
class StreamableHTTPClient : public StreamableHTTPBase {
  public:
    explicit StreamableHTTPClient(const HTTPClientConfig& InConfig);
    virtual ~StreamableHTTPClient() = default;

    // === Client-Specific Operations ===

    /**
     * Client-specific connection initialization sequence
     * Establishes HTTP session and initiates MCP handshake
     */
    virtual MCPTask_Void InitializeConnection();

    /**
     * Starts receiving messages from server (GET SSE stream)
     * Begins listening for server-sent events and processes incoming messages
     */
    virtual MCPTask_Void StartMessageReceiving();

    /**
     * Stops receiving messages from server
     * Closes SSE stream and stops message processing loop
     */
    virtual MCPTask_Void StopMessageReceiving();

    // === Client Connection Management ===

    /**
     * Handles client session lifecycle
     * Manages session creation, validation, and renewal
     */
    virtual MCPTask_Void ManageClientSession();

    /**
     * Client-specific reconnection logic
     * Implements exponential backoff and session recovery
     */
    virtual MCPTask_Void HandleConnectionLoss();

    /**
     * Client-initiated session termination
     * Sends DELETE request to gracefully terminate session
     */
    virtual MCPTask_Void TerminateClientSession();

    // === Configuration Access ===

    /**
     * Gets the current client configuration
     */
    const HTTPClientConfig& GetConfig() const {
        return m_Config;
    }

    /**
     * Updates client configuration (requires reconnection)
     */
    void UpdateConfig(const HTTPClientConfig& InNewConfig);

  protected:
    // === Protected Client Methods ===

    /**
     * Creates HTTP client session with appropriate settings
     */
    virtual unique_ptr<Poco::Net::HTTPClientSession> CreateClientSession();

    /**
     * Establishes SSE stream connection for receiving messages
     */
    virtual MCPTask_Void EstablishSSEStream();

    /**
     * Processes incoming SSE events from server
     */
    virtual MCPTask_Void ProcessServerSentEvents();

    /**
     * Handles SSE connection interruption and recovery
     */
    virtual MCPTask_Void HandleSSEDisconnection();

    /**
     * Validates server response for session management
     */
    virtual bool ValidateServerResponse(const Poco::Net::HTTPResponse& InResponse);

    /**
     * Extracts session information from server headers
     */
    virtual optional<string> ExtractSessionFromHeaders(const Poco::Net::HTTPResponse& InResponse);

  private:
    // === Member Variables ===

    // Client configuration
    HTTPClientConfig m_Config;

    // SSE stream management
    unique_ptr<std::istream> m_SSEStream;
    bool m_IsReceivingMessages{false};
    std::thread m_MessageReceiveThread;

    // Authentication
    shared_ptr<OAuthServerProvider> m_AuthProvider;

    // Connection state
    std::chrono::steady_clock::time_point m_LastServerContact;
    bool m_IsHandshakeComplete{false};

    // Thread synchronization
    mutable std::mutex m_ConfigMutex;
    mutable std::mutex m_StreamMutex;
    std::atomic<bool> m_ShouldStopReceiving{false};

    // Client-specific session data
    optional<string> m_ServerProtocolVersion;
    optional<string> m_ServerImplementationName;
};

MCP_NAMESPACE_END