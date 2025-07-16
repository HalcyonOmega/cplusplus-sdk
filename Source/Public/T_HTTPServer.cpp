#include <iostream>

#include "CoreSDK/Common/Capabilities.h"
#include "CoreSDK/Common/Implementation.h"
#include "CoreSDK/Core/MCPServer.h"
#include "CoreSDK/Messages/MCPMessages.h"
#include "CoreSDK/Transport/HTTPTransport.h"

void ShowMenu()
{
	std::cout << "\n=== MCP Transport Test ===" << std::endl;
	std::cout << "1. Test STDIO Transport" << std::endl;
	std::cout << "2. Test HTTP Transport" << std::endl;
	std::cout << "3. Exit" << std::endl;
	std::cout << "Choice: ";
}

int main()
{
	std::cout << "=== MCP HTTP Server Demo ===" << std::endl;

	MCP::MCPServer Server{ MCP::ETransportType::StreamableHTTP,
		std::nullopt,
		MCP::Implementation{ "MCP HTTP Server", "V1.0.0", MCP::EProtocolVersion::V2025_03_26 },
		MCP::ServerCapabilities{} };

	Server.Start();

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
			case 2:
				std::cout << "HTTP Testing..." << std::endl;
				break;
			case 3:
				std::cout << "Exiting..." << std::endl;
				Server.Stop();
				return 0;
			default:
				std::cout << "Invalid choice!" << std::endl;
				break;
		}
	}
}