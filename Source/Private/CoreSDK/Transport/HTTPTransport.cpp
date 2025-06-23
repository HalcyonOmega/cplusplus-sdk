#include "CoreSDK/Transport/HTTPTransport.h"

#include <Poco/Net/HTTPMessage.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>
#include <Poco/URI.h>

MCP_NAMESPACE_BEGIN

// HTTPTransportClient Implementation
HTTPTransportClient::HTTPTransportClient(const HTTPTransportOptions& InOptions)
    : m_Options(InOptions) {
    TriggerStateChange(TransportState::Disconnected);
}

HTTPTransportClient::~HTTPTransportClient() {
    if (m_CurrentState != TransportState::Disconnected) {
        try {
            Stop().GetResult();
        } catch (...) {
            // Ignore errors during destruction
        }
    }
}

MCPTask_Void HTTPTransportClient::Start() {
    if (m_CurrentState != TransportState::Disconnected) {
        throw std::runtime_error("Transport already started or in progress");
    }

    try {
        TriggerStateChange(TransportState::Connecting);

        co_await ConnectToServer();

        TriggerStateChange(TransportState::Connected);
    } catch (const std::exception& e) {
        TriggerStateChange(TransportState::Error);
        if (m_ErrorHandler) {
            m_ErrorHandler("Failed to start HTTP transport: " + std::string(e.what()));
        }
        throw;
    }

    co_return;
}

MCPTask_Void HTTPTransportClient::Stop() {
    if (m_CurrentState == TransportState::Disconnected) { co_return; }

    try {
        m_ShouldStop = true;

        // Stop SSE thread
        if (m_SSEThread.isRunning()) { m_SSEThread.join(); }

        Cleanup();
        TriggerStateChange(TransportState::Disconnected);
    } catch (const std::exception& e) {
        TriggerStateChange(TransportState::Error);
        if (m_ErrorHandler) {
            m_ErrorHandler("Error stopping HTTP transport: " + std::string(e.what()));
        }
    }

    co_return;
}

bool HTTPTransportClient::IsConnected() const {
    return m_CurrentState == TransportState::Connected;
}

TransportState HTTPTransportClient::GetState() const {
    return m_CurrentState;
}

MCPTask<std::string> HTTPTransportClient::SendRequest(const RequestBase& InRequest) {
    if (!IsConnected()) { throw std::runtime_error("Transport not connected"); }

    RequestID requestID = InRequest.ID;

    // Create promise for response
    auto pendingRequest = std::make_unique<PendingRequest>();
    pendingRequest->RequestID = requestID;
    pendingRequest->StartTime = std::chrono::steady_clock::now();

    auto future = pendingRequest->Promise.get_future();

    {
        Poco::Mutex::ScopedLock lock(m_RequestsMutex);
        m_PendingRequests[requestID] = std::move(pendingRequest);
    }

    // Send the request via HTTP POST
    co_await SendHTTPMessage(InRequest);

    // Wait for response with timeout
    auto status = future.wait_for(m_Options.RequestTimeout);
    if (status == std::future_status::timeout) {
        Poco::Mutex::ScopedLock lock(m_RequestsMutex);
        m_PendingRequests.erase(requestID);
        throw std::runtime_error("Request timeout");
    }

    co_return future.get();
}

MCPTask_Void HTTPTransportClient::SendResponse(const ResponseBase& InResponse) {
    co_await SendHTTPMessage(InResponse);
}

MCPTask_Void HTTPTransportClient::SendErrorResponse(const ErrorBase& InError) {
    co_await SendHTTPMessage(InError);
}

MCPTask_Void HTTPTransportClient::SendNotification(const NotificationBase& InNotification) {
    co_await SendHTTPMessage(InNotification);
}

void HTTPTransportClient::SetMessageHandler(MessageHandler InHandler) {
    m_MessageHandler = InHandler;
}

void HTTPTransportClient::SetRequestHandler(RequestHandler InHandler) {
    m_RequestHandler = InHandler;
}

void HTTPTransportClient::SetResponseHandler(ResponseHandler InHandler) {
    m_ResponseHandler = InHandler;
}

void HTTPTransportClient::SetNotificationHandler(NotificationHandler InHandler) {
    m_NotificationHandler = InHandler;
}

void HTTPTransportClient::SetErrorHandler(ErrorHandler InHandler) {
    m_ErrorHandler = InHandler;
}

