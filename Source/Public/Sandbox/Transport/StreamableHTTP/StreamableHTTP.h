#pragma once

#include "../TransportPseudoCode.h"
#include "Core.h"

// Poco Net includes
#include <Poco/Event.h>
#include <Poco/Exception.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/NetException.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Runnable.h>
#include <Poco/StreamCopier.h>
#include <Poco/Thread.h>
#include <Poco/URI.h>

MCP_NAMESPACE_BEGIN

// SSE Event structure for Server-Sent Events
struct SSEEvent {
    std::string ID;
    std::string Type;
    std::string Data;
    std::optional<int> Retry;

    SSEEvent() = default;
    SSEEvent(const std::string& InData) : Data(InData) {}
};

// SSE Stream wrapper for Poco streams
class SSEStream {
  private:
    std::unique_ptr<std::istream> m_Stream;
    bool m_IsOpen;

  public:
    SSEStream(std::unique_ptr<std::istream> InStream)
        : m_Stream(std::move(InStream)), m_IsOpen(true) {}

    ~SSEStream() {
        Close();
    }

    bool IsOpen() const {
        return m_IsOpen && m_Stream && m_Stream->good();
    }

    void Close() {
        m_IsOpen = false;
        m_Stream.reset();
    }

    MCPTask_Void ReadEvent() {
        SSEEvent Event;
        std::string Line;

        while (IsOpen() && std::getline(*m_Stream, Line)) {
            if (Line.empty()) {
                // Empty line signals end of event
                co_return Event;
            }

            if (Line.starts_with("id:")) {
                Event.ID = Line.substr(3);
            } else if (Line.starts_with("event:")) {
                Event.Type = Line.substr(6);
            } else if (Line.starts_with("data:")) {
                if (!Event.Data.empty()) Event.Data += "\n";
                Event.Data += Line.substr(5);
            } else if (Line.starts_with("retry:")) {
                Event.Retry = std::stoi(Line.substr(6));
            }
        }

        co_return Event;
    }
};

// Session management for HTTP transport
struct MCPSession {
    std::string SessionID;
    std::chrono::system_clock::time_point CreatedAt;
    std::chrono::system_clock::time_point LastActivity;
    bool IsActive;

    MCPSession(const std::string& InSessionID)
        : SessionID(InSessionID), CreatedAt(std::chrono::system_clock::now()),
          LastActivity(std::chrono::system_clock::now()), IsActive(true) {}
};

// HTTP Transport Configuration
struct HTTPTransportConfig {
    std::string Host = "localhost";
    int Port = 8080;
    std::string MCPEndpoint = "/mcp";
    bool UseSSL = false;
    std::chrono::seconds RequestTimeout = std::chrono::seconds(30);
    bool ValidateOrigin = true;
    std::vector<std::string> AllowedOrigins;
};

// Streamable HTTP Transport Implementation using Poco::Net
class StreamableHTTPTransport : public ITransport {
  private:
    HTTPTransportConfig m_Config;
    std::optional<MCPSession> m_Session;
    std::unique_ptr<Poco::Net::HTTPClientSession> m_HTTPSession;
    std::unique_ptr<SSEStream> m_SSEStream;
    std::function<void(const MessageBase&)> m_MessageHandler;
    std::function<void(const std::string&)> m_ErrorHandler;
    bool m_IsConnected;
    std::string m_LastEventID;

  public:
    StreamableHTTPTransport(const HTTPTransportConfig& InConfig)
        : m_Config(InConfig), m_IsConnected(false) {}

    MCPTask_Void Connect() override {
        try {
            // Create HTTP session using Poco
            if (m_Config.UseSSL) {
                m_HTTPSession =
                    std::make_unique<Poco::Net::HTTPSClientSession>(m_Config.Host, m_Config.Port);
            } else {
                m_HTTPSession =
                    std::make_unique<Poco::Net::HTTPClientSession>(m_Config.Host, m_Config.Port);
            }

            m_HTTPSession->setTimeout(Poco::Timespan(m_Config.RequestTimeout.count(), 0));

            // Send InitializeRequest to establish session
            JSONRPCRequest InitRequest = CreateInitializeRequest();
            co_await SendInitializeRequest(InitRequest);

            m_IsConnected = true;
        } catch (const Poco::Exception& Ex) {
            if (m_ErrorHandler) {
                m_ErrorHandler("Failed to connect HTTP transport: " + Ex.displayText());
            }
            throw std::runtime_error("HTTP connection failed: " + Ex.displayText());
        }

        co_return;
    }

