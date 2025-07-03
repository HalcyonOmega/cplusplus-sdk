#include "CoreSDK/Transport/HTTPTransport.h"

#include <Poco/Net/HTTPMessage.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>
#include <Poco/URI.h>

#include "CoreSDK/Common/RuntimeError.h"
#include "Utilities/JSON/JSONMessages.h"

MCP_NAMESPACE_BEGIN

HTTPTransportClient::HTTPTransportClient(const HTTPTransportOptions& InOptions) : m_Options(InOptions) {}

HTTPTransportClient::~HTTPTransportClient() noexcept
{
	if (GetState() != TransportState::Disconnected)
	{
		try
		{
			Disconnect().await_resume();
		}
		catch (...)
		{
			// Ignore errors during destruction
		}
	}
}

MCPTask_Void HTTPTransportClient::Connect()
{
	if (GetState() != TransportState::Disconnected)
	{
		HandleRuntimeError("Transport already started or in progress");
		co_return;
	}

	try
	{
		SetState(TransportState::Connecting);

		co_await ConnectToServer();

		SetState(TransportState::Connected);
	}
	catch (const std::exception& e)
	{
		SetState(TransportState::Error);
		HandleRuntimeError("Failed to start HTTP transport: " + std::string(e.what()));
		co_return;
	}

	co_return;
}

MCPTask_Void HTTPTransportClient::Disconnect()
{
	if (m_CurrentState == TransportState::Disconnected)
	{
		co_return;
	}

	try
	{
		m_ShouldStop = true;

		// Stop SSE thread
		if (m_SSEThread.isRunning())
		{
			m_SSEThread.join();
		}

		Cleanup();
		SetState(TransportState::Disconnected);
	}
	catch (const std::exception& Except)
	{
		SetState(TransportState::Error);
		HandleRuntimeError("Error stopping HTTP transport: " + std::string(Except.what()));
	}

	co_return;
}

std::string HTTPTransportClient::GetConnectionInfo() const
{
	std::string Protocol = m_Options.UseHTTPS ? "https" : "http";
	return Protocol + "://" + m_Options.Host + ":" + std::to_string(m_Options.Port) + m_Options.Path;
}

void HTTPTransportClient::run() { StartSSEConnection(); }

MCPTask_Void HTTPTransportClient::ConnectToServer()
{
	try
	{
		std::lock_guard<std::mutex> lock(m_ConnectionMutex);

		// Create HTTP session
		m_HTTPSession = std::make_unique<Poco::Net::HTTPClientSession>(m_Options.Host, m_Options.Port);
		m_HTTPSession->setTimeout(m_Options.ConnectTimeout);

		// Test connection with a ping
		Poco::Net::HTTPRequest Request(
			Poco::Net::HTTPRequest::HTTP_POST, m_Options.Path, Poco::Net::HTTPMessage::HTTP_1_1);
		Request.setContentType("application/json");
		Request.set("Accept", "text/event-stream");

		// TODO: @HalcyonOmega Update this to use the actual ping type
		JSONData PingMessage = { { "jsonrpc", "2.0" }, { "method", "ping" }, { "id", "connection_test" } };

		std::string Body = PingMessage.dump();
		Request.setContentLength(static_cast<std::streamsize>(Body.length()));

		std::ostream& RequestStream = m_HTTPSession->sendRequest(Request);
		RequestStream << Body;

		Poco::Net::HTTPResponse Response;
		std::istream& ResponseStream = m_HTTPSession->receiveResponse(Response);

		if (Response.getStatus() != Poco::Net::HTTPResponse::HTTP_OK)
		{
			HandleRuntimeError("Server connection failed: " + Response.getReason());
			co_return;
		}

		// Start SSE connection for real-time communication
		m_ShouldStop = false;
		m_SSEThread.start(*this);
	}
	catch (const std::exception& e)
	{
		HandleRuntimeError("Failed to connect to HTTP server: " + std::string(e.what()));
		co_return;
	}

	co_return;
}