void HTTPTransportClient::SetStateChangeHandler(StateChangeHandler InHandler) {
    m_StateChangeHandler = InHandler;
}

std::string HTTPTransportClient::GetConnectionInfo() const {
    std::string protocol = m_Options.UseHTTPS ? "https" : "http";
    return protocol + "://" + m_Options.Host + ":" + std::to_string(m_Options.Port)
           + m_Options.Path;
}

void HTTPTransportClient::run() {
    StartSSEConnection();
}

MCPTask_Void HTTPTransportClient::ConnectToServer() {
    try {
        Poco::Mutex::ScopedLock lock(m_ConnectionMutex);

        // Create HTTP session
        m_HTTPSession =
            std::make_unique<Poco::Net::HTTPClientSession>(m_Options.Host, m_Options.Port);
        m_HTTPSession->setTimeout(m_Options.ConnectTimeout);

        // Test connection with a ping
        Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_POST, m_Options.Path,
                                       Poco::Net::HTTPMessage::HTTP_1_1);
        request.setContentType("application/json");
        request.set("Accept", "text/event-stream");

        JSONValue pingMessage = {{"jsonrpc", "2.0"}, {"method", "ping"}, {"id", "connection_test"}};

        std::string body = pingMessage.dump();
        request.setContentLength(body.length());

        std::ostream& requestStream = m_HTTPSession->sendRequest(request);
        requestStream << body;

        Poco::Net::HTTPResponse response;
        std::istream& responseStream = m_HTTPSession->receiveResponse(response);

        if (response.getStatus() != Poco::Net::HTTPResponse::HTTP_OK) {
            throw std::runtime_error("Server connection failed: " + response.getReason());
        }

        // Start SSE connection for real-time communication
        m_ShouldStop = false;
        m_SSEThread.start(*this);

    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to connect to HTTP server: " + std::string(e.what()));
    }

    co_return;
}

MCPTask_Void HTTPTransportClient::SendHTTPMessage(const JSONValue& InMessage) {
    if (!m_HTTPSession) { throw std::runtime_error("HTTP session not initialized"); }

    try {
        Poco::Mutex::ScopedLock lock(m_ConnectionMutex);

        Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_POST, m_Options.Path,
                                       Poco::Net::HTTPMessage::HTTP_1_1);
        request.setContentType("application/json");

        std::string body = InMessage.dump();
        request.setContentLength(body.length());

        std::ostream& requestStream = m_HTTPSession->sendRequest(request);
        requestStream << body;

        Poco::Net::HTTPResponse response;
        std::istream& responseStream = m_HTTPSession->receiveResponse(response);

        if (response.getStatus() != Poco::Net::HTTPResponse::HTTP_OK) {
            throw std::runtime_error("HTTP request failed: " + response.getReason());
        }

    } catch (const std::exception& e) {
        HandleConnectionError("Error sending HTTP message: " + std::string(e.what()));
        throw;
    }

    co_return;
}

void HTTPTransportClient::StartSSEConnection() {
    try {
        // Create separate session for SSE
        auto sseSession =
            std::make_unique<Poco::Net::HTTPClientSession>(m_Options.Host, m_Options.Port);

        Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET, m_Options.Path + "/events",
                                       Poco::Net::HTTPMessage::HTTP_1_1);
        request.set("Accept", "text/event-stream");
        request.set("Cache-Control", "no-cache");

        std::ostream& requestStream = sseSession->sendRequest(request);

        Poco::Net::HTTPResponse response;
        m_SSEStream = std::unique_ptr<std::istream>(&sseSession->receiveResponse(response));

        if (response.getStatus() != Poco::Net::HTTPResponse::HTTP_OK) {
            throw std::runtime_error("SSE connection failed: " + response.getReason());
        }

        // Process SSE events
        std::string line;
        while (!m_ShouldStop && std::getline(*m_SSEStream, line)) {
            if (!line.empty()) { ProcessSSELine(line); }
        }

    } catch (const std::exception& e) {
        if (!m_ShouldStop) {
            HandleConnectionError("SSE connection error: " + std::string(e.what()));
        }
    }
}