    MCPTask_Void Disconnect() override {
        try {
            // Send session termination if session exists
            if (m_Session.has_value()) { co_await SendSessionTermination(); }

            // Close SSE stream if open
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
            if (m_ErrorHandler) { m_ErrorHandler("Error during disconnect: " + Ex.displayText()); }
        }

        co_return;
    }

    MCPTask_Void SendMessage(const MessageBase& InMessage) override {
        if (!m_IsConnected || !m_HTTPSession) {
            throw std::runtime_error("Transport not connected");
        }

        try {
            Poco::Net::HTTPRequest Request = CreatePOSTRequest(InMessage);
            AddSessionHeader(Request);
            AddAcceptHeaders(Request);

            Poco::Net::HTTPResponse Response;
            co_await SendHTTPRequest(Request, Response, InMessage);
            co_await ProcessHTTPResponse(Response, InMessage);
        } catch (const Poco::Exception& Ex) {
            if (m_ErrorHandler) { m_ErrorHandler("HTTP request failed: " + Ex.displayText()); }
            throw std::runtime_error("HTTP request failed: " + Ex.displayText());
        }

        co_return;
    }

    MCPTask_Void SendBatch(const JSONRPCBatch& InBatch) override {
        MessageBase BatchMessage = ConvertBatchToMessage(InBatch);
        co_await SendMessage(BatchMessage);
        co_return;
    }

    MCPTask_Void ListenForServerMessages() {
        if (!m_IsConnected || !m_HTTPSession) {
            throw std::runtime_error("Transport not connected");
        }

        try {
            Poco::Net::HTTPRequest GetRequest = CreateGETRequest();
            AddSessionHeader(GetRequest);
            AddSSEAcceptHeader(GetRequest);
            AddLastEventIDHeader(GetRequest);

            Poco::Net::HTTPResponse Response;
            std::istream& ResponseStream = m_HTTPSession->receiveResponse(Response);

            if (Response.getStatus() == Poco::Net::HTTPResponse::HTTP_OK
                && Response.getContentType() == "text/event-stream") {
                m_SSEStream = std::make_unique<SSEStream>(
                    std::make_unique<std::istream>(ResponseStream.rdbuf()));

                while (m_SSEStream && m_SSEStream->IsOpen()) {
                    SSEEvent Event = co_await m_SSEStream->ReadEvent();
                    ProcessSSEEvent(Event);
                }
            }
        } catch (const Poco::Exception& Ex) {
            if (m_ErrorHandler) { m_ErrorHandler("SSE stream error: " + Ex.displayText()); }
        }

        co_return;
    }

    void SetMessageHandler(std::function<void(const MessageBase&)> InHandler) override {
        m_MessageHandler = InHandler;
    }

    void SetErrorHandler(std::function<void(const std::string&)> InHandler) override {
        m_ErrorHandler = InHandler;
    }

    bool IsConnected() const override {
        return m_IsConnected;
    }
    std::string GetTransportType() const override {
        return "streamable-http";
    }

  private:
    MCPTask_Void SendInitializeRequest(const JSONRPCRequest& InRequest) {
        Poco::Net::HTTPRequest Request = CreatePOSTRequest(InRequest);
        AddAcceptHeaders(Request);

        Poco::Net::HTTPResponse Response;
        co_await SendHTTPRequest(Request, Response, InRequest);

        // Extract session ID from response headers
        if (Response.has("Mcp-Session-Id")) {
            std::string SessionID = Response.get("Mcp-Session-Id");
            m_Session = MCPSession(SessionID);
        }

        co_await ProcessHTTPResponse(Response, InRequest);
        co_return;
    }

    MCPTask_Void SendSessionTermination() {
        if (!m_Session.has_value() || !m_HTTPSession) { co_return; }

        try {
            Poco::Net::HTTPRequest DeleteRequest(Poco::Net::HTTPRequest::HTTP_DELETE,
                                                 m_Config.MCPEndpoint,
                                                 Poco::Net::HTTPMessage::HTTP_1_1);
            DeleteRequest.set("Mcp-Session-Id", m_Session->SessionID);

            m_HTTPSession->sendRequest(DeleteRequest);

            Poco::Net::HTTPResponse Response;
            m_HTTPSession->receiveResponse(Response);

            // Server may respond with 405 if termination not supported
            if (Response.getStatus() != Poco::Net::HTTPResponse::HTTP_METHOD_NOT_ALLOWED) {
                m_Session.reset();
            }
        } catch (const Poco::Exception& Ex) {
            if (m_ErrorHandler) {
                m_ErrorHandler("Session termination failed: " + Ex.displayText());
            }
        }

        co_return;
    }

