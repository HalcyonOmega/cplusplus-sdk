#include "CoreSDK/Transport/HTTPTransport.h"

#include <Poco/Net/HTTPMessage.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>
#include <Poco/URI.h>

#include <utility>

#include "CoreSDK/Common/RuntimeError.h"
#include "CoreSDK/Messages/MCPMessages.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Utilities/JSON/JSONMessages.h"

MCP_NAMESPACE_BEGIN

HTTPTransportClient::HTTPTransportClient(HTTPTransportOptions InOptions) : ITransport(), m_Options(std::move(InOptions))
{}

HTTPTransportClient::~HTTPTransportClient() noexcept
{
	if (GetState() != ETransportState::Disconnected)
	{
		try
		{
			HTTPTransportClient::Disconnect();
		}
		catch (...)
		{
			// Ignore errors during destruction
		}
	}
}

void HTTPTransportClient::Connect()
{
	if (GetState() != ETransportState::Disconnected)
	{
		HandleRuntimeError("Transport already started or in progress");
		return;
	}

	try
	{
		SetState(ETransportState::Connecting);

		ConnectToServer();

		SetState(ETransportState::Connected);
	}
	catch (const std::exception& e)
	{
		SetState(ETransportState::Error);
		HandleRuntimeError("Failed to start HTTP transport: " + std::string(e.what()));
	}
}

void HTTPTransportClient::Disconnect()
{
	if (GetState() == ETransportState::Disconnected)
	{
		return;
	}

	try
	{
		m_ShouldStop = true;

		// Stop SSE thread
		if (m_SSEThread.joinable())
		{
			m_SSEThread.join();
		}

		Cleanup();
		SetState(ETransportState::Disconnected);
	}
	catch (const std::exception& Except)
	{
		SetState(ETransportState::Error);
		HandleRuntimeError("Error stopping HTTP transport: " + std::string(Except.what()));
	}
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

		const JSONData PingMessage = PingRequest{};

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
		// TODO: @HalcyonOmega This should start automatically
		// m_SSEThread.start(*this);
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
		Request.set("MCP-Protocol-Version", ToString(m_Options.ProtocolVersion));

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
	m_SSEStream.reset();
	m_HTTPSession.reset();
}

// MCPHTTPRequestHandler Implementation
MCPHTTPRequestHandler::MCPHTTPRequestHandler(const Poco::Net::HTTPServerRequest& InResponse,
	HTTPTransportServer* InServer)
	: m_Server(InServer)
{}

void MCPHTTPRequestHandler::handleRequest(Poco::Net::HTTPServerRequest& InRequest,
	Poco::Net::HTTPServerResponse& InResponse)
{
	LogMessage("HTTP Request Handler - Request Received: " + InRequest.getURI() + " " + InRequest.getMethod());
	if (InRequest.getMethod() == "GET")
	{
		InResponse.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
		InResponse.setContentType("text/plain");
		std::ostream& responseStream = InResponse.send();
		responseStream << "Hey Hey Hey it's MCP!\n";
	}
	else
	{
		m_Server->HandleHTTPRequest(InRequest, InResponse);
	}
}

// MCPHTTPRequestHandlerFactory Implementation
MCPHTTPRequestHandlerFactory::MCPHTTPRequestHandlerFactory() {}

bool MCPHTTPRequestHandlerFactory::SetServer(HTTPTransportServer* InServer)
{
	if (InServer)
	{
		m_Server = InServer;
		LogMessage("Server Set Successfully For Handler Factory!");
		return true;
	}
	return false;
}

Poco::Net::HTTPRequestHandler* MCPHTTPRequestHandlerFactory::createRequestHandler(
	const Poco::Net::HTTPServerRequest& InRequest)
{
	if (m_Server)
	{
		return new MCPHTTPRequestHandler(InRequest, m_Server);
	}
	return nullptr;
}

// HTTPTransportServer Implementation
HTTPTransportServer::HTTPTransportServer(HTTPTransportOptions InOptions) : m_Options(std::move(InOptions))
{
	SetState(ETransportState::Disconnected);
}

HTTPTransportServer::~HTTPTransportServer()
{
	if (GetState() != ETransportState::Disconnected)
	{
		try
		{
			HTTPTransportServer::Disconnect();
		}
		catch (...)
		{
			// Ignore errors during destruction
		}
	}
}

void HTTPTransportServer::Connect()
{
	if (GetState() != ETransportState::Disconnected)
	{
		HandleRuntimeError("Transport already started");
		return;
	}

	LogMessage("Connecting...");
	try
	{
		SetState(ETransportState::Connecting);

		LogMessage("Make HTTP Server...");
		m_ServerSocket = std::make_unique<Poco::Net::ServerSocket>(m_Options.Port);
		const auto& Params = new Poco::Net::HTTPServerParams();
		const auto HandlerFactory = new MCPHTTPRequestHandlerFactory();

		LogMessage("Try Set Server For Handler Factory!");
		HandlerFactory->SetServer(this);

		m_HTTPServer = std::make_unique<Poco::Net::HTTPServer>(HandlerFactory, *m_ServerSocket, Params);

		m_HTTPServer->start();
		SetState(ETransportState::Connected);
	}
	catch (const std::exception& e)
	{
		SetState(ETransportState::Error);
		HandleRuntimeError("Failed to start HTTP server transport: " + std::string(e.what()));
	}
}

void HTTPTransportServer::Disconnect()
{
	if (GetState() == ETransportState::Disconnected)
	{
		return;
	}

	try
	{
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

		m_HTTPServer.reset();
		m_ServerSocket.reset();

		SetState(ETransportState::Disconnected);
	}
	catch (const std::exception& Except)
	{
		SetState(ETransportState::Error);
		HandleRuntimeError("Error stopping HTTP server transport: " + std::string(Except.what()));
	}
}

std::string HTTPTransportServer::GetConnectionInfo() const
{
	const std::string Protocol = m_Options.UseHTTPS ? "https" : "http";
	return Protocol + "://127.0.0.1:" + std::to_string(m_Options.Port) + m_Options.Path;
}

void HTTPTransportServer::HandleHTTPRequest(Poco::Net::HTTPServerRequest& InRequest,
	Poco::Net::HTTPServerResponse& InResponse)
{
	LogMessage("HTTP Request Received");
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

			const std::string ClientID = GenerateUUID();
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

void HTTPTransportServer::TransmitMessage(const JSONData& InMessage,
	const std::optional<std::vector<ConnectionID>>& InConnectionIDs)
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
				*Iterator->second->Stream << MessageStr;
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
std::unique_ptr<ITransport> CreateHTTPServerTransportImpl(const HTTPTransportOptions& InOptions)
{
	return std::make_unique<HTTPTransportServer>(InOptions);
}

std::unique_ptr<ITransport> CreateHTTPClientTransportImpl(const HTTPTransportOptions& InOptions)
{
	return std::make_unique<HTTPTransportClient>(InOptions);
}

MCP_NAMESPACE_END