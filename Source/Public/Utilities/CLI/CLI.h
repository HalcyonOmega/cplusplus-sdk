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
    static future<void> RunClient(const string& url_or_command,
                                  const vector<string>& args);

    // Server functionality
    static future<void> RunServer(int port); // port = -1 for stdio

    // Utility functions
    static bool IsValidUrl(const string& url);
    static string GetUrlProtocol(const string& url);
    static void PrintUsage();
    static void LogMessage(const string& message);
    static void LogError(const string& error);

    // HTTP server handlers
    static void HandleSSEConnection(HttpRequest& req, HttpResponse& res);
    static void HandlePostMessage(HttpRequest& req, HttpResponse& res);

    // Static members for server state
    static vector<shared_ptr<MCP::Server>> active_servers_;
    static shared_ptr<HttpServer> http_server_;
};