MCPTask_Void HTTPTransportClient::TransmitMessage(
	const JSONData& InMessage, const std::optional<std::vector<ConnectionID>>& InConnectionIDs)
{
	(void)InConnectionIDs;

	if (!m_HTTPSession)
	{
		HandleRuntimeError("HTTP session not initialized");
		co_return;
	}

	try
	{
		std::lock_guard<std::mutex> Lock(m_ConnectionMutex);

		Poco::Net::HTTPRequest Request(
			Poco::Net::HTTPRequest::HTTP_POST, m_Options.Path, Poco::Net::HTTPMessage::HTTP_1_1);
		Request.setContentType("application/json");

		std::string Body = InMessage.dump();
		Request.setContentLength(static_cast<std::streamsize>(Body.length()));

		std::ostream& RequestStream = m_HTTPSession->sendRequest(Request);
		RequestStream << Body;

		Poco::Net::HTTPResponse Response;
		std::istream& ResponseStream = m_HTTPSession->receiveResponse(Response);
		(void)ResponseStream;

		if (Response.getStatus() != Poco::Net::HTTPResponse::HTTP_OK)
		{
			HandleRuntimeError("HTTP request failed: " + Response.getReason());
			co_return;
		}
	}
	catch (const std::exception& Except)
	{
		HandleRuntimeError("Error sending HTTP message: " + std::string(Except.what()));
		co_return;
	}

	co_return;
}

void HTTPTransportClient::StartSSEConnection()
{
	try
	{
		// Create separate session for SSE
		auto SSESession = std::make_unique<Poco::Net::HTTPClientSession>(m_Options.Host, m_Options.Port);

		Poco::Net::HTTPRequest Request(
			Poco::Net::HTTPRequest::HTTP_GET, m_Options.Path + "/events", Poco::Net::HTTPMessage::HTTP_1_1);
		Request.set("Accept", "text/event-stream");
		Request.set("Cache-Control", "no-cache");

		std::ostream& RequestStream = SSESession->sendRequest(Request);
		(void)RequestStream;

		Poco::Net::HTTPResponse Response;
		m_SSEStream = std::unique_ptr<std::istream>(&SSESession->receiveResponse(Response));

		if (Response.getStatus() != Poco::Net::HTTPResponse::HTTP_OK)
		{
			HandleRuntimeError("SSE connection failed: " + Response.getReason());
			return;
		}

		// Process SSE events
		std::string Line;
		while (!m_ShouldStop && std::getline(*m_SSEStream, Line))
		{
			if (!Line.empty())
			{
				ProcessSSELine(Line);
			}
		}
	}
	catch (const std::exception& Except)
	{
		if (!m_ShouldStop)
		{
			HandleRuntimeError("SSE connection error: " + std::string(Except.what()));
		}
	}
}

void HTTPTransportClient::ProcessSSELine(const std::string& InLine)
{
	try
	{
		// SSE format: "data: <json>\n"
		if (InLine.substr(0, 6) == "data: ")
		{
			std::string JSONData = InLine.substr(6);
			auto Message = JSONData::parse(JSONData);

			if (!IsValidJSONRPC(Message))
			{
				HandleRuntimeError("Invalid JSON-RPC message received via SSE");
				return;
			}
			CallMessageRouter(Message);
		}
	}
	catch (const std::exception& Except)
	{
		HandleRuntimeError("Error processing SSE line: " + std::string(Except.what()));
	}
}

void HTTPTransportClient::Cleanup()
{
	// Close SSE stream
	m_SSEStream.reset();

	// Close HTTP session
	m_HTTPSession.reset();

	// Clear pending requests
	{
		std::lock_guard<std::mutex> Lock(m_RequestsMutex);
		for (auto& [id, request] : m_PendingRequests)
		{
			request->Promise.set_exception(std::make_exception_ptr(std::runtime_error("Transport closed")));
		}
		m_PendingRequests.clear();
	}
}

// MCPHTTPRequestHandler Implementation
MCPHTTPRequestHandler::MCPHTTPRequestHandler(HTTPTransportServer* InServer) : m_Server(InServer) {}

void MCPHTTPRequestHandler::handleRequest(
	Poco::Net::HTTPServerRequest& Request, Poco::Net::HTTPServerResponse& Response)
{
	m_Server->HandleHTTPRequest(Request, Response);
}

// MCPHTTPRequestHandlerFactory Implementation
MCPHTTPRequestHandlerFactory::MCPHTTPRequestHandlerFactory(HTTPTransportServer* InServer) : m_Server(InServer) {}

Poco::Net::HTTPRequestHandler* MCPHTTPRequestHandlerFactory::createRequestHandler(
	const Poco::Net::HTTPServerRequest& InRequest)
{
	// TODO: @HalcyonOmega - Why does the request need to be passed in?
	(void)InRequest;
	return new MCPHTTPRequestHandler(m_Server);
}

// HTTPTransportServer Implementation
HTTPTransportServer::HTTPTransportServer(const HTTPTransportOptions& InOptions) : m_Options(InOptions)
{
	SetState(TransportState::Disconnected);
}

HTTPTransportServer::~HTTPTransportServer()
{
	if (m_CurrentState != TransportState::Disconnected)
	{
		try
		{
			Disconnect().await_resume();
		}
		catch (...)
		{
			// Ignore errors during destruction
		}
	}
}

