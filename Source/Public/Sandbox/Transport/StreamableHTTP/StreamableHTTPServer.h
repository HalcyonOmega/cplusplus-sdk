#pragma once

#include "Core.h"
#include "StreamableHTTPBase.h"

// Poco Net includes
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Thread.h>

MCP_NAMESPACE_BEGIN

// Server-specific configuration options
struct StreamableHTTPServerOptions {
    // Function that generates a session ID for the transport
    // Return empty optional to disable session management (stateless mode)
    optional<function<string()>> SessionIDGenerator;

    // Callback for session initialization events
    optional<function<void(const string&)>> OnSessionInitialized;

    // If true, server returns JSON responses instead of starting SSE streams
    // Default is false (SSE streams are preferred per MCP spec)
    bool EnableJSONResponse = false;

    // Event store for resumability support
    shared_ptr<EventStore> EventStore;

    // Maximum number of concurrent connections
    int MaxConnections = 100;

    // Thread pool size for handling requests
    int ThreadPoolSize = 16;
};

// Forward declarations
class MCPRequestHandler;
class MCPRequestHandlerFactory;

// Server implementation of Streamable HTTP transport
class StreamableHTTPServer : public StreamableHTTPBase {
  private:
    unique_ptr<Poco::Net::HTTPServer> m_HTTPServer;
    unique_ptr<Poco::Net::ServerSocket> m_ServerSocket;
    StreamableHTTPServerOptions m_ServerOptions;

    // Session management (stateful mode only)
    optional<function<string()>> m_SessionIDGenerator;

    // Response stream mapping for SSE connections
    unordered_map<string, shared_ptr<HTTP::Response>> m_StreamMapping;
    map<string, string> m_RequestToStreamMapping; // RequestID -> StreamID

    bool m_Started = false;

  public:
    explicit StreamableHTTPServer(const HTTPTransportConfig& InConfig,
                                  const StreamableHTTPServerOptions& InOptions = {})
        : StreamableHTTPBase(InConfig), m_ServerOptions(InOptions),
          m_SessionIDGenerator(InOptions.SessionIDGenerator) {
        SetEventStore(InOptions.EventStore);
    }

    ~StreamableHTTPServer() override {
        Disconnect().wait();
    }

    // ITransport interface implementation
    MCPTask_Void Connect() override {
        try {
            // Create server socket
            m_ServerSocket = make_unique<Poco::Net::ServerSocket>(m_Config.Port);

            // Create HTTP server with custom request handler factory
            auto HandlerFactory = make_unique<MCPRequestHandlerFactory>(this);
            m_HTTPServer = make_unique<Poco::Net::HTTPServer>(
                HandlerFactory.release(), *m_ServerSocket, new Poco::Net::HTTPServerParams());

            // Configure server parameters
            auto ServerParams = m_HTTPServer->params();
            ServerParams->setMaxThreads(m_ServerOptions.ThreadPoolSize);
            ServerParams->setMaxQueued(m_ServerOptions.MaxConnections);

            // Start the server
            m_HTTPServer->start();
            m_IsConnected = true;
            m_Started = true;

        } catch (const Poco::Exception& Ex) {
            CallErrorHandler("Failed to start HTTP server: " + Ex.displayText());
            throw runtime_error("HTTP server start failed: " + Ex.displayText());
        }

        co_return;
    }

    MCPTask_Void Disconnect() override {
        try {
            if (m_HTTPServer) {
                m_HTTPServer->stop();
                m_HTTPServer.reset();
            }

            if (m_ServerSocket) {
                m_ServerSocket->close();
                m_ServerSocket.reset();
            }

            // Close all active streams
            for (auto& [StreamID, Response] : m_StreamMapping) {
                if (Response) { Response->End(); }
            }
            m_StreamMapping.clear();
            m_RequestToStreamMapping.clear();

            m_IsConnected = false;
            m_Started = false;

        } catch (const Poco::Exception& Ex) {
            CallErrorHandler("Error during server shutdown: " + Ex.displayText());
        }

        co_return;
    }

