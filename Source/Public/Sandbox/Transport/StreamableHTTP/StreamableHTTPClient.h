#pragma once

#include "Auth/Providers/Provider.h"
#include "Core.h"
#include "StreamableHTTPBase.h"

// Poco Net includes
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/StreamCopier.h>
#include <Poco/Thread.h>

MCP_NAMESPACE_BEGIN

// Client-specific configuration options
struct StreamableHTTPClientOptions {
    // Authentication provider for OAuth flows
    shared_ptr<OAuthClientProvider> AuthProvider;

    // Additional request headers
    map<string, string> RequestHeaders;

    // Reconnection settings
    StreamableHTTPReconnectionOptions ReconnectionOptions =
        DEFAULT_STREAMABLE_HTTP_RECONNECTION_OPTIONS;

    // Optional session ID for reconnection
    optional<string> SessionID;
};

// Client implementation of Streamable HTTP transport
class StreamableHTTPClient : public StreamableHTTPBase {
  private:
    unique_ptr<Poco::Net::HTTPClientSession> m_HTTPSession;
    unique_ptr<SSEStream> m_SSEStream;
    StreamableHTTPClientOptions m_ClientOptions;
    thread m_ReadThread;
    atomic<bool> m_ReadThreadRunning;
    atomic<bool> m_AbortRequested;

    // Reconnection state
    int m_ReconnectionAttempts;
    chrono::steady_clock::time_point m_LastReconnectTime;

  public:
    explicit StreamableHTTPClient(const HTTPTransportConfig& InConfig,
                                  const StreamableHTTPClientOptions& InOptions = {})
        : StreamableHTTPBase(InConfig), m_ClientOptions(InOptions), m_ReadThreadRunning(false),
          m_AbortRequested(false), m_ReconnectionAttempts(0) {
        // Set session ID if provided
        if (InOptions.SessionID.has_value()) { CreateSession(InOptions.SessionID.value()); }
    }

    ~StreamableHTTPClient() override {
        Disconnect().wait();
    }

    // ITransport interface implementation
    MCPTask_Void Connect() override {
        try {
            // Create HTTP session using Poco
            if (m_Config.UseSSL) {
                m_HTTPSession =
                    make_unique<Poco::Net::HTTPSClientSession>(m_Config.Host, m_Config.Port);
            } else {
                m_HTTPSession =
                    make_unique<Poco::Net::HTTPClientSession>(m_Config.Host, m_Config.Port);
            }

            m_HTTPSession->setTimeout(Poco::Timespan(m_Config.RequestTimeout.count(), 0));

            // Send initialize request to establish session
            co_await SendInitializeRequest();

            m_IsConnected = true;

            // Start reading thread for SSE messages
            StartReadingThread();

        } catch (const Poco::Exception& Ex) {
            CallErrorHandler("Failed to connect HTTP transport: " + Ex.displayText());
            throw runtime_error("HTTP connection failed: " + Ex.displayText());
        }

        co_return;
    }

    MCPTask_Void Disconnect() override {
        try {
            m_AbortRequested = true;

            // Send session termination if session exists
            if (HasValidSession()) { co_await SendSessionTermination(); }

            // Stop reading thread
            StopReadingThread();

            // Close SSE stream
            if (m_SSEStream) {
                m_SSEStream->Close();
                m_SSEStream.reset();
            }

            // Close HTTP session
            if (m_HTTPSession) {
                m_HTTPSession->reset();
                m_HTTPSession.reset();
            }

            m_IsConnected = false;

        } catch (const Poco::Exception& Ex) {
            CallErrorHandler("Error during disconnect: " + Ex.displayText());
        }

        co_return;
    }

    MCPTask_Void SendMessage(const MessageBase& InMessage) override {
        if (!m_IsConnected || !m_HTTPSession) { throw runtime_error("Transport not connected"); }

        try {
            // Create POST request according to MCP spec
            Poco::Net::HTTPRequest Request = CreatePOSTRequest();
            AddHeaders(Request);
            AddSessionHeader(Request);

            // Send HTTP request with message
            co_await SendHTTPRequest(Request, InMessage);

        } catch (const Poco::Exception& Ex) {
            CallErrorHandler("HTTP request failed: " + Ex.displayText());
            throw runtime_error("HTTP request failed: " + Ex.displayText());
        }

        co_return;
    }

    MCPTask_Void SendBatch(const JSONRPCBatch& InBatch) override {
        // Convert batch to message and send
        MessageBase BatchMessage = ConvertBatchToMessage(InBatch);
        co_await SendMessage(BatchMessage);
        co_return;
    }

