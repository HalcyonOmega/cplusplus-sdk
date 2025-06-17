#pragma once

#include <chrono>
#include <coroutine>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

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
#include <Poco/Pipe.h>
#include <Poco/PipeStream.h>
#include <Poco/Process.h>
#include <Poco/Runnable.h>
#include <Poco/StreamCopier.h>
#include <Poco/Thread.h>
#include <Poco/URI.h>

MCP_NAMESPACE_BEGIN

// Forward declarations
struct JSONRPCMessage;
struct JSONRPCRequest;
struct JSONRPCResponse;
struct JSONRPCNotification;
struct JSONRPCBatch;

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

    std::coroutine_handle<> ReadEventAsync() {
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

// Process wrapper for Poco::Process
class ProcessWrapper {
  private:
    Poco::Process::PID m_ProcessID;
    std::unique_ptr<Poco::Pipe> m_StdinPipe;
    std::unique_ptr<Poco::Pipe> m_StdoutPipe;
    std::unique_ptr<Poco::Pipe> m_StderrPipe;
    std::unique_ptr<Poco::PipeOutputStream> m_StdinStream;
    std::unique_ptr<Poco::PipeInputStream> m_StdoutStream;
    std::unique_ptr<Poco::PipeInputStream> m_StderrStream;
    bool m_IsRunning;

  public:
    ProcessWrapper(const std::string& InExecutable, const std::vector<std::string>& InArguments)
        : m_IsRunning(false) {
        m_StdinPipe = std::make_unique<Poco::Pipe>();
        m_StdoutPipe = std::make_unique<Poco::Pipe>();
        m_StderrPipe = std::make_unique<Poco::Pipe>();

        Poco::Process::Args Args(InArguments.begin(), InArguments.end());

        try {
            Poco::ProcessHandle Handle = Poco::Process::launch(
                InExecutable, Args, m_StdinPipe.get(), m_StdoutPipe.get(), m_StderrPipe.get());

            m_ProcessID = Handle.id();
            m_IsRunning = true;

            m_StdinStream = std::make_unique<Poco::PipeOutputStream>(*m_StdinPipe);
            m_StdoutStream = std::make_unique<Poco::PipeInputStream>(*m_StdoutPipe);
            m_StderrStream = std::make_unique<Poco::PipeInputStream>(*m_StderrPipe);

        } catch (const Poco::Exception& Ex) {
            throw std::runtime_error("Failed to launch process: " + Ex.displayText());
        }
    }

    ~ProcessWrapper() {
        if (m_IsRunning) {
            try {
                Poco::Process::kill(m_ProcessID);
            } catch (...) {
                // Ignore errors during cleanup
            }
        }
    }

    std::coroutine_handle<> WriteToStdin(const std::string& InData) {
        if (!m_IsRunning || !m_StdinStream) {
            throw std::runtime_error("Process not running or stdin not available");
        }

        try {
            *m_StdinStream << InData;
            m_StdinStream->flush();
        } catch (const Poco::Exception& Ex) {
            throw std::runtime_error("Failed to write to stdin: " + Ex.displayText());
        }

        co_return;
    }

    std::coroutine_handle<> ReadLineFromStdout() {
        if (!m_IsRunning || !m_StdoutStream) { co_return std::string{}; }

        std::string Line;
        try {
            std::getline(*m_StdoutStream, Line);
        } catch (const Poco::Exception&) {
            // Stream may be closed
        }

        co_return Line;
    }

    std::coroutine_handle<> ReadLineFromStderr() {
        if (!m_IsRunning || !m_StderrStream) { co_return std::string{}; }

        std::string Line;
        try {
            std::getline(*m_StderrStream, Line);
        } catch (const Poco::Exception&) {
            // Stream may be closed
        }

        co_return Line;
    }

    void CloseStdin() {
        if (m_StdinStream) {
            m_StdinStream->close();
            m_StdinStream.reset();
        }
    }

    std::coroutine_handle<> WaitForExit() {
        if (m_IsRunning) {
            try {
                int ExitCode = Poco::Process::wait(m_ProcessID);
                m_IsRunning = false;
                co_return ExitCode;
            } catch (const Poco::Exception& Ex) {
                throw std::runtime_error("Failed to wait for process: " + Ex.displayText());
            }
        }
        co_return 0;
    }

    bool IsRunning() const {
        return m_IsRunning;
    }
};

// Transport base interface
class ITransport {
  public:
    virtual ~ITransport() = default;

    // Core transport operations
    virtual std::coroutine_handle<> ConnectAsync() = 0;
    virtual std::coroutine_handle<> DisconnectAsync() = 0;
    virtual std::coroutine_handle<> SendMessageAsync(const JSONRPCMessage& InMessage) = 0;
    virtual std::coroutine_handle<> SendBatchAsync(const JSONRPCBatch& InBatch) = 0;

    // Message reception callbacks
    virtual void SetMessageHandler(std::function<void(const JSONRPCMessage&)> InHandler) = 0;
    virtual void SetErrorHandler(std::function<void(const std::string&)> InHandler) = 0;

    // State management
    virtual bool IsConnected() const = 0;
    virtual std::string GetTransportType() const = 0;
};

// Transport types enumeration
enum class TransportType { Stdio, StreamableHTTP, Custom };

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

// stdio Transport Implementation using Poco::Process
class StdioTransport : public ITransport {
  private:
    std::string m_ExecutablePath;
    std::vector<std::string> m_Arguments;
    std::unique_ptr<ProcessWrapper> m_ServerProcess;
    std::function<void(const JSONRPCMessage&)> m_MessageHandler;
    std::function<void(const std::string&)> m_ErrorHandler;
    bool m_IsConnected;

  public:
    StdioTransport(const std::string& InExecutablePath, const std::vector<std::string>& InArguments)
        : m_ExecutablePath(InExecutablePath), m_Arguments(InArguments), m_IsConnected(false) {}

    std::coroutine_handle<> ConnectAsync() override {
        try {
            // Launch subprocess using Poco::Process
            m_ServerProcess = std::make_unique<ProcessWrapper>(m_ExecutablePath, m_Arguments);

            // Start reading from stdout
            co_await StartReadingFromStdout();

            // Start reading from stderr for logging
            co_await StartReadingFromStderr();

            m_IsConnected = true;
        } catch (const std::exception& Ex) {
            if (m_ErrorHandler) {
                m_ErrorHandler("Failed to connect stdio transport: " + std::string(Ex.what()));
            }
            throw;
        }

        co_return;
    }

    std::coroutine_handle<> DisconnectAsync() override {
        if (m_ServerProcess) {
            // Close stdin to signal termination
            m_ServerProcess->CloseStdin();

            // Wait for process to terminate
            co_await m_ServerProcess->WaitForExit();

            m_ServerProcess.reset();
        }
        m_IsConnected = false;
        co_return;
    }

    std::coroutine_handle<> SendMessageAsync(const JSONRPCMessage& InMessage) override {
        if (!m_IsConnected || !m_ServerProcess) {
            throw std::runtime_error("Transport not connected");
        }

        // Serialize message to JSON
        std::string JsonData = SerializeToJSON(InMessage);

        // Ensure no embedded newlines (spec requirement)
        if (JsonData.find('\n') != std::string::npos) {
            throw std::runtime_error("Message contains embedded newlines");
        }

        // Write to stdin with newline delimiter
        co_await m_ServerProcess->WriteToStdin(JsonData + "\n");
        co_return;
    }

    std::coroutine_handle<> SendBatchAsync(const JSONRPCBatch& InBatch) override {
        JSONRPCMessage BatchMessage = ConvertBatchToMessage(InBatch);
        co_await SendMessageAsync(BatchMessage);
        co_return;
    }

    void SetMessageHandler(std::function<void(const JSONRPCMessage&)> InHandler) override {
        m_MessageHandler = InHandler;
    }

    void SetErrorHandler(std::function<void(const std::string&)> InHandler) override {
        m_ErrorHandler = InHandler;
    }

    bool IsConnected() const override {
        return m_IsConnected;
    }
    std::string GetTransportType() const override {
        return "stdio";
    }

  private:
    std::coroutine_handle<> StartReadingFromStdout() {
        while (m_IsConnected && m_ServerProcess && m_ServerProcess->IsRunning()) {
            std::string Line = co_await m_ServerProcess->ReadLineFromStdout();
            if (!Line.empty()) { ProcessReceivedMessage(Line); }
        }
        co_return;
    }

    std::coroutine_handle<> StartReadingFromStderr() {
        while (m_IsConnected && m_ServerProcess && m_ServerProcess->IsRunning()) {
            std::string LogLine = co_await m_ServerProcess->ReadLineFromStderr();
            if (!LogLine.empty() && m_ErrorHandler) { m_ErrorHandler(LogLine); }
        }
        co_return;
    }

    void ProcessReceivedMessage(const std::string& InJsonData) {
        try {
            JSONRPCMessage Message = DeserializeFromJSON(InJsonData);
            if (m_MessageHandler) { m_MessageHandler(Message); }
        } catch (const std::exception& Ex) {
            if (m_ErrorHandler) {
                m_ErrorHandler("Failed to parse JSON-RPC message: " + std::string(Ex.what()));
            }
        }
    }
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
    std::function<void(const JSONRPCMessage&)> m_MessageHandler;
    std::function<void(const std::string&)> m_ErrorHandler;
    bool m_IsConnected;
    std::string m_LastEventID;

  public:
    StreamableHTTPTransport(const HTTPTransportConfig& InConfig)
        : m_Config(InConfig), m_IsConnected(false) {}

    std::coroutine_handle<> ConnectAsync() override {
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

    std::coroutine_handle<> DisconnectAsync() override {
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

    std::coroutine_handle<> SendMessageAsync(const JSONRPCMessage& InMessage) override {
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

    std::coroutine_handle<> SendBatchAsync(const JSONRPCBatch& InBatch) override {
        JSONRPCMessage BatchMessage = ConvertBatchToMessage(InBatch);
        co_await SendMessageAsync(BatchMessage);
        co_return;
    }

    std::coroutine_handle<> ListenForServerMessages() {
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
                    SSEEvent Event = co_await m_SSEStream->ReadEventAsync();
                    ProcessSSEEvent(Event);
                }
            }
        } catch (const Poco::Exception& Ex) {
            if (m_ErrorHandler) { m_ErrorHandler("SSE stream error: " + Ex.displayText()); }
        }

        co_return;
    }

    void SetMessageHandler(std::function<void(const JSONRPCMessage&)> InHandler) override {
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
    std::coroutine_handle<> SendInitializeRequest(const JSONRPCRequest& InRequest) {
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

    std::coroutine_handle<> SendSessionTermination() {
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

    std::coroutine_handle<> SendHTTPRequest(Poco::Net::HTTPRequest& InRequest,
                                            Poco::Net::HTTPResponse& OutResponse,
                                            const JSONRPCMessage& InMessage) {
        std::string JsonData = SerializeToJSON(InMessage);
        InRequest.setContentLength(JsonData.length());

        std::ostream& RequestStream = m_HTTPSession->sendRequest(InRequest);
        RequestStream << JsonData;

        std::istream& ResponseStream = m_HTTPSession->receiveResponse(OutResponse);

        co_return;
    }

    std::coroutine_handle<> ProcessHTTPResponse(const Poco::Net::HTTPResponse& InResponse,
                                                const JSONRPCMessage& InOriginalMessage) {
        if (InResponse.getStatus() == Poco::Net::HTTPResponse::HTTP_ACCEPTED) {
            // Accepted - for notifications/responses only
            co_return;
        }

        if (InResponse.getStatus() == Poco::Net::HTTPResponse::HTTP_NOT_FOUND
            && m_Session.has_value()) {
            // Session expired - need to reinitialize
            m_Session.reset();
            co_await ConnectAsync();
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

            JSONRPCMessage Response = DeserializeFromJSON(ResponseBody);
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

                JSONRPCMessage Message = DeserializeFromJSON(InEvent.Data);
                if (m_MessageHandler) { m_MessageHandler(Message); }
            } catch (const std::exception& Ex) {
                if (m_ErrorHandler) {
                    m_ErrorHandler("Failed to parse SSE message: " + std::string(Ex.what()));
                }
            }
        }
    }

    Poco::Net::HTTPRequest CreatePOSTRequest(const JSONRPCMessage& InMessage) {
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

// Custom Transport Interface
class CustomTransport : public ITransport {
  private:
    std::string m_CustomTransportType;
    std::function<void(const JSONRPCMessage&)> m_MessageHandler;
    std::function<void(const std::string&)> m_ErrorHandler;

  public:
    CustomTransport(const std::string& InTransportType) : m_CustomTransportType(InTransportType) {}

    // Custom transport implementations must override all pure virtual methods
    virtual std::coroutine_handle<> ConnectAsync() override = 0;
    virtual std::coroutine_handle<> DisconnectAsync() override = 0;
    virtual std::coroutine_handle<> SendMessageAsync(const JSONRPCMessage& InMessage) override = 0;
    virtual std::coroutine_handle<> SendBatchAsync(const JSONRPCBatch& InBatch) override = 0;

    void SetMessageHandler(std::function<void(const JSONRPCMessage&)> InHandler) override {
        m_MessageHandler = InHandler;
    }

    void SetErrorHandler(std::function<void(const std::string&)> InHandler) override {
        m_ErrorHandler = InHandler;
    }

    std::string GetTransportType() const override {
        return m_CustomTransportType;
    }

  protected:
    // Helper methods for custom transport implementations
    void TriggerMessageReceived(const JSONRPCMessage& InMessage) {
        if (m_MessageHandler) { m_MessageHandler(InMessage); }
    }

    void TriggerError(const std::string& InError) {
        if (m_ErrorHandler) { m_ErrorHandler(InError); }
    }
};

// Transport Factory
class TransportFactory {
  public:
    static std::unique_ptr<ITransport>
    CreateStdioTransport(const std::string& InExecutablePath,
                         const std::vector<std::string>& InArguments) {
        return std::make_unique<StdioTransport>(InExecutablePath, InArguments);
    }

    static std::unique_ptr<ITransport>
    CreateStreamableHTTPTransport(const HTTPTransportConfig& InConfig) {
        return std::make_unique<StreamableHTTPTransport>(InConfig);
    }

    static std::unique_ptr<ITransport>
    CreateCustomTransport(const std::string& InTransportType,
                          std::function<std::unique_ptr<CustomTransport>()> InFactory) {
        return InFactory();
    }
};

// Transport Manager - Orchestrates transport lifecycle
class TransportManager {
  private:
    std::unique_ptr<ITransport> m_Transport;
    bool m_IsInitialized;

  public:
    TransportManager() : m_IsInitialized(false) {}

    void SetTransport(std::unique_ptr<ITransport> InTransport) {
        m_Transport = std::move(InTransport);
    }

    std::coroutine_handle<> InitializeAsync() {
        if (!m_Transport) { throw std::runtime_error("No transport configured"); }

        co_await m_Transport->ConnectAsync();
        m_IsInitialized = true;
        co_return;
    }

    std::coroutine_handle<> ShutdownAsync() {
        if (m_Transport && m_IsInitialized) {
            co_await m_Transport->DisconnectAsync();
            m_IsInitialized = false;
        }
        co_return;
    }

    std::coroutine_handle<> SendAsync(const JSONRPCMessage& InMessage) {
        if (!m_IsInitialized) { throw std::runtime_error("Transport not initialized"); }

        co_await m_Transport->SendMessageAsync(InMessage);
        co_return;
    }

    void SetMessageHandler(std::function<void(const JSONRPCMessage&)> InHandler) {
        if (m_Transport) { m_Transport->SetMessageHandler(InHandler); }
    }

    void SetErrorHandler(std::function<void(const std::string&)> InHandler) {
        if (m_Transport) { m_Transport->SetErrorHandler(InHandler); }
    }

    bool IsConnected() const {
        return m_Transport && m_Transport->IsConnected();
    }

    std::string GetTransportType() const {
        return m_Transport ? m_Transport->GetTransportType() : "none";
    }
};

MCP_NAMESPACE_END