    MCPTask_Void SendHTTPRequest(Poco::Net::HTTPRequest& InRequest,
                                 Poco::Net::HTTPResponse& OutResponse,
                                 const MessageBase& InMessage) {
        std::string JsonData = SerializeToJSON(InMessage);
        InRequest.setContentLength(JsonData.length());

        std::ostream& RequestStream = m_HTTPSession->sendRequest(InRequest);
        RequestStream << JsonData;

        std::istream& ResponseStream = m_HTTPSession->receiveResponse(OutResponse);

        co_return;
    }

    MCPTask_Void ProcessHTTPResponse(const Poco::Net::HTTPResponse& InResponse,
                                     const MessageBase& InOriginalMessage) {
        if (InResponse.getStatus() == Poco::Net::HTTPResponse::HTTP_ACCEPTED) {
            // Accepted - for notifications/responses only
            co_return;
        }

        if (InResponse.getStatus() == Poco::Net::HTTPResponse::HTTP_NOT_FOUND
            && m_Session.has_value()) {
            // Session expired - need to reinitialize
            m_Session.reset();
            co_await Connect();
            co_return;
        }

        if (InResponse.getStatus() >= Poco::Net::HTTPResponse::HTTP_BAD_REQUEST) {
            HandleHTTPError(InResponse);
            co_return;
        }

        std::string ContentType = InResponse.getContentType();

        if (ContentType == "application/json") {
            // Single JSON response
            std::istream& ResponseStream =
                m_HTTPSession->receiveResponse(const_cast<Poco::Net::HTTPResponse&>(InResponse));

            std::string ResponseBody;
            Poco::StreamCopier::copyToString(ResponseStream, ResponseBody);

            MessageBase Response = DeserializeFromJSON(ResponseBody);
            if (m_MessageHandler) { m_MessageHandler(Response); }
        } else if (ContentType == "text/event-stream") {
            // SSE stream initiated - will be handled by ListenForServerMessages
        }

        co_return;
    }

    void ProcessSSEEvent(const SSEEvent& InEvent) {
        if (!InEvent.Data.empty()) {
            try {
                // Update last event ID for resumability
                if (!InEvent.ID.empty()) { m_LastEventID = InEvent.ID; }

                MessageBase Message = DeserializeFromJSON(InEvent.Data);
                if (m_MessageHandler) { m_MessageHandler(Message); }
            } catch (const std::exception& Ex) {
                if (m_ErrorHandler) {
                    m_ErrorHandler("Failed to parse SSE message: " + std::string(Ex.what()));
                }
            }
        }
    }

    Poco::Net::HTTPRequest CreatePOSTRequest(const MessageBase& InMessage) {
        Poco::Net::HTTPRequest Request(Poco::Net::HTTPRequest::HTTP_POST, m_Config.MCPEndpoint,
                                       Poco::Net::HTTPMessage::HTTP_1_1);
        Request.setContentType("application/json");
        return Request;
    }

    Poco::Net::HTTPRequest CreateGETRequest() {
        Poco::Net::HTTPRequest Request(Poco::Net::HTTPRequest::HTTP_GET, m_Config.MCPEndpoint,
                                       Poco::Net::HTTPMessage::HTTP_1_1);
        return Request;
    }

    void AddSessionHeader(Poco::Net::HTTPRequest& InRequest) {
        if (m_Session.has_value()) { InRequest.set("Mcp-Session-Id", m_Session->SessionID); }
    }

    void AddAcceptHeaders(Poco::Net::HTTPRequest& InRequest) {
        InRequest.set("Accept", "application/json, text/event-stream");
    }

    void AddSSEAcceptHeader(Poco::Net::HTTPRequest& InRequest) {
        InRequest.set("Accept", "text/event-stream");
    }

    void AddLastEventIDHeader(Poco::Net::HTTPRequest& InRequest) {
        if (!m_LastEventID.empty()) { InRequest.set("Last-Event-ID", m_LastEventID); }
    }

    void HandleHTTPError(const Poco::Net::HTTPResponse& InResponse) {
        if (m_ErrorHandler) {
            std::string ErrorMsg = "HTTP Error " + std::to_string(InResponse.getStatus()) + ": "
                                   + InResponse.getReason();
            m_ErrorHandler(ErrorMsg);
        }
    }
};

MCP_NAMESPACE_END