    // Starts an SSE stream for server messages
    MCPTask_Void StartServerMessageStream() {
        if (!m_IsConnected || !m_HTTPSession) { throw runtime_error("Transport not connected"); }

        try {
            // Create GET request for SSE stream
            Poco::Net::HTTPRequest GetRequest = CreateGETRequest();
            AddHeaders(GetRequest);
            AddSessionHeader(GetRequest);
            AddSSEAcceptHeader(GetRequest);
            AddLastEventIDHeader(GetRequest);

            // Send request and receive SSE stream
            Poco::Net::HTTPResponse Response;
            istream& ResponseStream = m_HTTPSession->receiveResponse(Response);

            if (Response.getStatus() == Poco::Net::HTTPResponse::HTTP_OK
                && Response.getContentType() == "text/event-stream") {
                // Create SSE stream wrapper
                m_SSEStream = make_unique<SSEStream>(make_unique<istream>(ResponseStream.rdbuf()));

                // Process SSE events
                while (m_SSEStream && m_SSEStream->IsOpen() && !m_AbortRequested) {
                    SSEEvent Event = co_await m_SSEStream->ReadEvent();
                    ProcessSSEEvent(Event);
                }

            } else if (Response.getStatus() == Poco::Net::HTTPResponse::HTTP_METHOD_NOT_ALLOWED) {
                // Server doesn't support SSE streams
                CallErrorHandler("Server does not support SSE streams");
            } else {
                HandleHTTPError(Response);
            }

        } catch (const Poco::Exception& Ex) {
            CallErrorHandler("SSE stream error: " + Ex.displayText());

            // Attempt reconnection if configured
            if (ShouldAttemptReconnection()) { co_await AttemptReconnection(); }
        }

        co_return;
    }

    // Terminates the current session
    MCPTask_Void TerminateSession() {
        if (!HasValidSession() || !m_HTTPSession) { co_return; }

        co_await SendSessionTermination();
        co_return;
    }

  protected:
    // Implement JSON serialization - this would use the actual JSON system
    string SerializeToJSON(const MessageBase& InMessage) override {
        // TODO: Implement proper JSON serialization using the codebase's JSON system
        return "{}"; // Placeholder
    }

    MessageBase DeserializeFromJSON(const string& InJSON) override {
        // TODO: Implement proper JSON deserialization using the codebase's JSON system
        MessageBase Message;
        return Message; // Placeholder
    }

  private:
    // Sends an initialize request to establish the session
    MCPTask_Void SendInitializeRequest() {
        MessageBase InitRequest = CreateInitializeRequest();

        Poco::Net::HTTPRequest Request = CreatePOSTRequest();
        AddHeaders(Request);
        // No session header for initialization

        Poco::Net::HTTPResponse Response;
        co_await SendHTTPRequestWithResponse(Request, Response, InitRequest);

        // Extract session ID from response if present
        if (Response.has("Mcp-Session-Id")) {
            string SessionID = Response.get("Mcp-Session-Id");
            CreateSession(SessionID);
        }

        co_await ProcessHTTPResponse(Response);
        co_return;
    }

    // Sends session termination request
    MCPTask_Void SendSessionTermination() {
        if (!HasValidSession() || !m_HTTPSession) { co_return; }

        try {
            Poco::Net::HTTPRequest DeleteRequest(Poco::Net::HTTPRequest::HTTP_DELETE,
                                                 m_Config.MCPEndpoint,
                                                 Poco::Net::HTTPMessage::HTTP_1_1);
            AddSessionHeader(DeleteRequest);

            m_HTTPSession->sendRequest(DeleteRequest);

            Poco::Net::HTTPResponse Response;
            m_HTTPSession->receiveResponse(Response);

            // Server may respond with 405 if termination not supported
            if (Response.getStatus() != Poco::Net::HTTPResponse::HTTP_METHOD_NOT_ALLOWED) {
                InvalidateSession();
            }

        } catch (const Poco::Exception& Ex) {
            CallErrorHandler("Session termination failed: " + Ex.displayText());
        }

        co_return;
    }

    // HTTP request creation helpers
    Poco::Net::HTTPRequest CreatePOSTRequest() {
        Poco::Net::HTTPRequest Request(Poco::Net::HTTPRequest::HTTP_POST, m_Config.MCPEndpoint,
                                       Poco::Net::HTTPMessage::HTTP_1_1);
        Request.setContentType("application/json");
        return Request;
    }

    Poco::Net::HTTPRequest CreateGETRequest() {
        return Poco::Net::HTTPRequest(Poco::Net::HTTPRequest::HTTP_GET, m_Config.MCPEndpoint,
                                      Poco::Net::HTTPMessage::HTTP_1_1);
    }

    // Header management
    void AddHeaders(Poco::Net::HTTPRequest& InRequest) {
        // Add Accept header as required by MCP spec
        InRequest.set("Accept", "application/json, text/event-stream");

        // Add custom headers from options
        for (const auto& [Key, Value] : m_ClientOptions.RequestHeaders) {
            InRequest.set(Key, Value);
        }
    }

    void AddSessionHeader(Poco::Net::HTTPRequest& InRequest) {
        if (HasValidSession()) { InRequest.set("Mcp-Session-Id", GetSessionID().value()); }
    }

    void AddSSEAcceptHeader(Poco::Net::HTTPRequest& InRequest) {
        InRequest.set("Accept", "text/event-stream");
    }

    void AddLastEventIDHeader(Poco::Net::HTTPRequest& InRequest) {
        if (!m_LastEventID.empty()) { InRequest.set("Last-Event-ID", m_LastEventID); }
    }

