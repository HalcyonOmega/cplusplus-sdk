#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Util/ServerApplication.h>

#include <iostream>

#include "CoreSDK/Common/Capabilities.h"
#include "CoreSDK/Common/Implementation.h"
#include "CoreSDK/Core/MCPServer.h"
#include "CoreSDK/Messages/MCPMessages.h"
#include "CoreSDK/Transport/HTTPTransport.h"

class MCPHTTPServerApp : public Poco::Util::ServerApplication
{
protected:
	int main(const std::vector<std::string>& args) override
	{
		MCP::MCPServer Server{ MCP::ETransportType::StreamableHTTP,
			std::make_unique<MCP::HTTPTransportOptions>(),
			MCP::Implementation{ "MCP HTTP Server", "V1.0.0", MCP::EProtocolVersion::V2025_03_26 },
			MCP::ServerCapabilities{} };

		Server.Start();

		MCP::PingRequest pingRequest;
		MCP::JSONData Json = pingRequest;
		std::cout << "\n PingRequest JSON:" << std::endl;
		std::cout << Json.dump(2) << std::endl;

		waitForTerminationRequest();

		Server.Stop();
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
				return 0;
			default:
				std::cout << "Invalid choice!" << std::endl;
				break;
			case 2:
				std::cout << "HTTP Testing..." << std::endl;
				MCPHTTPServerApp app;
				return app.run(argc, argv);
		}
	}
}