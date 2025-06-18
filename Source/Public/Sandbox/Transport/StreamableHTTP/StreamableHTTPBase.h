#pragma once

#include "../ITransport.h"
#include "Communication/Transport/EventStore.h"
#include "Core.h"
#include "HTTPProxy.h"
#include "SSEEvent.h"
#include "SSEStream.h"

// Poco Net includes
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/StreamCopier.h>
#include <Poco/URI.h>

MCP_NAMESPACE_BEGIN

// HTTP Transport Configuration shared between client and server
struct HTTPTransportConfig {
    string Host = "localhost";
    int Port = 8080;
    string MCPEndpoint = "/mcp";
    bool UseSSL = false;
    chrono::seconds RequestTimeout = chrono::seconds(30);
    bool ValidateOrigin = true;
    vector<string> AllowedOrigins;
};

// Session management for HTTP transport
struct MCPSession {
    string SessionID;
    chrono::system_clock::time_point CreatedAt;
    chrono::system_clock::time_point LastActivity;
    bool IsActive;

    MCPSession(const string& InSessionID)
        : SessionID(InSessionID), CreatedAt(chrono::system_clock::now()),
          LastActivity(chrono::system_clock::now()), IsActive(true) {}
};

// Reconnection options for client connections
struct StreamableHTTPReconnectionOptions {
    int MaxReconnectionDelay = 30000;    // 30 seconds
    int InitialReconnectionDelay = 1000; // 1 second
    double ReconnectionDelayGrowFactor = 1.5;
    int MaxRetries = 2;
};

static inline constexpr StreamableHTTPReconnectionOptions
    DEFAULT_STREAMABLE_HTTP_RECONNECTION_OPTIONS = {1000, 30000, 1.5, 2};

// Base class for Streamable HTTP transport implementations
class StreamableHTTPBase : public ITransport {
  protected:
    HTTPTransportConfig m_Config;
    optional<MCPSession> m_Session;
    function<void(const MessageBase&)> m_MessageHandler;
    function<void(const string&)> m_ErrorHandler;
    bool m_IsConnected;
    string m_LastEventID;

    // EventStore for resumability support
    shared_ptr<EventStore> m_EventStore;

  public:
    explicit StreamableHTTPBase(const HTTPTransportConfig& InConfig)
        : m_Config(InConfig), m_IsConnected(false) {}

    ~StreamableHTTPBase() override = default;

    // ITransport interface
    void SetMessageHandler(function<void(const MessageBase&)> InHandler) override {
        m_MessageHandler = InHandler;
    }

    void SetErrorHandler(function<void(const string&)> InHandler) override {
        m_ErrorHandler = InHandler;
    }

    bool IsConnected() const override {
        return m_IsConnected;
    }

    TransportType GetTransportType() const override {
        return TransportType::HTTP;
    }

    // Session management helpers
    bool HasValidSession() const {
        return m_Session.has_value() && m_Session->IsActive;
    }

    optional<string> GetSessionID() const {
        return m_Session.has_value() ? make_optional(m_Session->SessionID) : nullopt;
    }

    void SetEventStore(shared_ptr<EventStore> InEventStore) {
        m_EventStore = InEventStore;
    }

  protected:
    // Creates a session from a session ID
    void CreateSession(const string& InSessionID) {
        m_Session = MCPSession(InSessionID);
    }

    // Invalidates the current session
    void InvalidateSession() {
        if (m_Session.has_value()) { m_Session->IsActive = false; }
    }

    // Updates session activity timestamp
    void UpdateSessionActivity() {
        if (m_Session.has_value()) { m_Session->LastActivity = chrono::system_clock::now(); }
    }

    // Helper to format SSE events according to the spec
    static string FormatSSEEvent(const string& InEvent, const string& InData,
                                 const optional<string>& InID = nullopt) {
        stringstream ss;
        if (InID) { ss << "id: " << *InID << "\n"; }
        if (!InEvent.empty()) { ss << "event: " << InEvent << "\n"; }

        // SSE data MAY contain multiple lines; split on \n and prefix each line with "data: "
        size_t Start = 0;
        while (Start <= InData.size()) {
            size_t End = InData.find('\n', Start);
            string Line = InData.substr(Start, End == string::npos ? string::npos : End - Start);
            ss << "data: " << Line << "\n";
            if (End == string::npos) break;
            Start = End + 1;
        }
        ss << "\n"; // End of event
        return ss.str();
    }

    // Processes an SSE event and calls the message handler
    void ProcessSSEEvent(const SSEEvent& InEvent) {
        if (!InEvent.Data.empty()) {
            try {
                // Update last event ID for resumability
                if (!InEvent.ID.empty()) { m_LastEventID = InEvent.ID; }

                // Parse JSON-RPC message from event data
                MessageBase Message = DeserializeFromJSON(InEvent.Data);
                if (m_MessageHandler) { m_MessageHandler(Message); }
            } catch (const exception& Ex) {
                CallErrorHandler("Failed to parse SSE message: " + string(Ex.what()));
            }
        }
    }

    // Helper to safely invoke error handler
    void CallErrorHandler(const string& InError) {
        if (m_ErrorHandler) { m_ErrorHandler(InError); }
    }

    // Helper to invoke message handler
    void CallMessageHandler(const MessageBase& InMessage) {
        if (m_MessageHandler) { m_MessageHandler(InMessage); }
    }

    // Virtual methods for subclasses to implement JSON serialization
    virtual string SerializeToJSON(const MessageBase& InMessage) = 0;
    virtual MessageBase DeserializeFromJSON(const string& InJSON) = 0;
};

MCP_NAMESPACE_END