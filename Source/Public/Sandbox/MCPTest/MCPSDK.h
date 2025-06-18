#pragma once

// MCP SDK - Complete Model Context Protocol Implementation
// Version: 1.0.0
// Specification: 2024-11-05

// Core components
#include "MCPMessages.h"
#include "MCPTask.h"
#include "MCPTypes.h"

// Transport layer
#include "HTTPTransport.h"
#include "ITransport.h"
#include "StdioTransport.h"

// Protocol layer
#include "MCPProtocol.h"

// User-friendly API
#include "SimpleMCPAPI.h"

/*
USAGE EXAMPLES:

1. Simple Server Example:
```cpp
#include "MCPSDK.h"

MCPTaskVoid RunSimpleServer()
{
    auto server = SimpleMCPServer::CreateStdio();

    // Add a simple tool
    server->AddTool(CreateTool("greet", "Greets a person",
        [](const auto& args) -> ToolResult {
            std::string name = args.contains("name") ?
                args.at("name").get<std::string>() : "World";
            return {{"Hello, " + name + "!"}, false, ""};
        }));

    // Add a simple resource
    server->AddResource(CreateResource("file:///config.txt", "Configuration",
        []() -> ResourceContent {
            return {"file:///config.txt", "key=value\nother=data", "text/plain"};
        }));

    co_await server->Start();

    // Server runs until stopped
    // In practice, you'd wait for a signal or condition
    co_await server->Stop();
}
```

2. Simple Client Example:
```cpp
#include "MCPSDK.h"

MCPTaskVoid RunSimpleClient()
{
    auto client = SimpleMCPClient::CreateStdio("mcp-server", {"--arg1", "value1"});

    bool connected = co_await client->Connect();
    if (!connected) {
        co_return;
    }

    // List available tools
    auto tools = co_await client->ListTools();
    for (const auto& tool : tools) {
        std::cout << "Tool: " << tool << std::endl;
    }

    // Call a tool
    std::unordered_map<std::string, nlohmann::json> args;
    args["name"] = "Alice";
    auto result = co_await client->CallTool("greet", args);

    if (!result.IsError) {
        for (const auto& output : result.TextOutputs) {
            std::cout << "Result: " << output << std::endl;
        }
    }

    co_await client->Disconnect();
}
```

3. HTTP Server Example:
```cpp
#include "MCPSDK.h"

MCPTaskVoid RunHTTPServer()
{
    auto server = SimpleMCPServer::CreateHTTP(8080);

    // Add tools, prompts, resources...

    co_await server->Start();
    std::cout << "MCP Server running on http://localhost:8080/mcp" << std::endl;

    // Wait for shutdown signal
    // ...

    co_await server->Stop();
}
```

4. Advanced Protocol Usage:
```cpp
#include "MCPSDK.h"

MCPTaskVoid RunAdvancedServer()
{
    auto transport = TransportFactory::CreateStdioTransport({});

    Implementation serverInfo{"MyAdvancedServer", "1.0.0"};
    ServerCapabilities capabilities;
    capabilities.Tools = ToolsCapability{true}; // Support tool list changes
    capabilities.Resources = ResourcesCapability{true, true}; // Support subscriptions

    auto protocol = std::make_unique<MCPServer>(std::move(transport), serverInfo, capabilities);

    // Register tool with full control
    Tool myTool;
    myTool.Name = "advanced_tool";
    myTool.Description = "An advanced tool with schema validation";
    myTool.InputSchema.Type = "object";
    myTool.InputSchema.Properties = {
        {"input", nlohmann::json{{"type", "string"}, {"description", "Input text"}}}
    };
    myTool.InputSchema.Required = {"input"};

    protocol->RegisterTool(myTool,
        [](const auto& args) -> MCPTask<CallToolResult> {
            CallToolResult result;
            result.CallContent = {TextContent{"text", "Processed: " +
args.at("input").get<std::string>()}}; result.IsError = false; co_return result;
        });

    co_await protocol->Start();

    // Protocol handles all MCP communication
    // Server continues running...
}
```

FEATURES:
- ✅ Complete MCP 2024-11-05 specification coverage
- ✅ Modern C++20 coroutines for async operations
- ✅ Both Stdio and StreamableHTTP transports
- ✅ Client and Server implementations
- ✅ Strongly typed message system
- ✅ Simple API for beginners
- ✅ Advanced API for full control
- ✅ JSON schema validation
- ✅ Progress reporting and cancellation
- ✅ Resource subscriptions
- ✅ Sampling requests
- ✅ Comprehensive error handling
- ✅ Thread-safe operations
- ✅ Poco library integration for networking
- ✅ nlohmann/json for JSON handling

ARCHITECTURE:
User Code
    ↓
SimpleMCP API (Easy to use)
    ↓
MCPProtocol (Full MCP spec)
    ↓
ITransport (Abstraction)
    ↓
StdioTransport | HTTPTransport
    ↓
Poco | System APIs
*/