    // HTTP request sending
    MCPTask_Void SendHTTPRequest(Poco::Net::HTTPRequest& InRequest, const MessageBase& InMessage) {
        string JsonData = SerializeToJSON(InMessage);
        InRequest.setContentLength(JsonData.length());

        ostream& RequestStream = m_HTTPSession->sendRequest(InRequest);
        RequestStream << JsonData;

        Poco::Net::HTTPResponse Response;
        m_HTTPSession->receiveResponse(Response);

        co_await ProcessHTTPResponse(Response);
        co_return;
    }

    MCPTask_Void SendHTTPRequestWithResponse(Poco::Net::HTTPRequest& InRequest,
                                             Poco::Net::HTTPResponse& OutResponse,
                                             const MessageBase& InMessage) {
        string JsonData = SerializeToJSON(InMessage);
        InRequest.setContentLength(JsonData.length());

        ostream& RequestStream = m_HTTPSession->sendRequest(InRequest);
        RequestStream << JsonData;

        m_HTTPSession->receiveResponse(OutResponse);
        co_return;
    }

    // Response processing
    MCPTask_Void ProcessHTTPResponse(const Poco::Net::HTTPResponse& InResponse) {
        if (InResponse.getStatus() == Poco::Net::HTTPResponse::HTTP_ACCEPTED) {
            // Accepted - for notifications/responses only
            co_return;
        }

        if (InResponse.getStatus() == Poco::Net::HTTPResponse::HTTP_NOT_FOUND
            && HasValidSession()) {
            // Session expired - need to reinitialize
            InvalidateSession();
            co_await Connect();
            co_return;
        }

        if (InResponse.getStatus() >= Poco::Net::HTTPResponse::HTTP_BAD_REQUEST) {
            HandleHTTPError(InResponse);
            co_return;
        }

        // Handle successful responses
        string ContentType = InResponse.getContentType();
        if (ContentType == "application/json") {
            // Single JSON response
            // Response body would be processed here
        } else if (ContentType == "text/event-stream") {
            // SSE stream initiated - handled separately
        }

        co_return;
    }

    void HandleHTTPError(const Poco::Net::HTTPResponse& InResponse) {
        string ErrorMsg =
            "HTTP Error " + to_string(InResponse.getStatus()) + ": " + InResponse.getReason();
        CallErrorHandler(ErrorMsg);
    }

    // Threading for SSE reading
    void StartReadingThread() {
        m_ReadThreadRunning = true;
        m_ReadThread = thread([this]() { ReadLoop(); });
    }

    void StopReadingThread() {
        m_ReadThreadRunning = false;
        if (m_ReadThread.joinable()) { m_ReadThread.join(); }
    }

    void ReadLoop() {
        while (m_ReadThreadRunning && !m_AbortRequested) {
            try {
                StartServerMessageStream().wait();
            } catch (const exception& Ex) {
                CallErrorHandler("Read loop error: " + string(Ex.what()));
            }

            // Brief pause before potential reconnection
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    }

    // Reconnection logic
    bool ShouldAttemptReconnection() {
        return m_ReconnectionAttempts < m_ClientOptions.ReconnectionOptions.MaxRetries
               && !m_AbortRequested;
    }

    MCPTask_Void AttemptReconnection() {
        auto Now = chrono::steady_clock::now();
        auto TimeSinceLastReconnect =
            chrono::duration_cast<chrono::milliseconds>(Now - m_LastReconnectTime);

        int DelayMs = CalculateReconnectionDelay();

        if (TimeSinceLastReconnect.count() < DelayMs) {
            this_thread::sleep_for(chrono::milliseconds(DelayMs - TimeSinceLastReconnect.count()));
        }

        m_LastReconnectTime = chrono::steady_clock::now();
        m_ReconnectionAttempts++;

        try {
            co_await Connect();
            m_ReconnectionAttempts = 0; // Reset on successful reconnection
        } catch (const exception& Ex) {
            CallErrorHandler("Reconnection attempt failed: " + string(Ex.what()));
        }

        co_return;
    }

    int CalculateReconnectionDelay() {
        int Delay = m_ClientOptions.ReconnectionOptions.InitialReconnectionDelay;
        for (int i = 0; i < m_ReconnectionAttempts; ++i) {
            Delay = static_cast<int>(
                Delay * m_ClientOptions.ReconnectionOptions.ReconnectionDelayGrowFactor);
            if (Delay > m_ClientOptions.ReconnectionOptions.MaxReconnectionDelay) {
                Delay = m_ClientOptions.ReconnectionOptions.MaxReconnectionDelay;
                break;
            }
        }
        return Delay;
    }

    // Placeholder methods that would be implemented with proper MCP message creation
    MessageBase CreateInitializeRequest() {
        // TODO: Create proper MCP initialize request
        MessageBase Request;
        return Request;
    }

    MessageBase ConvertBatchToMessage(const JSONRPCBatch& InBatch) {
        // TODO: Convert batch to proper message format
        MessageBase Message;
        return Message;
    }
};

MCP_NAMESPACE_END