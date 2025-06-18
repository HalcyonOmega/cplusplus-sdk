#pragma once

#include "Auth/Providers/Provider.h"
#include "Core.h"
#include "Sandbox/IMCP.h"
#include "StreamableHTTPBase.h"

// Poco Net includes
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/StreamCopier.h>
#include <Poco/Thread.h>

MCP_NAMESPACE_BEGIN

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