void HTTPTransportClient::ProcessSSELine(const std::string& InLine) {
    try {
        // SSE format: "data: <json>\n"
        if (InLine.substr(0, 6) == "data: ") {
            std::string jsonData = InLine.substr(6);
            auto message = JSONValue::parse(jsonData);

            if (!IsValidJSONRPC(message)) {
                HandleConnectionError("Invalid JSON-RPC message received via SSE");
                return;
            }

            if (m_MessageHandler) { m_MessageHandler(jsonData); }

            // Check if it's a response to a pending request
            if (message.contains("id")
                && (message.contains("result") || message.contains("error"))) {
                std::string requestID = MessageUtils::ExtractRequestID(message);

                Poco::Mutex::ScopedLock lock(m_RequestsMutex);
                auto it = m_PendingRequests.find(requestID);
                if (it != m_PendingRequests.end()) {
                    if (message.contains("result")) {
                        it->second->Promise.set_value(message["result"].dump());
                    } else {
                        std::string errorMsg = message["error"]["message"].get<std::string>();
                        it->second->Promise.set_exception(
                            std::make_exception_ptr(std::runtime_error(errorMsg)));
                    }
                    m_PendingRequests.erase(it);
                    return;
                }
            }

            // Handle as request or notification
            if (message.contains("method")) {
                std::string method = MessageUtils::ExtractMethod(message);
                JSONValue params = MessageUtils::ExtractParams(message);

                if (message.contains("id")) {
                    // Request
                    if (m_RequestHandler) {
                        std::string requestID = MessageUtils::ExtractRequestID(message);
                        m_RequestHandler(method, params, requestID);
                    }
                } else {
                    // Notification
                    if (m_NotificationHandler) { m_NotificationHandler(method, params); }
                }
            }
        }
    } catch (const std::exception& e) {
        HandleConnectionError("Error processing SSE line: " + std::string(e.what()));
    }
}

void HTTPTransportClient::HandleConnectionError(const std::string& InError) {
    TriggerStateChange(TransportState::Error);
    if (m_ErrorHandler) { m_ErrorHandler(InError); }
}

void HTTPTransportClient::Cleanup() {
    // Close SSE stream
    m_SSEStream.reset();

    // Close HTTP session
    m_HTTPSession.reset();

    // Clear pending requests
    {
        Poco::Mutex::ScopedLock lock(m_RequestsMutex);
        for (auto& [id, request] : m_PendingRequests) {
            request->Promise.set_exception(
                std::make_exception_ptr(std::runtime_error("Transport closed")));
        }
        m_PendingRequests.clear();
    }
}

// MCPHTTPRequestHandler Implementation
MCPHTTPRequestHandler::MCPHTTPRequestHandler(HTTPTransportServer* InServer) : m_Server(InServer) {}

void MCPHTTPRequestHandler::handleRequest(Poco::Net::HTTPServerRequest& InRequest,
                                          Poco::Net::HTTPServerResponse& InResponse) {
    m_Server->HandleHTTPRequest(InRequest, InResponse);
}

// MCPHTTPRequestHandlerFactory Implementation
MCPHTTPRequestHandlerFactory::MCPHTTPRequestHandlerFactory(HTTPTransportServer* InServer)
    : m_Server(InServer) {}

Poco::Net::HTTPRequestHandler*
MCPHTTPRequestHandlerFactory::createRequestHandler(const Poco::Net::HTTPServerRequest& InRequest) {
    return new MCPHTTPRequestHandler(m_Server);
}

// HTTPTransportServer Implementation
HTTPTransportServer::HTTPTransportServer(const HTTPTransportOptions& InOptions)
    : m_Options(InOptions) {
    TriggerStateChange(TransportState::Disconnected);
}

HTTPTransportServer::~HTTPTransportServer() {
    if (m_CurrentState != TransportState::Disconnected) {
        try {
            Stop().GetResult();
        } catch (...) {
            // Ignore errors during destruction
        }
    }
}

MCPTask_Void HTTPTransportServer::Start() {
    if (m_CurrentState != TransportState::Disconnected) {
        throw std::runtime_error("Transport already started");
    }

    try {
        TriggerStateChange(TransportState::Connecting);

        // Create server socket
        m_ServerSocket = std::make_unique<Poco::Net::ServerSocket>(m_Options.Port);

        // Create request handler factory
        m_HandlerFactory = std::make_unique<MCPHTTPRequestHandlerFactory>(this);

        // Create HTTP server
        m_HTTPServer =
            std::make_unique<Poco::Net::HTTPServer>(m_HandlerFactory.get(), *m_ServerSocket);

        // Start the server
        m_HTTPServer->start();

        TriggerStateChange(TransportState::Connected);
    } catch (const std::exception& e) {
        TriggerStateChange(TransportState::Error);
        if (m_ErrorHandler) {
            m_ErrorHandler("Failed to start HTTP server transport: " + std::string(e.what()));
        }
        throw;
    }

    co_return;
}

