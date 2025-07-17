#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Util/ServerApplication.h>

#include <iostream>

#include "CoreSDK/Common/Capabilities.h"
#include "CoreSDK/Common/Implementation.h"
#include "CoreSDK/Core/MCPServer.h"
#include "CoreSDK/Messages/MCPMessages.h"
#include "CoreSDK/Transport/HTTPTransport.h"

// --- Request Handler ---
// This class is responsible for processing an incoming HTTP request
// and generating a response. A new instance is created for each request.
class MyRequestHandler : public Poco::Net::HTTPRequestHandler
{
public:
	void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) override
	{

		std::cout << "Request received from " << request.clientAddress().toString() << std::endl;
		std::cout << "URI: " << request.getURI() << std::endl;

		// Set the response headers
		response.setStatus(Poco::Net::HTTPResponse::HTTP_OK); // 200 OK
		response.setContentType("text/plain");

		// Get the output stream and write the response body
		std::ostream& out = response.send();
		out << "Hello from Poco Server!";
		out.flush();
	}
};

// --- Request Handler Factory ---
// This factory is responsible for creating new request handlers.
// The HTTPServer uses this to get a handler for each new connection.
class MyRequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory
{
public:
	Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest& request) override
	{
		return new MyRequestHandler;
	}
};

// --- Server Application ---
// This is the main class for our server application. It inherits from
// Poco::Util::ServerApplication to get basic server infrastructure for free.
class MyServerApp : public Poco::Util::ServerApplication
{
protected:
	int main(const std::vector<std::string>& args) override
	{
		unsigned short port = 8080;

		// Create a server socket
		Poco::Net::ServerSocket svs(port);

		// Create the HTTP server
		Poco::Net::HTTPServer srv(new MyRequestHandlerFactory, svs, new Poco::Net::HTTPServerParams);

		std::cout << "🚀 Server started on port " << port << std::endl;

		// Start the server
		srv.start();

		// Wait for the server to be shut down
		waitForTerminationRequest();

		std::cout << "🛑 Shutting down server..." << std::endl;

		// Stop the server
		srv.stop();

		return Application::EXIT_OK;
	}
};

void ShowMenu()
{
	std::cout << "\n=== MCP Transport Test ===" << std::endl;
	std::cout << "1. Test STDIO Transport" << std::endl;
	std::cout << "2. Test HTTP Transport" << std::endl;
	std::cout << "3. Exit" << std::endl;
	std::cout << "Choice: ";
}

int main(int argc, char** argv)
{
	std::cout << "=== MCP HTTP Server Demo ===" << std::endl;
	//
	// MCP::MCPServer Server{ MCP::ETransportType::StreamableHTTP,
	// 	std::make_unique<MCP::HTTPTransportOptions>(),
	// 	MCP::Implementation{ "MCP HTTP Server", "V1.0.0", MCP::EProtocolVersion::V2025_03_26 },
	// 	MCP::ServerCapabilities{} };

	// Server.Start();

	int choice;
	while (true)
	{
		ShowMenu();
		std::cin >> choice;

		switch (choice)
		{
			case 1:
				std::cout << "Stdio Testing..." << std::endl;
				break;
			case 3:
				std::cout << "Exiting..." << std::endl;
				// Server.Stop();
				return 0;
			default:
				std::cout << "Invalid choice!" << std::endl;
				break;
			case 2:
				std::cout << "HTTP Testing..." << std::endl;
				MyServerApp app;
				return app.run(argc, argv);
		}
	}
}