    MCPTask_Void SendMessage(const MessageBase& InMessage) override {
        // For server, this sends a message to all connected clients via their SSE streams
        string MessageData = SerializeToJSON(InMessage);
        string EventID = GenerateEventID();

        for (auto& [StreamID, Response] : m_StreamMapping) {
            if (Response) { WriteSSEEventToResponse(Response, InMessage, EventID); }
        }

        co_return;
    }

    MCPTask_Void SendBatch(const JSONRPCBatch& InBatch) override {
        MessageBase BatchMessage = ConvertBatchToMessage(InBatch);
        co_await SendMessage(BatchMessage);
        co_return;
    }

    // Starts the server - required by Transport interface but is a no-op
    // since connections are managed per-request
    future<void> Start() override {
        return Connect();
    }

    future<void> Close() override {
        return Disconnect();
    }

    // Server-specific methods for handling HTTP requests
    MCPTask_Void HandleRequest(const HTTP::Request& InRequest,
                               shared_ptr<HTTP::Response> InResponse,
                               const optional<string>& InBody = nullopt) {
        try {
            HTTP::Method Method = InRequest.GetMethod();

            switch (Method) {
                case HTTP::Method::Post:
                    co_await HandlePostRequest(InRequest, InResponse, InBody);
                    break;

                case HTTP::Method::Get: co_await HandleGetRequest(InRequest, InResponse); break;

                case HTTP::Method::Delete:
                    co_await HandleDeleteRequest(InRequest, InResponse);
                    break;

                default: co_await HandleUnsupportedRequest(InResponse); break;
            }

        } catch (const exception& Ex) {
            CallErrorHandler("Error handling request: " + string(Ex.what()));
            InResponse->SetStatus(HTTP::Status::InternalServerError);
            InResponse->End();
        }

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
    // Handles POST requests containing JSON-RPC messages
    MCPTask_Void HandlePostRequest(const HTTP::Request& InRequest,
                                   shared_ptr<HTTP::Response> InResponse,
                                   const optional<string>& InBody) {
        // Validate session if in stateful mode
        if (IsStatefulMode() && !IsInitializationRequest(InBody.value_or(""))) {
            if (!ValidateSession(InRequest, InResponse)) { co_return; }
        }

        // Parse the JSON-RPC message(s)
        if (!InBody.has_value() || InBody->empty()) {
            InResponse->SetStatus(HTTP::Status::BadRequest);
            InResponse->End("Missing request body");
            co_return;
        }

        try {
            // Determine if this contains requests, notifications, or responses
            MessageType MsgType = DetermineMessageType(InBody.value());

            if (MsgType == MessageType::NotificationOrResponse) {
                // HTTP 202 Accepted for notifications/responses
                InResponse->SetStatus(HTTP::Status::Accepted);
                InResponse->End();

                // Process the message
                MessageBase Message = DeserializeFromJSON(InBody.value());
                CallMessageHandler(Message);

            } else if (MsgType == MessageType::Request) {
                // Contains requests - either return JSON or start SSE stream
                if (m_ServerOptions.EnableJSONResponse) {
                    co_await HandleJSONResponse(InRequest, InResponse, InBody.value());
                } else {
                    co_await HandleSSEResponse(InRequest, InResponse, InBody.value());
                }

            } else {
                InResponse->SetStatus(HTTP::Status::BadRequest);
                InResponse->End("Invalid message format");
            }

        } catch (const exception& Ex) {
            CallErrorHandler("Failed to process POST request: " + string(Ex.what()));
            InResponse->SetStatus(HTTP::Status::BadRequest);
            InResponse->End();
        }

        co_return;
    }

    // Handles GET requests for SSE streams
    MCPTask_Void HandleGetRequest(const HTTP::Request& InRequest,
                                  shared_ptr<HTTP::Response> InResponse) {
        // Validate session if in stateful mode
        if (IsStatefulMode()) {
            if (!ValidateSession(InRequest, InResponse)) { co_return; }
        }

        // Check if client accepts SSE
        string AcceptHeader = InRequest.GetHeaders().Get("Accept");
        if (AcceptHeader.find("text/event-stream") == string::npos) {
            InResponse->SetStatus(HTTP::Status::NotAcceptable);
            InResponse->End();
            co_return;
        }

        // Set up SSE stream
        InResponse->GetHeaders().Set("Content-Type", "text/event-stream");
        InResponse->GetHeaders().Set("Cache-Control", "no-cache");
        InResponse->GetHeaders().Set("Connection", "keep-alive");
        InResponse->SetStatus(HTTP::Status::Ok);

        // Add CORS headers if needed
        if (m_Config.ValidateOrigin) { AddCORSHeaders(InRequest, InResponse); }

        // Handle resumability
        string LastEventID = InRequest.GetHeaders().Get("Last-Event-ID");
        if (!LastEventID.empty() && m_EventStore) {
            co_await ReplayEvents(LastEventID, InResponse);
        }

        // Register the stream for future messages
        string StreamID = GenerateStreamID();
        m_StreamMapping[StreamID] = InResponse;

        // Keep the connection alive - the response will be ended when the client disconnects
        // or when the server shuts down

        co_return;
    }

    // Handles DELETE requests to terminate sessions
    MCPTask_Void HandleDeleteRequest(const HTTP::Request& InRequest,
                                     shared_ptr<HTTP::Response> InResponse) {
        if (!IsStatefulMode()) {
            // Session termination not supported in stateless mode
            InResponse->SetStatus(HTTP::Status::MethodNotAllowed);
            InResponse->End();
            co_return;
        }

        // Validate session
        if (!ValidateSession(InRequest, InResponse)) { co_return; }

        // Terminate the session
        InvalidateSession();

        // Remove associated streams
        string SessionID = GetSessionID().value_or("");
        for (auto It = m_StreamMapping.begin(); It != m_StreamMapping.end();) {
            // In a real implementation, we'd track which streams belong to which session
            // For now, we'll just remove all streams (simplified)
            It->second->End();
            It = m_StreamMapping.erase(It);
        }

        InResponse->SetStatus(HTTP::Status::Ok);
        InResponse->End();

        co_return;
    }

    // Handles unsupported HTTP methods
    MCPTask_Void HandleUnsupportedRequest(shared_ptr<HTTP::Response> InResponse) {
        InResponse->SetStatus(HTTP::Status::MethodNotAllowed);
        InResponse->GetHeaders().Set("Allow", "POST, GET, DELETE");
        InResponse->End();
        co_return;
    }

    // Session validation
    bool ValidateSession(const HTTP::Request& InRequest, shared_ptr<HTTP::Response> InResponse) {
        if (!InRequest.GetHeaders().Has("Mcp-Session-Id")) {
            InResponse->SetStatus(HTTP::Status::BadRequest);
            InResponse->End("Missing session ID");
            return false;
        }

        string SessionID = InRequest.GetHeaders().Get("Mcp-Session-Id");
        if (!HasValidSession() || GetSessionID().value_or("") != SessionID) {
            InResponse->SetStatus(HTTP::Status::NotFound);
            InResponse->End("Invalid session");
            return false;
        }

        UpdateSessionActivity();
        return true;
    }

    // JSON response handling
    MCPTask_Void HandleJSONResponse(const HTTP::Request& InRequest,
                                    shared_ptr<HTTP::Response> InResponse,
                                    const string& InRequestBody) {
        // Process the request and generate response
        MessageBase Request = DeserializeFromJSON(InRequestBody);
        MessageBase Response = ProcessRequest(Request);

        string ResponseJSON = SerializeToJSON(Response);

        InResponse->SetStatus(HTTP::Status::Ok);
        InResponse->GetHeaders().Set("Content-Type", "application/json");
        InResponse->End(ResponseJSON);

        co_return;
    }

    // SSE response handling
    MCPTask_Void HandleSSEResponse(const HTTP::Request& InRequest,
                                   shared_ptr<HTTP::Response> InResponse,
                                   const string& InRequestBody) {
        // Set up SSE stream
        InResponse->GetHeaders().Set("Content-Type", "text/event-stream");
        InResponse->GetHeaders().Set("Cache-Control", "no-cache");
        InResponse->GetHeaders().Set("Connection", "keep-alive");
        InResponse->SetStatus(HTTP::Status::Ok);

        // Add session header if in stateful mode and this is initialization
        if (IsStatefulMode() && IsInitializationRequest(InRequestBody)) {
            string SessionID = GenerateSessionID();
            CreateSession(SessionID);
            InResponse->GetHeaders().Set("Mcp-Session-Id", SessionID);

            // Notify callback if provided
            if (m_ServerOptions.OnSessionInitialized.has_value()) {
                m_ServerOptions.OnSessionInitialized.value()(SessionID);
            }
        }

        // Process the request
        MessageBase Request = DeserializeFromJSON(InRequestBody);

        // Send related messages first (if any)
        // This would involve processing the request and sending any intermediate messages

        // Finally send the response
        MessageBase Response = ProcessRequest(Request);
        string EventID = GenerateEventID();
        WriteSSEEventToResponse(InResponse, Response, EventID);

        // End the stream after sending the response
        InResponse->End();

        co_return;
    }

    // Event and stream management
    bool WriteSSEEventToResponse(shared_ptr<HTTP::Response> InResponse,
                                 const MessageBase& InMessage,
                                 const optional<string>& InEventID = nullopt) {
        if (!InResponse) return false;

        try {
            string MessageData = SerializeToJSON(InMessage);
            string SSEEvent = FormatSSEEvent("", MessageData, InEventID);

            InResponse->Write(SSEEvent);
            return true;

        } catch (const exception& Ex) {
            CallErrorHandler("Failed to write SSE event: " + string(Ex.what()));
            return false;
        }
    }

    MCPTask_Void ReplayEvents(const string& InLastEventID, shared_ptr<HTTP::Response> InResponse) {
        if (!m_EventStore) { co_return; }

        try {
            // Use EventStore to replay events after the specified ID
            string StreamID = GetCurrentStreamID(); // This would be implementation-specific
            co_await m_EventStore->ReplayEventsAfter(
                InLastEventID,
                [this, InResponse](const EventID& InEventID,
                                   const MessageBase& InMessage) -> future<void> {
                    WriteSSEEventToResponse(InResponse, InMessage, InEventID);
                    return make_ready_future();
                });

        } catch (const exception& Ex) {
            CallErrorHandler("Failed to replay events: " + string(Ex.what()));
        }

        co_return;
    }

    // Helper methods
    bool IsStatefulMode() const {
        return m_SessionIDGenerator.has_value();
    }

    bool IsInitializationRequest(const string& InRequestBody) {
        // TODO: Parse JSON and check if this is an InitializeRequest
        return InRequestBody.find("initialize") != string::npos; // Simplified check
    }

    enum class MessageType { Request, NotificationOrResponse, Invalid };

    MessageType DetermineMessageType(const string& InJSON) {
        // TODO: Parse JSON and determine message type
        // For now, simplified logic
        if (InJSON.find("\"method\"") != string::npos) {
            return MessageType::Request;
        } else if (InJSON.find("\"result\"") != string::npos
                   || InJSON.find("\"error\"") != string::npos) {
            return MessageType::NotificationOrResponse;
        }
        return MessageType::Invalid;
    }

    string GenerateSessionID() {
        if (m_SessionIDGenerator.has_value()) { return m_SessionIDGenerator.value()(); }
        return "";
    }

    string GenerateEventID() {
        // TODO: Generate unique event ID
        static atomic<int> Counter{0};
        return "event_" + to_string(Counter++);
    }

    string GenerateStreamID() {
        // TODO: Generate unique stream ID
        static atomic<int> Counter{0};
        return "stream_" + to_string(Counter++);
    }

    string GetCurrentStreamID() {
        // TODO: Get current stream ID for the context
        return "default_stream";
    }

    void AddCORSHeaders(const HTTP::Request& InRequest, shared_ptr<HTTP::Response> InResponse) {
        string Origin = InRequest.GetHeaders().Get("Origin");
        if (!Origin.empty() && IsAllowedOrigin(Origin)) {
            InResponse->GetHeaders().Set("Access-Control-Allow-Origin", Origin);
            InResponse->GetHeaders().Set("Access-Control-Allow-Credentials", "true");
        }
    }

    bool IsAllowedOrigin(const string& InOrigin) {
        if (m_Config.AllowedOrigins.empty()) {
            return true; // Allow all if no restrictions
        }

        return find(m_Config.AllowedOrigins.begin(), m_Config.AllowedOrigins.end(), InOrigin)
               != m_Config.AllowedOrigins.end();
    }

    MessageBase ProcessRequest(const MessageBase& InRequest) {
        // TODO: Process the request and generate appropriate response
        // This would involve calling the actual MCP request handlers
        MessageBase Response;
        return Response;
    }

    MessageBase ConvertBatchToMessage(const JSONRPCBatch& InBatch) {
        // TODO: Convert batch to proper message format
        MessageBase Message;
        return Message;
    }

    // Friend class for request handler factory
    friend class MCPRequestHandlerFactory;
    friend class MCPRequestHandler;
};

// Request handler for individual HTTP requests
class MCPRequestHandler : public Poco::Net::HTTPRequestHandler {
  private:
    StreamableHTTPServer* m_Server;

