#pragma once

#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Runnable.h>

#include <chrono>
#include <future>
#include <thread>
#include <unordered_map>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Transport/ITransport.h"

// Forward declarations
class MCPHTTPRequestHandlerFactory;

MCP_NAMESPACE_BEGIN

// HTTP Client Transport
class HTTPTransportClient final : public ITransport,
								  public Poco::Runnable
{
public:
	explicit HTTPTransportClient(HTTPTransportOptions InOptions);
	~HTTPTransportClient() noexcept override;
	HTTPTransportClient(const HTTPTransportClient&) noexcept = delete;
	HTTPTransportClient(HTTPTransportClient&&) noexcept = delete;
	HTTPTransportClient& operator=(const HTTPTransportClient&) noexcept = delete;
	HTTPTransportClient& operator=(HTTPTransportClient&&) noexcept = delete;

	// ITransport interface
	void Connect() override;
	void Disconnect() override;

	void TransmitMessage(const JSONData& InMessage,
		const std::optional<std::vector<ConnectionID>>& InConnectionIDs) override;

	[[nodiscard]] std::string GetConnectionInfo() const override;

protected:
	// Poco::Runnable interface for SSE reading
	void run() override;

private:
	VoidTask ConnectToServer();
	void StartSSEConnection();
	void ProcessSSELine(const std::string& InLine);
	void Cleanup();

	HTTPTransportOptions m_Options;
	std::unique_ptr<Poco::Net::HTTPClientSession> m_HTTPSession;
	std::unique_ptr<std::istream> m_SSEStream;

	std::jthread m_SSEThread;
	std::atomic<bool> m_ShouldStop{ false };
	std::string m_SSEBuffer;

	mutable std::mutex m_ConnectionMutex;
};

// HTTP Server Transport
class HTTPTransportServer : public ITransport
{
public:
	explicit HTTPTransportServer(HTTPTransportOptions InOptions);
	~HTTPTransportServer() noexcept override;

	// ITransport interface
	void Connect() override;
	void Disconnect() override;

	void TransmitMessage(const JSONData& InMessage,
		const std::optional<std::vector<ConnectionID>>& InConnectionIDs) override;

	[[nodiscard]] std::string GetConnectionInfo() const override;

	// Server-specific methods
	void HandleHTTPRequest(Poco::Net::HTTPServerRequest& InRequest, Poco::Net::HTTPServerResponse& InResponse);
	VoidTask HandleGetMessageEndpoint(const Poco::Net::HTTPServerRequest& InRequest,
		Poco::Net::HTTPServerResponse& InResponse);
	void RegisterSSEClient(const std::string& InClientID, Poco::Net::HTTPServerResponse& InResponse);
	void UnregisterSSEClient(const std::string& InClientID);
	VoidTask StreamMessagesToClient(const std::string& InClientID);

private:
	void ProcessReceivedMessage(const std::string& InMessage);

	HTTPTransportOptions m_Options;
	std::unique_ptr<Poco::Net::HTTPServer> m_HTTPServer;
	std::unique_ptr<Poco::Net::ServerSocket> m_ServerSocket;

	// SSE client management
	struct SSEClient
	{
		std::string ClientID;
		Poco::Net::HTTPServerResponse* Response{};
		std::ostream* Stream{};
		std::chrono::steady_clock::time_point ConnectedTime;
		bool IsActive{ false };
	};

	std::unordered_map<std::string, std::unique_ptr<SSEClient>> m_SSEClients;
	mutable std::mutex m_ClientsMutex;
};

// HTTP Server Transport Request Handler
class MCPHTTPRequestHandler final : public Poco::Net::HTTPRequestHandler
{
public:
	explicit MCPHTTPRequestHandler(const Poco::Net::HTTPServerRequest& InResponse, HTTPTransportServer* InServer);
	void handleRequest(Poco::Net::HTTPServerRequest& InRequest, Poco::Net::HTTPServerResponse& InResponse) override;

private:
	HTTPTransportServer* m_Server;
};

// HTTP Server Transport Request Handler Factory
class MCPHTTPRequestHandlerFactory final : public Poco::Net::HTTPRequestHandlerFactory
{
public:
	explicit MCPHTTPRequestHandlerFactory();
	[[nodiscard]] Poco::Net::HTTPRequestHandler* createRequestHandler(
		const Poco::Net::HTTPServerRequest& InRequest) override;
	bool SetServer(HTTPTransportServer* InServer);

private:
	HTTPTransportServer* m_Server{ nullptr };
};

MCP_NAMESPACE_END