MCPTask_Void HTTPTransportServer::Connect()
{
	if (GetState() != TransportState::Disconnected)
	{
		HandleRuntimeError("Transport already started");
	}

	try
	{
		SetState(TransportState::Connecting);

		// Create server socket
		m_ServerSocket = std::make_unique<Poco::Net::ServerSocket>(m_Options.Port);

		// Create request handler factory
		m_HandlerFactory = std::make_unique<MCPHTTPRequestHandlerFactory>(this);

		// Create HTTP server
		m_HTTPServer = std::make_unique<Poco::Net::HTTPServer>(m_HandlerFactory.get(), *m_ServerSocket);

		// Start the server
		m_HTTPServer->start();

		SetState(TransportState::Connected);
	}
	catch (const std::exception& e)
	{
		SetState(TransportState::Error);
		HandleRuntimeError("Failed to start HTTP server transport: " + std::string(e.what()));
	}

	co_return;
}

MCPTask_Void HTTPTransportServer::Disconnect()
{
	if (GetState() == TransportState::Disconnected)
	{
		co_return;
	}

	try
	{
		// Stop HTTP server
		if (m_HTTPServer)
		{
			m_HTTPServer->stop();
		}

		// Close all SSE clients
		{
			std::lock_guard<std::mutex> Lock(m_ClientsMutex);
			for (auto& [id, client] : m_SSEClients)
			{
				client->IsActive = false;
			}
			m_SSEClients.clear();
		}

		// Clear pending requests
		{
			std::lock_guard<std::mutex> Lock(m_RequestsMutex);
			for (auto& [id, request] : m_PendingRequests)
			{
				request->Promise.set_exception(std::make_exception_ptr(std::runtime_error("Server stopped")));
			}
			m_PendingRequests.clear();
		}

		m_HTTPServer.reset();
		m_HandlerFactory.reset();
		m_ServerSocket.reset();

		SetState(TransportState::Disconnected);
	}
	catch (const std::exception& Except)
	{
		SetState(TransportState::Error);
		HandleRuntimeError("Error stopping HTTP server transport: " + std::string(Except.what()));
	}

	co_return;
}

std::string HTTPTransportServer::GetConnectionInfo() const
{
	std::string Protocol = m_Options.UseHTTPS ? "https" : "http";
	// TODO: @HalcyonOmega - Pretty sure spec says you should use 127.0.0.1
	// instead of 0.0.0.0
	return Protocol + "://0.0.0.0:" + std::to_string(m_Options.Port) + m_Options.Path;
}

