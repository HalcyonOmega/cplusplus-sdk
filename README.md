# MCP C++ SDK

***Work In Progress***

## Overview

This SDK provides both simple and advanced APIs for implementing MCP clients and servers. It supports all MCP features including tools, prompts, resources, sampling, and real-time communication over Stdio and StreamableHTTP transports.

**MCP Specification Compliance Target**: 2025-03-26

## Table Of Contents
- [Prerequisites](#prerequisites)
- [Building](#building)
- [Usage](#usage)
- [Contributing](#contributing)
- [License](#license)

## Prerequisites
- CMake 3.20+
- C++20 compatible compiler

### Third-Party Libraries

#### Poco C++ Libraries
- **Version**: Latest
- **License**: Boost Software License
- **Source**: https://github.com/pocoproject/poco
- **Purpose**: Data types, networking, HTTP support, & utilities

#### nlohmann/json

- **Version**: Latest (from develop branch)
- **License**: MIT
- **Source**: https://github.com/nlohmann/json
- **Purpose**: JSON parsing and serialization

## Building

```cmake
find_package(Poco REQUIRED COMPONENTS Foundation Net)
target_link_libraries(your_target PRIVATE Poco::Foundation Poco::Net)
target_compile_features(your_target PRIVATE cxx_std_20)
```

### Build Steps
1. Clone the repository:
   ```bash
   ```

2. Create a build directory:
   ```bash
   ```

3. Configure and build:
   ```bash
   ```

4. Install (optional):
   ```bash
   ```

## Usage

The SDK provides both simple and advanced APIs:
```cpp
// Simple API
auto server = MCPServer::CreateHTTP(8080);
server->AddTool(CreateTool("example", "Example tool", [](auto args) -> ToolResult {
    return {{"Hello from tool!"}, false, ""};
}));
co_await server->Start();

// Advanced API with error codes
try {
    auto result = co_await protocol->CallTool("example", {});
} catch (const MCPException& e) {
    if (e.GetCode() == MCPErrorCodes::TOOL_NOT_FOUND) {
        // Handle tool not found
    }
}
```

#### Simple Server

```cpp
#include "MCPSDK.h"

MCPTaskVoid RunServer()
{
    auto server = MCPServer::CreateStdio();
    
    // Add a greeting tool
    server->AddTool(CreateTool("greet", "Greets a person",
        [](const auto& args) -> ToolResult {
            std::string name = args.contains("name") ? 
                args.at("name").get<std::string>() : "World";
            return {{"Hello, " + name + "!"}, false, ""};
        }));
    
    // Add a file resource
    server->AddResource(CreateResource("file:///config.txt", "Configuration",
        []() -> ResourceContent {
            return {"file:///config.txt", "setting=value", "text/plain"};
        }));
    
    co_await server->Start();
    server->LogInfo("Server started successfully");
    
    // Server runs until stopped
    co_await server->Stop();
}
```

### Simple Client

```cpp
#include "MCPSDK.h"

MCPTaskVoid RunClient()
{
    auto client = MCPClient::CreateStdio("my-mcp-server", {"--config", "dev"});
    
    bool connected = co_await client->Connect();
    if (!connected) {
        std::cerr << "Failed to connect" << std::endl;
        co_return;
    }
    
    // List available tools
    auto tools = co_await client->ListTools();
    for (const auto& tool : tools) {
        std::cout << "Available tool: " << tool << std::endl;
    }
    
    // Call a tool
    auto result = co_await client->CallTool("greet", {{"name", "Alice"}});
    if (!result.IsError) {
        for (const auto& output : result.TextOutputs) {
            std::cout << "Tool output: " << output << std::endl;
        }
    }
    
    co_await client->Disconnect();
}
```

### HTTP Server

```cpp
#include "MCPSDK.h"

MCPTaskVoid RunHTTPServer()
{
    auto server = MCPServer::CreateHTTP(8080);
    
    // Add your tools, prompts, and resources...
    
    co_await server->Start();
    std::cout << "MCP Server running on http://localhost:8080/mcp" << std::endl;
    
    // Server continues running...
    co_await server->Stop();
}
```

### Custom Tool with Schema Validation

```cpp
#include "MCPSDK.h"

MCPTaskVoid RunAdvancedServer()
{
    auto transport = TransportFactory::CreateStdioTransport({});
    
    Implementation serverInfo{"AdvancedServer", "1.0.0"};
    ServerCapabilities capabilities;
    capabilities.Tools = ToolsCapability{true}; // Support notifications
    
    auto server = std::make_unique<MCPServer>(
        std::move(transport), serverInfo, capabilities);
    
    // Define a tool with complete schema
    Tool calculatorTool;
    calculatorTool.Name = "calculate";
    calculatorTool.Description = "Performs mathematical calculations";
    calculatorTool.InputSchema.Type = "object";
    calculatorTool.InputSchema.Properties = {
        {"expression", nlohmann::json{
            {"type", "string"},
            {"description", "Mathematical expression to evaluate"},
            {"pattern", "^[0-9+\\-*/().\\s]+$"}
        }}
    };
    calculatorTool.InputSchema.Required = {"expression"};
    
    server->RegisterTool(calculatorTool, 
        [](const auto& args) -> MCPTask<CallToolResult> {
            std::string expr = args.at("expression").get<std::string>();
            
            // Evaluate expression (implementation details omitted)
            double result = EvaluateExpression(expr);
            
            CallToolResult toolResult;
            toolResult.CallContent = {
                TextContent{"text", "Result: " + std::to_string(result)}
            };
            toolResult.IsError = false;
            
            co_return toolResult;
        });
    
    co_await server->Start();
    
    // Server handles all MCP protocol automatically
}
```

### Resource with Subscriptions

```cpp
// Server side - resource that changes over time
void SetupDynamicResource(MCPServer& server)
{
    Resource logResource;
    logResource.URI = "log://application/events";
    logResource.Name = "Application Events";
    logResource.Description = "Real-time application event log";
    logResource.MimeType = "text/plain";
    
    server.RegisterResource(logResource, 
        []() -> MCPTask<ReadResourceResult> {
            ReadResourceResult result;
            result.Contents = {
                TextContent{"text", GetCurrentLogContent()}
            };
            co_return result;
        });
    
    // Notify subscribers when log updates
    std::thread([&server]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            server.NotifyResourceUpdated("log://application/events");
        }
    }).detach();
}

// Client side - subscribe to resource updates
MCPTaskVoid SubscribeToLogs(MCPClient& client)
{
    // Subscribe to log updates
    co_await client.SubscribeToResource("log://application/events");
    
    // Handle updates via event handler
    client.OnResourceUpdated([&client](const std::string& uri) {
        if (uri == "log://application/events") {
            // Re-read the resource to get latest content
            auto content = client.ReadResource(uri);
            // Process updated content...
        }
    });
}
```

### Sampling Integration

```cpp
// Server requesting LLM sampling from client
MCPTaskVoid HandleComplexQuery(MCPServer& server, const std::string& query)
{
    // Prepare messages for LLM
    std::vector<SamplingMessage> messages;
    messages.push_back({
        Role::User, 
        TextContent{"text", "Analyze this query: " + query}
    });
    
    // Request sampling from client
    auto result = co_await server.RequestSampling(
        messages,
        1000, // max tokens
        "You are a helpful analysis assistant",
        "thisServer", // include this server's context
        0.7 // temperature
    );
    
    // Process LLM response
    std::cout << "LLM Analysis: " << result.ResponseContent.Text << std::endl;
}
```

## Contributing

This is an implementation of the MCP specification. For issues or enhancements, please refer to the MCP specification documentation. 

## License

This project is licensed under the MIT License. See LICENSE.md for details.

