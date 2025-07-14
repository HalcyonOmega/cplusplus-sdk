#include "CoreSDK/Transport/HTTPTransport.h"

#include <Poco/Net/HTTPMessage.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>
#include <Poco/URI.h>

#include <utility>

#include "CoreSDK/Common/RuntimeError.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Utilities/JSON/JSONMessages.h"

MCP_NAMESPACE_BEGIN

HTTPTransportClient::HTTPTransportClient(HTTPTransportOptions InOptions) : ITransport(), m_Options(std::move(InOptions))
{}

HTTPTransportClient::~HTTPTransportClient() noexcept
{
	if (GetState() != TransportState::Disconnected)
	{
		try
		{
			HTTPTransportClient::Disconnect().await_resume();
		}
		catch (...)
		{
			// Ignore errors during destruction
		}
	}
}

VoidTask HTTPTransportClient::Connect()
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

VoidTask HTTPTransportClient::Disconnect()
{
	if (GetState() == TransportState::Disconnected)
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
	const std::string Protocol = m_Options.UseHTTPS ? "https" : "http";
	return Protocol + "://" + m_Options.Host + ":" + std::to_string(m_Options.Port) + m_Options.Path;
}

void HTTPTransportClient::run() { StartSSEConnection(); }

VoidTask HTTPTransportClient::ConnectToServer()
{
	try
	{
		std::lock_guard lock(m_ConnectionMutex);

		// Create an HTTP session
		m_HTTPSession = std::make_unique<Poco::Net::HTTPClientSession>(m_Options.Host, m_Options.Port);
		m_HTTPSession->setTimeout(m_Options.ConnectTimeout);

		// Test connection with a ping
		Poco::Net::HTTPRequest Request(Poco::Net::HTTPRequest::HTTP_POST,
			m_Options.Path,
			Poco::Net::HTTPMessage::HTTP_1_1);
		Request.setContentType("application/json");
		Request.set("Accept", "text/event-stream");

		// TODO: @HalcyonOmega Update this to use the actual ping type
		const JSONData PingMessage = { { "jsonrpc", "2.0" }, { "method", "ping" }, { "id", "connection_test" } };

		const std::string Body = PingMessage.dump();
		Request.setContentLength(static_cast<std::streamsize>(Body.length()));

		std::ostream& RequestStream = m_HTTPSession->sendRequest(Request);
		RequestStream << Body;

		Poco::Net::HTTPResponse Response;
		const std::istream& ResponseStream = m_HTTPSession->receiveResponse(Response);

		(void)ResponseStream;

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

void HTTPTransportClient::TransmitMessage(const JSONData& InMessage,
	const std::optional<std::vector<ConnectionID>>& InConnectionIDs)
{
	(void)InConnectionIDs;

	if (!m_HTTPSession)
	{
		HandleRuntimeError("HTTP session not initialized");
		return;
	}

	try
	{
		std::lock_guard Lock(m_ConnectionMutex);

		Poco::Net::HTTPRequest Request(Poco::Net::HTTPRequest::HTTP_POST,
			m_Options.Path,
			Poco::Net::HTTPMessage::HTTP_1_1);
		Request.setContentType("application/json");
		Request.set("MCP-Protocol-Version", m_ClientInfo.ProtocolVersion);

		const std::string Body = InMessage.dump();
		Request.setContentLength(static_cast<std::streamsize>(Body.length()));

		std::ostream& RequestStream = m_HTTPSession->sendRequest(Request);
		RequestStream << Body;

		Poco::Net::HTTPResponse Response;
		const std::istream& ResponseStream = m_HTTPSession->receiveResponse(Response);
		(void)ResponseStream;

		if (Response.getStatus() != Poco::Net::HTTPResponse::HTTP_OK)
		{
			HandleRuntimeError("HTTP request failed: " + Response.getReason());
		}
	}
	catch (const std::exception& Except)
	{
		HandleRuntimeError("Error sending HTTP message: " + std::string(Except.what()));
	}
}

void HTTPTransportClient::StartSSEConnection()
{
	try
	{
		// Create a separate session for SSE
		const auto SSESession = std::make_unique<Poco::Net::HTTPClientSession>(m_Options.Host, m_Options.Port);

		Poco::Net::HTTPRequest Request(Poco::Net::HTTPRequest::HTTP_GET,
			m_Options.Path + "/events",
			Poco::Net::HTTPMessage::HTTP_1_1);
		Request.set("Accept", "text/event-stream");
		Request.set("Cache-Control", "no-cache");

		const std::ostream& RequestStream = SSESession->sendRequest(Request);
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
			const auto Message = JSONData::parse(JSONData);

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
	m_Client->m_MessageManager->ClearPendingRequests;
}

// MCPHTTPRequestHandler Implementation
MCPHTTPRequestHandler::MCPHTTPRequestHandler(HTTPTransportServer* InServer) : m_Server(InServer) {}

void MCPHTTPRequestHandler::handleRequest(Poco::Net::HTTPServerRequest& InRequest,
	Poco::Net::HTTPServerResponse& InResponse)
{
	m_Server->HandleHTTPRequest(InRequest, InResponse);
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
HTTPTransportServer::HTTPTransportServer(HTTPTransportOptions InOptions) : m_Options(std::move(InOptions))
{
	SetState(TransportState::Disconnected);
}

HTTPTransportServer::~HTTPTransportServer()
{
	if (GetState() != TransportState::Disconnected)
	{
		try
		{
			HTTPTransportServer::Disconnect().await_resume();
		}
		catch (...)
		{
			// Ignore errors during destruction
		}
	}
}

VoidTask HTTPTransportServer::Connect()
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

		// Create a request handler factory
		m_HandlerFactory = std::make_unique<MCPHTTPRequestHandlerFactory>(this);

		const auto& Params = new Poco::Net::HTTPServerParams();

		// Create an HTTP server
		m_HTTPServer = std::make_unique<Poco::Net::HTTPServer>(m_HandlerFactory.get(), *m_ServerSocket, Params);

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

VoidTask HTTPTransportServer::Disconnect()
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
			std::lock_guard Lock(m_ClientsMutex);
			for (const auto& client : m_SSEClients | std::views::values)
			{
				client->IsActive = false;
			}
			m_SSEClients.clear();
		}

		// Clear pending requests
		{
			std::lock_guard Lock(m_RequestsMutex);
			for (const auto& request : m_PendingRequests | std::views::values)
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
void HTTPTransportServer::TransmitMessage(const JSONData& InMessage,
	const std::optional<std::vector<ConnectionID>>& InConnectionIDs)
{}

std::string HTTPTransportServer::GetConnectionInfo() const
{
	const std::string Protocol = m_Options.UseHTTPS ? "https" : "http";
	// TODO: @HalcyonOmega - Pretty sure spec says you should use 127.0.0.1
	// instead of 0.0.0.0
	return Protocol + "://127.0.0.1:" + std::to_string(m_Options.Port) + m_Options.Path;
}

void HTTPTransportServer::HandleHTTPRequest(Poco::Net::HTTPServerRequest& InRequest,
	Poco::Net::HTTPServerResponse& InResponse)
{
	try
	{
		const std::string Path = InRequest.getURI();

		if (const std::string Method = InRequest.getMethod(); Path == "/message" && Method == "GET")
		{
			// MCP StreamableHTTP GET endpoint implementation
			InResponse.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
			InResponse.setContentType("text/event-stream");
			InResponse.set("Cache-Control", "no-cache");
			InResponse.set("Connection", "keep-alive");
			InResponse.set("Access-Control-Allow-Origin", "*");
			InResponse.set("Access-Control-Allow-Headers", "Content-Type");
			InResponse.set("Access-Control-Allow-Methods", "GET, POST, OPTIONS");

			const std::string ClientID = GenerateUUID();
			RegisterSSEClient(ClientID, InResponse);

			// Send initial connection event
			std::ostream& ResponseStream = InResponse.send();
			ResponseStream << R"(data: {"type":"connection_established","clientId":")" << ClientID << "\"}\n\n";
			ResponseStream.flush();

			// Keep the connection alive until the client disconnects
			while (true)
			{
				{
					std::lock_guard Lock(m_ClientsMutex);

					if (auto Iter = m_SSEClients.find(ClientID); Iter == m_SSEClients.end() || !Iter->second->IsActive)
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

			// Wait until the client disconnects
			while (true)
			{
				std::lock_guard Lock(m_ClientsMutex);

				if (auto Iterator = m_SSEClients.find(ClientID);
					Iterator == m_SSEClients.end() || !Iterator->second->IsActive)
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
	std::lock_guard Lock(m_ClientsMutex);

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
	std::lock_guard Lock(m_ClientsMutex);

	if (const auto Iterator = m_SSEClients.find(InClientID); Iterator != m_SSEClients.end())
	{
		Iterator->second->IsActive = false;
		m_SSEClients.erase(Iterator);
	}
}

void HTTPTransportServer::TransmitMessage(const JSONData& InMessage)
{
	std::lock_guard Lock(m_ClientsMutex);

	const std::string MessageStr = "data: " + InMessage.dump() + "\n\n";

	auto Iterator = m_SSEClients.begin();
	while (Iterator != m_SSEClients.end())
	{
		try
		{
			if (Iterator->second->IsActive && Iterator->second->Stream != nullptr)
			{
				*(Iterator->second->Stream) << MessageStr;
				Iterator->second->Stream->flush();
				++Iterator;
			}
			else
			{
				Iterator = m_SSEClients.erase(Iterator);
			}
		}
		catch (const std::exception&)
		{
			// Client disconnected
			Iterator = m_SSEClients.erase(Iterator);
		}
	}

	co_return;
}

void HTTPTransportServer::ProcessReceivedMessage(const std::string& InMessage) { CallMessageRouter(InMessage); }

VoidTask HTTPTransportServer::HandleGetMessageEndpoint(const Poco::Net::HTTPServerRequest& InRequest,
	Poco::Net::HTTPServerResponse& InResponse)
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
	const std::string ClientID = GenerateUUID();

	// Register an SSE client for message streaming
	RegisterSSEClient(ClientID, InResponse);

	// Keep connection alive and stream messages
	co_await StreamMessagesToClient(ClientID);
}

VoidTask HTTPTransportServer::StreamMessagesToClient(const std::string& InClientID)
{
	try
	{
		while (true)
		{
			{
				std::lock_guard Lock(m_ClientsMutex);
				if (auto Iterator = m_SSEClients.find(InClientID);
					Iterator == m_SSEClients.end() || !Iterator->second->IsActive)
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
		(void)Except;
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