void HTTPTransportServer::HandleHTTPRequest(
	Poco::Net::HTTPServerRequest& InRequest, Poco::Net::HTTPServerResponse& InResponse)
{
	try
	{
		std::string Path = InRequest.getURI();
		std::string Method = InRequest.getMethod();

		if (Path == "/message" && Method == "GET")
		{
			// MCP StreamableHTTP GET endpoint implementation
			InResponse.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
			InResponse.setContentType("text/event-stream");
			InResponse.set("Cache-Control", "no-cache");
			InResponse.set("Connection", "keep-alive");
			InResponse.set("Access-Control-Allow-Origin", "*");
			InResponse.set("Access-Control-Allow-Headers", "Content-Type");
			InResponse.set("Access-Control-Allow-Methods", "GET, POST, OPTIONS");

			std::string ClientID = GenerateUUID();
			RegisterSSEClient(ClientID, InResponse);

			// Send initial connection event
			std::ostream& ResponseStream = InResponse.send();
			ResponseStream << "data: {\"type\":\"connection_established\",\"clientId\":\"" << ClientID << "\"}\n\n";
			ResponseStream.flush();

			// Keep connection alive until client disconnects
			while (true)
			{
				{
					std::lock_guard<std::mutex> Lock(m_ClientsMutex);

					auto Iter = m_SSEClients.find(ClientID);
					if (Iter == m_SSEClients.end() || !Iter->second->IsActive)
					{
						break;
					}
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}
		else if (Path == m_Options.Path + "/events")
		{
			// Legacy SSE endpoint (kept for backwards compatibility)
			InResponse.setContentType("text/event-stream");
			InResponse.set("Cache-Control", "no-cache");
			InResponse.set("Connection", "keep-alive");
			InResponse.set("Access-Control-Allow-Origin", "*");

			std::string ClientID = GenerateUUID();
			RegisterSSEClient(ClientID, InResponse);

			// Keep connection alive
			std::ostream& ResponseStream = InResponse.send();
			ResponseStream << "data: {\"type\":\"connection_established\"}\n\n";
			ResponseStream.flush();

			// Wait until client disconnects
			while (true)
			{
				std::lock_guard<std::mutex> Lock(m_ClientsMutex);

				auto Iterator = m_SSEClients.find(ClientID);
				if (Iterator == m_SSEClients.end() || !Iterator->second->IsActive)
				{
					break;
				}
				Lock.unlock();
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}
		else if (Path == m_Options.Path && Method == "POST")
		{
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
		}
		else if (Method == "OPTIONS")
		{
			// Handle CORS preflight requests
			InResponse.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
			InResponse.set("Access-Control-Allow-Origin", "*");
			InResponse.set("Access-Control-Allow-Headers", "Content-Type");
			InResponse.set("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
			InResponse.set("Access-Control-Max-Age", "86400");
			std::ostream& responseStream = InResponse.send();
			responseStream << "";
		}
		else
		{
			// Not found
			InResponse.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
			InResponse.setReason("Not Found");
			std::ostream& responseStream = InResponse.send();
			responseStream << "404 Not Found\n";
		}
	}
	catch (const std::exception& Except)
	{
		InResponse.setStatus(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
		InResponse.setReason("Internal Server Error");
		std::ostream& responseStream = InResponse.send();
		responseStream << "500 Internal Server Error: " << Except.what() << "\n";
	}
}

void HTTPTransportServer::RegisterSSEClient(const std::string& InClientID, Poco::Net::HTTPServerResponse& InResponse)
{
	std::lock_guard<std::mutex> Lock(m_ClientsMutex);

	auto client = std::make_unique<SSEClient>();
	client->ClientID = InClientID;
	client->Response = &InResponse;
	client->Stream = &InResponse.send();
	client->ConnectedTime = std::chrono::steady_clock::now();
	client->IsActive = true;

	m_SSEClients[InClientID] = std::move(client);
}

void HTTPTransportServer::UnregisterSSEClient(const std::string& InClientID)
{
	std::lock_guard<std::mutex> Lock(m_ClientsMutex);

	auto It = m_SSEClients.find(InClientID);
	if (It != m_SSEClients.end())
	{
		It->second->IsActive = false;
		m_SSEClients.erase(It);
	}
}

MCPTask_Void HTTPTransportServer::TransmitMessage(const JSONData& InMessage)
{
	std::lock_guard<std::mutex> Lock(m_ClientsMutex);

	std::string MessageStr = "data: " + InMessage.dump() + "\n\n";

	auto It = m_SSEClients.begin();
	while (It != m_SSEClients.end())
	{
		try
		{
			if (It->second->IsActive && (It->second->Stream != nullptr))
			{
				*(It->second->Stream) << MessageStr;
				It->second->Stream->flush();
				++It;
			}
			else
			{
				It = m_SSEClients.erase(It);
			}
		}
		catch (const std::exception&)
		{
			// Client disconnected
			It = m_SSEClients.erase(It);
		}
	}

	co_return;
}

void HTTPTransportServer::ProcessReceivedMessage(const std::string& InMessage) { CallMessageRouter(InMessage); }

MCPTask_Void HTTPTransportServer::HandleGetMessageEndpoint(
	Poco::Net::HTTPServerRequest& InRequest, Poco::Net::HTTPServerResponse& InResponse)
{
	(void)InRequest;
	// Set SSE headers
	InResponse.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
	InResponse.setContentType("text/event-stream");
	InResponse.set("Cache-Control", "no-cache");
	InResponse.set("Connection", "keep-alive");
	InResponse.set("Access-Control-Allow-Origin", "*");
	InResponse.set("Access-Control-Allow-Headers", "Content-Type");

	// Generate unique client ID
	std::string clientID = GenerateUUID();

	// Register SSE client for message streaming
	RegisterSSEClient(clientID, InResponse);

	// Keep connection alive and stream messages
	co_await StreamMessagesToClient(clientID);
}

MCPTask_Void HTTPTransportServer::StreamMessagesToClient(const std::string& InClientID)
{
	try
	{
		while (true)
		{
			{
				std::lock_guard<std::mutex> Lock(m_ClientsMutex);
				auto It = m_SSEClients.find(InClientID);
				if (It == m_SSEClients.end() || !It->second->IsActive)
				{
					break;
				}
			}

			// Wait a bit before checking again
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
	catch (const std::exception& Except)
	{
		// Client disconnected or error occurred
		UnregisterSSEClient(InClientID);
	}

	co_return;
}

// Factory functions
std::unique_ptr<ITransport> CreateHTTPTransportImpl(const HTTPTransportOptions& InOptions)
{
	return std::make_unique<HTTPTransportClient>(InOptions);
}

MCP_NAMESPACE_END