#pragma once

#include "Core.h"

// TODO: Implement WebSocket
class WebSocket;

using namespace std;

class CLI {
  public:
    CLI() = default;
    ~CLI() = default;

    // Main entry point
    static int Run(int argc, char* argv[]);

  private:
    // Client functionality
    static future<void> RunClient(const string& InURLOrCommand, const vector<string>& InArgs);

    // Server functionality
    static future<void> RunServer(int InPort); // port = -1 for stdio

    // Utility functions
    static bool IsValidURL(const string& InURL);
    static string GetURLProtocol(const string& InURL);
    static void PrintUsage();
    static void LogMessage(const string& InMessage);
    static void LogError(const string& InError);

    // HTTP server handlers
    static void HandleSSEConnection(HTTP::Request& InRequest, HTTP::Response& InResponse);
    static void HandlePostMessage(HTTP::Request& InRequest, HTTP::Response& InResponse);

    // Static members for server state
    static vector<shared_ptr<MCP::Server>> m_ActiveServers;
    static shared_ptr<HTTP::Server> m_HTTPServer;
};