MCPTask_Void HTTPTransportServer::Stop() {
    if (m_CurrentState == TransportState::Disconnected) { co_return; }

    try {
        // Stop HTTP server
        if (m_HTTPServer) { m_HTTPServer->stop(); }

        // Close all SSE clients
        {
            Poco::Mutex::ScopedLock lock(m_ClientsMutex);
            for (auto& [id, client] : m_SSEClients) { client->IsActive = false; }
            m_SSEClients.clear();
        }

        // Clear pending requests
        {
            Poco::Mutex::ScopedLock lock(m_RequestsMutex);
            for (auto& [id, request] : m_PendingRequests) {
                request->Promise.set_exception(
                    std::make_exception_ptr(std::runtime_error("Server stopped")));
            }
            m_PendingRequests.clear();
        }

        m_HTTPServer.reset();
        m_HandlerFactory.reset();
        m_ServerSocket.reset();

        TriggerStateChange(TransportState::Disconnected);
    } catch (const std::exception& e) {
        TriggerStateChange(TransportState::Error);
        if (m_ErrorHandler) {
            m_ErrorHandler("Error stopping HTTP server transport: " + std::string(e.what()));
        }
    }

    co_return;
}

bool HTTPTransportServer::IsConnected() const {
    return m_CurrentState == TransportState::Connected;
}

TransportState HTTPTransportServer::GetState() const {
    return m_CurrentState;
}

MCPTask<const ResponseBase&> HTTPTransportServer::SendRequest(const RequestBase& InRequest) {
    if (!IsConnected()) { throw std::runtime_error("Transport not connected"); }

    RequestID requestID = InRequest.ID;

    // Create promise for response
    auto pendingRequest = std::make_unique<PendingRequest>();
    pendingRequest->RequestID = requestID;
    pendingRequest->StartTime = std::chrono::steady_clock::now();

    auto future = pendingRequest->Promise.get_future();

    {
        Poco::Mutex::ScopedLock lock(m_RequestsMutex);
        m_PendingRequests[requestID] = std::move(pendingRequest);
    }

    // Send to all SSE clients
    co_await SendToSSEClients(InRequest);

    // Wait for response with timeout
    auto status = future.wait_for(std::chrono::seconds(30));
    if (status == std::future_status::timeout) {
        Poco::Mutex::ScopedLock lock(m_RequestsMutex);
        m_PendingRequests.erase(requestID);
        throw std::runtime_error("Request timeout");
    }

    co_return future.get();
}

MCPTask_Void HTTPTransportServer::SendResponse(const ResponseBase& InResponse) {
    co_await SendToSSEClients(InResponse);
}

MCPTask_Void HTTPTransportServer::SendErrorResponse(const ErrorBase& InError) {
    co_await SendToSSEClients(InError);
}

MCPTask_Void HTTPTransportServer::SendNotification(const NotificationBase& InNotification) {
    co_await SendToSSEClients(InNotification);
}

void HTTPTransportServer::SetMessageHandler(MessageHandler InHandler) {
    m_MessageHandler = InHandler;
}

void HTTPTransportServer::SetRequestHandler(RequestHandler InHandler) {
    m_RequestHandler = InHandler;
}

void HTTPTransportServer::SetResponseHandler(ResponseHandler InHandler) {
    m_ResponseHandler = InHandler;
}

void HTTPTransportServer::SetNotificationHandler(NotificationHandler InHandler) {
    m_NotificationHandler = InHandler;
}

void HTTPTransportServer::SetErrorHandler(ErrorHandler InHandler) {
    m_ErrorHandler = InHandler;
}

void HTTPTransportServer::SetStateChangeHandler(StateChangeHandler InHandler) {
    m_StateChangeHandler = InHandler;
}

std::string HTTPTransportServer::GetConnectionInfo() const {
    std::string protocol = m_Options.UseHTTPS ? "https" : "http";
    // TODO: @HalcyonOmega - Pretty sure spec says you should use 127.0.0.1 instead of 0.0.0.0
    return protocol + "://0.0.0.0:" + std::to_string(m_Options.Port) + m_Options.Path;
}

