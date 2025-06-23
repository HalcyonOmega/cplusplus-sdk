# MCP C++ SDK

**Status**: Updated for MCP Specification 2025-03-26 Compliance

## Usage

The SDK provides both simple and advanced APIs:

```cpp
// Simple API
auto server = SimpleMCPServer::CreateHTTP(8080);
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

## Dependencies

- **C++20 Compiler**: Coroutines, concepts, ranges support

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
- **Location**: `Source/Public/Utilities/ThirdParty/json.hpp`

## MCP Specification Compliance

This implementation targets **MCP Specification 2025-03-26** with:
- ✅ JSON-RPC 2.0 message handling
- ✅ StreamableHTTP transport with GET /message endpoint
- ✅ Resource pagination with cursor support
- ✅ Standard error code handling
- ✅ JSON schema validation for tools
- ✅ Progress reporting and cancellation
- ✅ Resource subscriptions and notifications

## Overview

This SDK provides both simple and advanced APIs for implementing MCP clients and servers. It supports all MCP features including tools, prompts, resources, sampling, and real-time communication over Stdio and StreamableHTTP transports.

## Features

- ✅ **Complete MCP 2024-11-05 Specification Coverage**
- ✅ **Modern C++20 Coroutines** - No callbacks, clean async/await syntax
- ✅ **Dual Transport Support** - Stdio and StreamableHTTP
- ✅ **Client & Server Implementations** - Full bidirectional communication
- ✅ **Strongly Typed Messages** - JSON-RPC with C++ type safety
- ✅ **Two-Tier API Design** - Simple for beginners, advanced for power users
- ✅ **Progress Reporting & Cancellation** - Full lifecycle management
- ✅ **Resource Subscriptions** - Real-time resource updates
- ✅ **Sampling Requests** - LLM integration support
- ✅ **Thread-Safe Operations** - Safe for concurrent use
- ✅ **Comprehensive Error Handling** - Robust error propagation
- ✅ **Schema Validation** - JSON Schema support for tools/prompts

## Quick Start

### Simple Server

```cpp
#include "MCPSDK.h"

MCPTaskVoid RunServer()
{
    auto server = SimpleMCPServer::CreateStdio();
    
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
    auto client = SimpleMCPClient::CreateStdio("my-mcp-server", {"--config", "dev"});
    
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
    auto server = SimpleMCPServer::CreateHTTP(8080);
    
    // Add your tools, prompts, and resources...
    
    co_await server->Start();
    std::cout << "MCP Server running on http://localhost:8080/mcp" << std::endl;
    
    // Server continues running...
    co_await server->Stop();
}
```

## Architecture

The SDK follows a layered architecture:

```
┌─────────────────────────────────┐
│           User Code             │
├─────────────────────────────────┤
│        SimpleMCP API            │ ← Easy to use, minimal setup
│    (SimpleMCPServer/Client)     │
├─────────────────────────────────┤
│       MCPProtocol Layer         │ ← Full MCP specification
│     (MCPServer/MCPClient)       │
├─────────────────────────────────┤
│      Transport Abstraction      │ ← Pluggable transports
│        (ITransport)             │
├─────────────────────────────────┤
│   StdioTransport │ HTTPTransport│ ← Concrete implementations
├─────────────────────────────────┤
│      Poco Libraries             │ ← Networking & process
│      nlohmann/json              │   management
└─────────────────────────────────┘
```

## Advanced Usage

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
MCPTaskVoid SubscribeToLogs(SimpleMCPClient& client)
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

## Message Flow

The SDK handles the complete MCP message lifecycle:

1. **User Action** → SimpleMCP API
2. **API Call** → MCPProtocol layer 
3. **Protocol** → Serializes to JSON-RPC → ITransport
4. **Transport** → Sends over Stdio/HTTP → Remote endpoint
5. **Remote** → Processes → Sends response
6. **Transport** → Receives response → MCPProtocol
7. **Protocol** → Deserializes → SimpleMCP API
8. **API** → Returns result → User code

## Transport Details

### Stdio Transport
- Communicates over stdin/stdout with external processes
- Supports both client (launching process) and server (being launched) modes
- Line-delimited JSON messages
- Automatic process lifecycle management

### StreamableHTTP Transport
- Client: Sends requests via HTTP POST, receives responses via Server-Sent Events
- Server: HTTP server with POST endpoint for requests, SSE for responses/notifications
- Supports multiple concurrent clients
- Automatic connection management and failover

## Thread Safety

All SDK components are thread-safe:
- Transport operations use internal mutexes
- Protocol handlers can be called from multiple threads
- SimpleMCP API is safe for concurrent access
- Coroutines handle async operations without blocking

## Error Handling

The SDK provides comprehensive error handling:
- Transport errors (connection failures, timeouts)
- Protocol errors (malformed messages, unsupported methods)
- Application errors (tool execution failures, resource access issues)
- All errors propagate through the coroutine system with proper exception handling

## Building

```cmake
find_package(Poco REQUIRED COMPONENTS Foundation Net)
target_link_libraries(your_target PRIVATE Poco::Foundation Poco::Net)
target_compile_features(your_target PRIVATE cxx_std_20)
```

### Prerequisites
- CMake 3.10 or higher
- C++20 compatible compiler
- Git

### Build Steps
1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/mcp-sdk.git
   cd mcp-sdk
   ```

2. Create a build directory:
   ```bash
   mkdir build
   cd build
   ```

3. Configure and build:
   ```bash
   cmake ..
   make
   ```

4. Install (optional):
   ```bash
   make install
   ```

## Integration Examples

### With Existing Applications

```cpp
class MyApplicationServer
{
private:
    std::unique_ptr<SimpleMCPServer> m_MCPServer;
    
public:
    void Initialize()
    {
        m_MCPServer = SimpleMCPServer::CreateHTTP(9001);
        
        // Expose application functions as MCP tools
        m_MCPServer->AddTool(CreateTool("get_user", "Get user info",
            [this](const auto& args) -> ToolResult {
                auto userInfo = GetUserFromDatabase(args.at("id"));
                return {{userInfo.ToJSON()}, false, ""};
            }));
        
        // Expose application data as MCP resources
        m_MCPServer->AddResource(CreateResource("app://config", "Configuration",
            [this]() -> ResourceContent {
                return {"app://config", GetConfigAsJSON(), "application/json"};
            }));
    }
    
    MCPTaskVoid Start()
    {
        co_await m_MCPServer->Start();
        m_MCPServer->LogInfo("MCP interface ready");
    }
};
```

## License

This project is licensed under the MIT License. See LICENSE.md for details.

## Contributing

This is an implementation of the MCP specification. For issues or enhancements, please refer to the MCP specification documentation. 