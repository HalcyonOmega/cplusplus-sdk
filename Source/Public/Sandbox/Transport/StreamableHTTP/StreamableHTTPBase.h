#pragma once

#include "../ITransport.h"
#include "Communication/Transport/EventStore.h"
#include "Core.h"
#include "Proxies/HTTPProxy.h"
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

// Forward declarations
struct JSONRPCBatch;
enum class HTTPMethod;
enum class HTTPStatusCode;

// Base class for Streamable HTTP transport implementations
// Handles all HTTP-related operations, serialization, and transport mechanics
class StreamableHTTPBase : public ITransport {
  public:
    virtual ~StreamableHTTPBase() = default;

    // === Core Transport Interface (ITransport) ===
    MCPTask_Void Connect() override;
    MCPTask_Void Disconnect() override;
    MCPTask_Void SendMessage(const MessageBase& InMessage) override;
    MCPTask_Void SendBatch(const JSONRPCBatch& InBatch) override;
    void SetMessageHandler(std::function<void(const MessageBase&)> InHandler) override;
    void SetErrorHandler(std::function<void(const std::string&)> InHandler) override;
    bool IsConnected() const override;
    TransportType GetTransportType() const override;

    [[deprecated("Not yet implemented - will be supported in a future version")]]
    bool Resume(const string& InResumptionToken) override;

    std::future<void> Start() override;
    std::future<void> Close() override;
    std::future<void> Send(const MessageBase& InMessage,
                           const TransportSendOptions& InOptions = {}) override;

  protected:
    // === HTTP Operations (All HTTP mechanics) ===
    virtual MCPTask<Poco::Net::HTTPRequest> CreateHTTPRequest(HTTPMethod InMethod,
                                                              const string& InEndpoint);
    virtual MCPTask_Void SendHTTPRequest(const Poco::Net::HTTPRequest& InRequest,
                                         const optional<MessageBase>& InMessage);
    virtual MCPTask_Void HandleHTTPResponse(const Poco::Net::HTTPResponse& InResponse);
    virtual MCPTask_Void ProcessIncomingHTTPRequest(const Poco::Net::HTTPRequest& InRequest,
                                                    Poco::Net::HTTPResponse& OutResponse);

    // === Message Serialization/Deserialization ===
    virtual string SerializeMessageToHTTP(const MessageBase& InMessage);
    virtual MessageBase DeserializeHTTPToMessage(const string& InHTTPBody);
    virtual string DetermineMessageType(const MessageBase& InMessage);

    // === Header Management (All header operations) ===
    virtual void AddRequiredHeaders(Poco::Net::HTTPRequest& InOutRequest);
    virtual void AddSessionHeader(Poco::Net::HTTPRequest& InOutRequest);
    virtual void AddResumabilityHeaders(Poco::Net::HTTPRequest& InOutRequest);
    virtual map<string, string> ExtractHeadersFromRequest(const Poco::Net::HTTPRequest& InRequest);
    virtual bool ValidateRequiredHeaders(const Poco::Net::HTTPRequest& InRequest);

    // === Session Management (All session operations) ===
    virtual MCPTask_Void CreateSession(const string& InSessionID);
    virtual void InvalidateSession();
    virtual void UpdateSessionActivity();
    virtual bool HasValidSession() const;
    virtual optional<string> GetSessionID() const;
    virtual bool ValidateSessionFromRequest(const Poco::Net::HTTPRequest& InRequest);
    virtual bool IsStatefulMode() const;

    // === SSE Stream Management (All SSE operations) ===
    virtual string FormatSSEEvent(const MessageBase& InMessage, const optional<string>& InEventID);
    virtual MessageBase ParseSSEEvent(const string& InSSEData);
    virtual MCPTask_Void StartSSEStream(Poco::Net::HTTPResponse& InOutResponse);
    virtual MCPTask_Void WriteMessageToSSEStream(Poco::Net::HTTPResponse& InOutResponse,
                                                 const MessageBase& InMessage);
    virtual MCPTask_Void ProcessSSEStreamData(const string& InStreamData);

    // === Error Handling (Generic HTTP error handling) ===
    virtual void HandleHTTPError(const Poco::Net::HTTPResponse& InResponse);
    virtual void HandleTransportError(const string& InErrorMessage);
    virtual void CallErrorHandler(const string& InError);
    virtual void CallMessageHandler(const MessageBase& InMessage);

    // === Resumability Support (EventStore integration) ===
    virtual void SetEventStore(shared_ptr<EventStore> InEventStore);
    virtual MCPTask<string> StoreEventForResumability(const MessageBase& InMessage);
    virtual MCPTask_Void ReplayEventsAfter(const string& InLastEventID);

    // === HTTP Method Handling (Generic method support) ===
    virtual MCPTask_Void HandlePOSTRequest(const Poco::Net::HTTPRequest& InRequest,
                                           Poco::Net::HTTPResponse& OutResponse);
    virtual MCPTask_Void HandleGETRequest(const Poco::Net::HTTPRequest& InRequest,
                                          Poco::Net::HTTPResponse& OutResponse);
    virtual MCPTask_Void HandleDELETERequest(const Poco::Net::HTTPRequest& InRequest,
                                             Poco::Net::HTTPResponse& OutResponse);
    virtual MCPTask_Void HandleUnsupportedMethod(Poco::Net::HTTPResponse& OutResponse);

    // === HTTP Status Code Management ===
    virtual void SetHTTPStatus(Poco::Net::HTTPResponse& InOutResponse, HTTPStatusCode InStatusCode,
                               const string& InMessage);
    virtual MCPTask_Void HandleStatusCode(HTTPStatusCode InStatusCode);

    // === Reconnection Support (Client-side) ===
    virtual bool ShouldAttemptReconnection();
    virtual std::chrono::milliseconds CalculateReconnectionDelay(int InAttempts);
    virtual MCPTask_Void AttemptReconnection();

    // === CORS and Security ===
    virtual void AddCORSHeaders(Poco::Net::HTTPResponse& InOutResponse,
                                const Poco::Net::HTTPRequest& InRequest);
    virtual bool ValidateOrigin(const string& InOrigin);
    virtual bool ValidateRequestSecurity(const Poco::Net::HTTPRequest& InRequest);

    // === Utility Methods ===
    virtual string GenerateEventID();
    virtual string GenerateSessionID();
    virtual bool IsInitializationMessage(const MessageBase& InMessage);
    virtual MessageBase ConvertBatchToMessage(const JSONRPCBatch& InBatch);

  protected:
    // === Member Variables ===

    // Connection state
    bool m_IsConnected{false};
    bool m_IsStateful{false};

    // Session management
    optional<string> m_SessionID;
    std::chrono::steady_clock::time_point m_LastActivity;

    // HTTP configuration
    string m_Host;
    int m_Port{80};
    string m_BasePath{"/"};
    bool m_UseHTTPS{false};

    // Handlers
    std::function<void(const MessageBase&)> m_MessageHandler;
    std::function<void(const std::string&)> m_ErrorHandler;

    // Event store for resumability
    shared_ptr<EventStore> m_EventStore;

    // CORS configuration
    vector<string> m_AllowedOrigins;
    bool m_CORSEnabled{false};

    // Reconnection state
    int m_ReconnectionAttempts{0};
    std::chrono::milliseconds m_MaxReconnectionDelay{30000};

    // HTTP session management
    unique_ptr<Poco::Net::HTTPClientSession> m_HTTPSession;

    // Thread safety
    mutable std::mutex m_StateMutex;
    mutable std::mutex m_SessionMutex;
};

MCP_NAMESPACE_END