void HTTPTransportServer::HandleHTTPRequest(Poco::Net::HTTPServerRequest& InRequest,
                                            Poco::Net::HTTPServerResponse& InResponse) {
    try {
        std::string path = InRequest.getURI();
        std::string method = InRequest.getMethod();

        if (path == "/message" && method == "GET") {
            // MCP StreamableHTTP GET endpoint implementation
            InResponse.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
            InResponse.setContentType("text/event-stream");
            InResponse.set("Cache-Control", "no-cache");
            InResponse.set("Connection", "keep-alive");
            InResponse.set("Access-Control-Allow-Origin", "*");
            InResponse.set("Access-Control-Allow-Headers", "Content-Type");
            InResponse.set("Access-Control-Allow-Methods", "GET, POST, OPTIONS");

            std::string clientID = GenerateClientID();
            RegisterSSEClient(clientID, InResponse);

            // Send initial connection event
            std::ostream& responseStream = InResponse.send();
            responseStream << "data: {\"type\":\"connection_established\",\"clientId\":\""
                           << clientID << "\"}\n\n";
            responseStream.flush();

            // Keep connection alive until client disconnects
            while (true) {
                {
                    Poco::Mutex::ScopedLock lock(m_ClientsMutex);
                    auto it = m_SSEClients.find(clientID);
                    if (it == m_SSEClients.end() || !it->second->IsActive) { break; }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

        } else if (path == m_Options.Path + "/events") {
            // Legacy SSE endpoint (kept for backwards compatibility)
            InResponse.setContentType("text/event-stream");
            InResponse.set("Cache-Control", "no-cache");
            InResponse.set("Connection", "keep-alive");
            InResponse.set("Access-Control-Allow-Origin", "*");

            std::string clientID = GenerateClientID();
            RegisterSSEClient(clientID, InResponse);

            // Keep connection alive
            std::ostream& responseStream = InResponse.send();
            responseStream << "data: {\"type\":\"connection_established\"}\n\n";
            responseStream.flush();

            // Wait until client disconnects
            while (true) {
                Poco::Mutex::ScopedLock lock(m_ClientsMutex);
                auto it = m_SSEClients.find(clientID);
                if (it == m_SSEClients.end() || !it->second->IsActive) { break; }
                lock.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

        } else if (path == m_Options.Path && method == "POST") {
            // JSON-RPC endpoint
            InResponse.setContentType("application/json");
            InResponse.set("Access-Control-Allow-Origin", "*");

            std::istream& requestStream = InRequest.stream();
            std::string body;
            Poco::StreamCopier::copyToString(requestStream, body);

            ProcessReceivedMessage(body);

            InResponse.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
            std::ostream& responseStream = InResponse.send();
            responseStream << "{\"status\":\"received\"}\n";

        } else if (method == "OPTIONS") {
            // Handle CORS preflight requests
            InResponse.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
            InResponse.set("Access-Control-Allow-Origin", "*");
            InResponse.set("Access-Control-Allow-Headers", "Content-Type");
            InResponse.set("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            InResponse.set("Access-Control-Max-Age", "86400");
            std::ostream& responseStream = InResponse.send();
            responseStream << "";

        } else {
            // Not found
            InResponse.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
            InResponse.setReason("Not Found");
            std::ostream& responseStream = InResponse.send();
            responseStream << "404 Not Found\n";
        }

    } catch (const std::exception& e) {
        InResponse.setStatus(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
        InResponse.setReason("Internal Server Error");
        std::ostream& responseStream = InResponse.send();
        responseStream << "500 Internal Server Error: " << e.what() << "\n";
    }
}

void HTTPTransportServer::RegisterSSEClient(const std::string& InClientID,
                                            Poco::Net::HTTPServerResponse& InResponse) {
    Poco::Mutex::ScopedLock lock(m_ClientsMutex);

    auto client = std::make_unique<SSEClient>();
    client->ClientID = InClientID;
    client->Response = &InResponse;
    client->Stream = &InResponse.send();
    client->ConnectedTime = std::chrono::steady_clock::now();
    client->IsActive = true;

    m_SSEClients[InClientID] = std::move(client);
}

void HTTPTransportServer::UnregisterSSEClient(const std::string& InClientID) {
    Poco::Mutex::ScopedLock lock(m_ClientsMutex);
    auto it = m_SSEClients.find(InClientID);
    if (it != m_SSEClients.end()) {
        it->second->IsActive = false;
        m_SSEClients.erase(it);
    }
}

MCPTask_Void HTTPTransportServer::SendToSSEClients(const JSONValue& InMessage) {
    Poco::Mutex::ScopedLock lock(m_ClientsMutex);

    std::string messageStr = "data: " + InMessage.dump() + "\n\n";

    auto it = m_SSEClients.begin();
    while (it != m_SSEClients.end()) {
        try {
            if (it->second->IsActive && it->second->Stream) {
                *(it->second->Stream) << messageStr;
                it->second->Stream->flush();
                ++it;
            } else {
                it = m_SSEClients.erase(it);
            }
        } catch (const std::exception&) {
            // Client disconnected
            it = m_SSEClients.erase(it);
        }
    }

    co_return;
}

void HTTPTransportServer::ProcessReceivedMessage(const std::string& InMessage) {
    try {
        auto message = JSONValue::parse(InMessage);

        if (!IsValidJSONRPC(message)) {
            if (m_ErrorHandler) { m_ErrorHandler("Invalid JSON-RPC message received"); }
            return;
        }

        if (m_MessageHandler) { m_MessageHandler(InMessage); }

        // Check if it's a response to a pending request
        if (message.contains("id") && (message.contains("result") || message.contains("error"))) {
            std::string requestID = MessageUtils::ExtractRequestID(message);

            Poco::Mutex::ScopedLock lock(m_RequestsMutex);
            auto it = m_PendingRequests.find(requestID);
            if (it != m_PendingRequests.end()) {
                if (message.contains("result")) {
                    it->second->Promise.set_value(message["result"].dump());
                } else {
                    std::string errorMsg = message["error"]["message"].get<std::string>();
                    it->second->Promise.set_exception(
                        std::make_exception_ptr(std::runtime_error(errorMsg)));
                }
                m_PendingRequests.erase(it);
                return;
            }
        }

        // Handle as request or notification
        if (message.contains("method")) {
            std::string method = MessageUtils::ExtractMethod(message);
            JSONValue params = MessageUtils::ExtractParams(message);

            if (message.contains("id")) {
                // Request
                if (m_RequestHandler) {
                    std::string requestID = MessageUtils::ExtractRequestID(message);
                    m_RequestHandler(method, params, requestID);
                }
            } else {
                // Notification
                if (m_NotificationHandler) { m_NotificationHandler(method, params); }
            }
        }
    } catch (const std::exception& e) {
        if (m_ErrorHandler) { m_ErrorHandler("Error parsing message: " + std::string(e.what())); }
    }
}

std::string HTTPTransportServer::GenerateClientID() const {
    uint64_t counter = m_ClientCounter.fetch_add(1);
    return "client_" + std::to_string(counter);
}

std::string HTTPTransportServer::GenerateUniqueClientID() const {
    return GenerateClientID();
}

MCPTask_Void
HTTPTransportServer::HandleGetMessageEndpoint(Poco::Net::HTTPServerRequest& InRequest,
                                              Poco::Net::HTTPServerResponse& InResponse) {
    // Set SSE headers
    InResponse.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
    InResponse.setContentType("text/event-stream");
    InResponse.set("Cache-Control", "no-cache");
    InResponse.set("Connection", "keep-alive");
    InResponse.set("Access-Control-Allow-Origin", "*");
    InResponse.set("Access-Control-Allow-Headers", "Content-Type");

    // Generate unique client ID
    std::string clientID = GenerateUniqueClientID();

    // Register SSE client for message streaming
    RegisterSSEClient(clientID, InResponse);

    // Keep connection alive and stream messages
    co_await StreamMessagesToClient(clientID);
}

MCPTask_Void HTTPTransportServer::StreamMessagesToClient(const std::string& InClientID) {
    try {
        while (true) {
            {
                Poco::Mutex::ScopedLock lock(m_ClientsMutex);
                auto it = m_SSEClients.find(InClientID);
                if (it == m_SSEClients.end() || !it->second->IsActive) { break; }
            }

            // Wait a bit before checking again
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } catch (const std::exception& e) {
        // Client disconnected or error occurred
        UnregisterSSEClient(InClientID);
    }

    co_return;
}

// Factory functions
std::unique_ptr<ITransport> CreateHTTPTransportImpl(const HTTPTransportOptions& InOptions) {
    return std::make_unique<HTTPTransportClient>(InOptions);
}

MCP_NAMESPACE_END