  public:
    explicit MCPRequestHandler(StreamableHTTPServer* InServer) : m_Server(InServer) {}

    void handleRequest(Poco::Net::HTTPServerRequest& InRequest,
                       Poco::Net::HTTPServerResponse& InResponse) override {
        try {
            // Convert Poco request/response to our HTTP types
            HTTP::Request Request = ConvertPocoRequest(InRequest);
            auto Response = make_shared<HTTP::Response>();

            // Read request body
            optional<string> Body;
            if (InRequest.getMethod() == Poco::Net::HTTPRequest::HTTP_POST) {
                istream& RequestStream = InRequest.stream();
                stringstream BodyStream;
                Poco::StreamCopier::copyStream(RequestStream, BodyStream);
                Body = BodyStream.str();
            }

            // Handle the request
            m_Server->HandleRequest(Request, Response, Body).wait();

            // Convert response back to Poco
            ConvertToPocoResponse(*Response, InResponse);

        } catch (const exception& Ex) {
            InResponse.setStatusAndReason(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
            ostream& ResponseStream = InResponse.send();
            ResponseStream << "Internal server error: " << Ex.what();
        }
    }

  private:
    HTTP::Request ConvertPocoRequest(const Poco::Net::HTTPServerRequest& InPocoRequest) {
        HTTP::Request Request;

        // Convert method
        string MethodStr = InPocoRequest.getMethod();
        if (MethodStr == "GET")
            Request.SetMethod(HTTP::Method::Get);
        else if (MethodStr == "POST")
            Request.SetMethod(HTTP::Method::Post);
        else if (MethodStr == "DELETE")
            Request.SetMethod(HTTP::Method::Delete);
        // Add other methods as needed

        Request.SetURI(InPocoRequest.getURI());

        // Convert headers
        for (const auto& Header : InPocoRequest) {
            Request.GetHeaders().Set(Header.first, Header.second);
        }

        return Request;
    }

    void ConvertToPocoResponse(const HTTP::Response& InResponse,
                               Poco::Net::HTTPServerResponse& OutPocoResponse) {
        // Convert status
        OutPocoResponse.setStatus(static_cast<Poco::Net::HTTPResponse::HTTPStatus>(
            static_cast<int>(InResponse.GetStatus())));

        // Convert headers
        // Note: This would need to access the headers from InResponse
        // The HTTP::Response class would need to provide iteration over headers

        // Send body
        string Body = InResponse.GetBody();
        if (!Body.empty()) {
            ostream& ResponseStream = OutPocoResponse.send();
            ResponseStream << Body;
        } else {
            OutPocoResponse.send();
        }
    }
};

// Request handler factory
class MCPRequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory {
  private:
    StreamableHTTPServer* m_Server;

  public:
    explicit MCPRequestHandlerFactory(StreamableHTTPServer* InServer) : m_Server(InServer) {}

    Poco::Net::HTTPRequestHandler*
    createRequestHandler(const Poco::Net::HTTPServerRequest& InRequest) override {
        return new MCPRequestHandler(m_Server);
    }
};

MCP_NAMESPACE_END