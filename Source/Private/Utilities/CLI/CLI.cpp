#include "Utilities/CLI/CLI.h"

// TODO: Implement WebSocket
class WebSocket;

MCP_NAMESPACE_BEGIN

future<void> CLI::RunClient(const string& url_or_command, const vector<string>& args) {
    return async(launch::async, [url_or_command, args]() {
        try {
            // Create client with info and capabilities
            ClientInfo client_info{"mcp-cpp test client", "0.1.0"};

            ClientCapabilities capabilities{};
            capabilities.sampling = true;

            auto client = make_unique<Client>(client_info, capabilities);

            unique_ptr<ClientTransport> client_transport;

            // Determine transport type based on URL or command
            if (CLI::IsValidUrl(url_or_command)) {
                string protocol = CLI::GetUrlProtocol(url_or_command);

                if (protocol == "http:" || protocol == "https:") {
                    client_transport = make_unique<SSEClientTransport>(url_or_command);
                } else if (protocol == "ws:" || protocol == "wss:") {
                    client_transport = make_unique<WebSocketClientTransport>(url_or_command);
                } else {
                    throw runtime_error("Unsupported URL protocol: " + protocol);
                }
            } else {
                client_transport = make_unique<StdioClientTransport>(url_or_command, args);
            }

            CLI::LogMessage("Connected to server.");

            // Connect to server
            client->Connect(*client_transport).wait();
            CLI::LogMessage("Initialized.");

            // Make a test request
            ListResourcesRequest request;
            client->Request(request).wait();

            // Clean shutdown
            client->Close().wait();
            CLI::LogMessage("Closed.");

        } catch (const exception& e) {
            CLI::LogError("Client error: " + string(e.what()));
            throw;
        }
    });
}

future<void> CLI::RunServer(int port) {
    return async(launch::async, [port]() {
        try {
            if (port != -1) {
                // HTTP/SSE server mode
                CLI::LogMessage("Starting HTTP server on port " + to_string(port));

                CLI::http_server_ = make_shared<HttpServer>();

                // Setup SSE endpoint
                CLI::http_server_->Get("/sse", [](HTTP_Request& req, HTTP_Response& res) {
                    CLI::HandleSSEConnection(req, res);
                });

                // Setup message endpoint
                CLI::http_server_->Post("/message", [](HTTP_Request& req, HTTP_Response& res) {
                    CLI::HandlePostMessage(req, res);
                });

                CLI::http_server_->Listen(port);
                CLI::LogMessage("Server running on http://localhost:" + to_string(port) + "/sse");

            } else {
                // Stdio server mode
                ServerInfo server_info{"mcp-cpp test server", "0.1.0"};

                ServerCapabilities capabilities{};
                capabilities.prompts = true;
                capabilities.resources = true;
                capabilities.tools = true;
                capabilities.logging = true;

                auto server = make_shared<MCP::Server>(server_info, capabilities);
                auto transport = make_unique<MCP::StdioServerTransport>();

                server->Connect(*transport).wait();
                CLI::LogMessage("Server running on stdio");

                // Keep server alive
                server->WaitForClose().wait();
            }

        } catch (const exception& e) {
            CLI::LogError("Server error: " + string(e.what()));
            throw;
        }
    });
}

inline int CLI::Run(int argc, char* argv[]) {
    if (argc < 2) {
        PrintUsage();
        return 1;
    }

    vector<string> args;
    for (int i = 1; i < argc; ++i) { args.emplace_back(argv[i]); }

    const string& command = args[0];

    try {
        if (command == "client") {
            if (args.size() < 2) {
                LogError("Usage: client <server_url_or_command> [args...]");
                return 1;
            }

            vector<string> client_args(args.begin() + 2, args.end());
            CLI_Implementation::RunClientAsync(args[1], client_args).wait();

        } else if (command == "server") {
            int port = -1; // default to stdio
            if (args.size() > 1) {
                try {
                    port = stoi(args[1]);
                } catch (const exception&) {
                    LogError("Invalid port number: " + args[1]);
                    return 1;
                }
            }

            CLI_Implementation::RunServerAsync(port).wait();

        } else {
            LogError("Unrecognized command: " + command);
            PrintUsage();
            return 1;
        }

    } catch (const exception& e) {
        LogError("Error: " + string(e.what()));
        return 1;
    }

    return 0;
}

inline bool CLI::IsValidUrl(const string& url) {
    // Simple URL validation - check for protocol
    return url.find("://") != string::npos;
}

inline string CLI::GetUrlProtocol(const string& url) {
    size_t pos = url.find("://");
    if (pos != string::npos) { return url.substr(0, pos + 1); }
    return MSG_NULL;
}

inline void CLI::PrintUsage() {
    cout << "Usage:\n";
    cout << "  client <server_url_or_command> [args...]  - Run as MCP client\n";
    cout << "  server [port]                            - Run as MCP server "
            "(default: stdio)\n";
}

inline void CLI::LogMessage(const string& message) {
    cout << message << endl;
}

inline void CLI::LogError(const string& error) {
    cerr << error << endl;
}

inline void CLI::HandleSSEConnection(HTTP_Request& req, HTTP_Response& res) {
    LogMessage("Got new SSE connection");

    auto transport = make_shared<MCP::SSEServerTransport>("/message", res);

    MCP::ServerInfo server_info{"mcp-cpp test server", "0.1.0"};

    MCP::ServerCapabilities capabilities{};

    auto server = make_shared<MCP::Server>(server_info, capabilities);
    active_servers_.push_back(server);

    server->SetCloseCallback([server]() {
        LogMessage("SSE connection closed");
        auto& servers = CLI::active_servers_;
        servers.erase(remove(servers.begin(), servers.end(), server), servers.end());
    });

    server->Connect(*transport);
}

inline void CLI::HandlePostMessage(HTTP_Request& req, HTTP_Response& res) {
    LogMessage("Received message");

    string session_id = req.GetQueryParameter("SessionID");

    // Find transport by session ID
    shared_ptr<MCP::SSEServerTransport> transport = nullptr;
    for (const auto& server : active_servers_) {
        auto sse_transport = dynamic_pointer_cast<MCP::SSEServerTransport>(server->GetTransport());
        if (sse_transport && sse_transport->GetSessionID() == session_id) {
            transport = sse_transport;
            break;
        }
    }

    if (!transport) {
        res.SetStatus(HTTPStatus::NotFound);
        res.Send("Session not found");
        return;
    }

    transport->HandlePostMessage(req, res);
}

MCP_NAMESPACE_END