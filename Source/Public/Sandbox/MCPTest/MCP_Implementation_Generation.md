# MCP C++ SDK Implementation Generation Prompt

## Task Overview
You are an expert C++ developer tasked with implementing the complete functionality for a Model Context Protocol (MCP) SDK based on the provided framework declarations. Your goal is to create production-ready implementations that are fully compliant with the MCP specification (2025-03-26), memory-safe, thread-safe, and performant.

## Framework Base
The framework consists of header-only declarations in `Source/Public/Sandbox/MCPTest/`:
- `MCPTask.h` - C++20 coroutine task system (COMPLETE - no implementation needed)
- `MCPTypes.h` - Core MCP data structures and types (COMPLETE - no implementation needed)  
- `MCPMessages.h` - All MCP message types and structures (COMPLETE - no implementation needed)
- `ITransport.h` - Transport abstraction layer (INTERFACE ONLY - needs implementation)
- `StdioTransport.h` - Stdio transport (DECLARATIONS ONLY - needs implementation)
- `HTTPTransport.h` - StreamableHTTP transport (DECLARATIONS ONLY - needs implementation)
- `MCPProtocol.h` - Core protocol handlers (DECLARATIONS ONLY - needs implementation)
- `SimpleMCPAPI.h` - User-friendly wrapper API (DECLARATIONS ONLY - needs implementation)

## Implementation Requirements

### 1. Code Generation Guidelines
- **Language**: Modern C++20 with coroutines, concepts, and ranges
- **Libraries**: Only Poco C++ Libraries and nlohmann/json
- **Memory Management**: Use RAII, smart pointers, no raw pointers for ownership
- **Error Handling**: Exception-safe code with proper error propagation
- **Thread Safety**: All public APIs must be thread-safe
- **Performance**: Optimize for low latency and high throughput
- **Maintainability**: Clean, readable, well-documented code

### 2. Dependencies to Use
```cpp
// Poco Libraries (allowed)
#include <Poco/Process.h>
#include <Poco/Pipe.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Thread.h>
#include <Poco/Mutex.h>
#include <Poco/Event.h>

// Standard Library (preferred for core functionality)
#include <coroutine>
#include <memory>
#include <atomic>
#include <mutex>
#include <future>
#include <unordered_map>
#include <vector>
#include <string>
#include <optional>
#include <functional>

// nlohmann JSON (allowed)
#include "json.hpp"
```

### 3. Naming Conventions (CRITICAL)
- **PascalCase** for everything (classes, functions, variables, files)
- **Acronyms stay ALL CAPS**: `HTTP`, `JSON`, `API`, `MCP`, `XML`, `RPC`
- **Private/Protected member variables**: Use `m_` prefix
- **Method parameters**: `In` prefix for inputs, `Out` prefix for outputs
- **No snake_case, camelCase, or kebab-case**

Example:
```cpp
class HTTPTransportClient {
private:
    std::string m_ServerURL;
    std::atomic<bool> m_IsConnected;
    
public:
    MCPTask<void> ConnectAsync(const std::string& InURL);
    void ProcessResponseSync(const JSONRPCResponse& InResponse, std::string& OutResult);
};
```

### 4. Implementation Priority Order

#### Phase 1: Core Transport Layer
1. **StdioTransport Implementation**
   - `StdioTransport::ConnectAsync()` - Launch process, setup pipes
   - `StdioTransport::SendMessageAsync()` - Write JSON to stdout
   - `StdioTransport::ReceiveMessageAsync()` - Read JSON from stdin
   - `StdioTransport::DisconnectAsync()` - Cleanup process and pipes
   - `StdioServerTransport` - Server-side stdio handling

2. **HTTPTransport Implementation**
   - `HTTPTransportClient::ConnectAsync()` - SSE connection setup
   - `HTTPTransportClient::SendMessageAsync()` - HTTP POST requests
   - `HTTPTransportClient::ReceiveMessageAsync()` - SSE event parsing
   - `HTTPTransportServer` - HTTP server with SSE support
   - `MCPHTTPRequestHandler` - Request processing logic

#### Phase 2: Protocol Layer
3. **MCPProtocol Base Implementation**
   - `MCPProtocol::InitializeAsync()` - Capability negotiation
   - `MCPProtocol::SendRequestAsync()` - Request/response tracking  
   - `MCPProtocol::HandleNotificationAsync()` - Notification processing
   - `MCPProtocol::ShutdownAsync()` - Graceful cleanup

4. **MCPClient Implementation**
   - `ListToolsAsync()` - Enumerate available tools
   - `CallToolAsync()` - Execute tool with parameters
   - `ListPromptsAsync()` - Get available prompts
   - `GetPromptAsync()` - Retrieve prompt with arguments
   - `ListResourcesAsync()` - Enumerate resources
   - `ReadResourceAsync()` - Fetch resource content
   - `CreateMessageAsync()` - Sampling requests

5. **MCPServer Implementation**
   - `AddTool()` - Register tool handlers
   - `AddPrompt()` - Register prompt handlers  
   - `AddResource()` - Register resource handlers
   - `HandleRequest()` - Process incoming requests
   - `SendProgress()` - Progress notifications
   - `SendLog()` - Logging output

#### Phase 3: User API Layer
6. **SimpleMCPAPI Implementation**
   - `SimpleMCPClient` - Easy-to-use client wrapper
   - `SimpleMCPServer` - Easy-to-use server wrapper
   - `CreateTool()` - Helper for tool creation
   - `CreatePrompt()` - Helper for prompt creation
   - `CreateResource()` - Helper for resource creation

### 5. Specific Implementation Requirements

#### StdioTransport Details
```cpp
// Required functionality to implement:
class StdioTransport {
private:
    Poco::Process::PID m_ProcessPID;
    std::unique_ptr<Poco::Pipe> m_StdinPipe;
    std::unique_ptr<Poco::Pipe> m_StdoutPipe;
    std::atomic<TransportState> m_State;
    mutable Poco::Mutex m_MessageMutex;
    
public:
    // Connect to process via stdio
    MCPTask<void> ConnectAsync(const StdioTransportOptions& InOptions) override;
    
    // Send JSON message via stdin
    MCPTask<void> SendMessageAsync(const nlohmann::json& InMessage) override;
    
    // Receive JSON message from stdout
    MCPTask<nlohmann::json> ReceiveMessageAsync() override;
    
    // Graceful disconnect
    MCPTask<void> DisconnectAsync() override;
};
```

#### HTTPTransport Details  
```cpp
// Required functionality to implement:
class HTTPTransportClient {
private:
    std::string m_ServerURL;
    std::unique_ptr<Poco::Net::HTTPClientSession> m_HTTPSession;
    std::atomic<bool> m_SSEConnected;
    std::thread m_SSEThread;
    std::queue<nlohmann::json> m_MessageQueue;
    mutable std::mutex m_QueueMutex;
    
public:
    // Connect via SSE for real-time communication
    MCPTask<void> ConnectAsync(const HTTPTransportOptions& InOptions) override;
    
    // Send via HTTP POST
    MCPTask<void> SendMessageAsync(const nlohmann::json& InMessage) override;
    
    // Receive via SSE event stream
    MCPTask<nlohmann::json> ReceiveMessageAsync() override;
};

class HTTPTransportServer {
private:
    std::unique_ptr<Poco::Net::HTTPServer> m_HTTPServer;
    std::vector<std::shared_ptr<SSEClient>> m_SSEClients;
    mutable std::mutex m_ClientsMutex;
    
public:
    // Start HTTP server
    MCPTask<void> StartAsync(const HTTPTransportOptions& InOptions);
    
    // Broadcast to all SSE clients
    MCPTask<void> BroadcastMessageAsync(const nlohmann::json& InMessage);
};
```

#### MCPProtocol Details
```cpp
// Required functionality to implement:
class MCPProtocol {
protected:
    std::shared_ptr<ITransport> m_Transport;
    std::unordered_map<std::string, std::promise<nlohmann::json>> m_PendingRequests;
    mutable std::mutex m_RequestsMutex;
    std::atomic<uint64_t> m_NextRequestID;
    
    // Template method for request/response handling
    template<typename TRequest, typename TResponse>
    MCPTask<TResponse> SendRequestAsync(const std::string& InMethod, const TRequest& InRequest);
    
    // Message processing loop
    MCPTask<void> ProcessMessagesAsync();
    
public:
    // Initialize connection and negotiate capabilities
    MCPTask<void> InitializeAsync(const InitializeRequest& InRequest);
    
    // Send notification (no response expected)
    MCPTask<void> SendNotificationAsync(const std::string& InMethod, const nlohmann::json& InParams);
    
    // Graceful shutdown
    MCPTask<void> ShutdownAsync();
};
```

### 6. Error Handling Strategy
- Use exceptions for unrecoverable errors
- Return `std::optional` or error codes for recoverable failures  
- Implement proper RAII for resource cleanup
- Use coroutine error propagation patterns

```cpp
// Example error handling pattern:
MCPTask<std::optional<ToolResult>> CallToolAsync(const std::string& InToolName, const nlohmann::json& InArguments) {
    try {
        auto request = CreateCallToolRequest(InToolName, InArguments);
        auto response = co_await SendRequestAsync<CallToolRequest, CallToolResponse>("tools/call", request);
        co_return ConvertToToolResult(response);
    }
    catch (const std::exception& e) {
        // Log error and return empty optional
        co_return std::nullopt;
    }
}
```

### 7. Thread Safety Requirements
- All public methods must be thread-safe
- Use appropriate synchronization primitives (mutex, atomic)
- Avoid data races in coroutine contexts
- Implement proper lifetime management

### 8. Performance Considerations
- Minimize memory allocations in hot paths
- Use move semantics wherever possible
- Implement connection pooling for HTTP transport
- Cache JSON parsing results when appropriate
- Use efficient data structures (unordered_map vs map)

### 9. Testing Integration Points
```cpp
// Provide testable interfaces:
class ITransportFactory {
public:
    virtual std::unique_ptr<ITransport> CreateStdioTransport() = 0;
    virtual std::unique_ptr<ITransport> CreateHTTPTransport() = 0;
};

// Allow dependency injection for testing
class MCPClient {
public:
    MCPClient(std::shared_ptr<ITransport> InTransport = nullptr);
    void SetTransport(std::shared_ptr<ITransport> InTransport);
};
```

## Implementation Deliverables

### Files to Create
1. **Source/Private/Communication/Transport/Stdio/StdioTransport.cpp**
   - Complete StdioTransport and StdioServerTransport implementations
   
2. **Source/Private/Communication/Transport/StreamableHTTP/StreamableHTTPTransport.cpp**  
   - Complete HTTPTransportClient and HTTPTransportServer implementations
   
3. **Source/Private/Client/Core/MCPClient.cpp**
   - Complete MCPClient implementation with all MCP client operations
   
4. **Source/Private/Server/Core/MCPServer.cpp**
   - Complete MCPServer implementation with all MCP server operations
   
5. **Source/Private/Core/MCPProtocol.cpp**
   - Base protocol implementation shared by client and server

6. **Source/Private/SimpleMCPAPI.cpp**
   - User-friendly wrapper implementations

### Documentation Updates
7. **Update README.md** - Add build instructions, usage examples, troubleshooting
8. **Create IMPLEMENTATION.md** - Technical implementation details and architecture decisions

## Quality Criteria

### Code Quality Checklist
- [ ] All coroutines properly handle exceptions
- [ ] Memory leaks prevented with RAII
- [ ] Thread-safe access to shared data
- [ ] Proper const-correctness throughout
- [ ] Move semantics used for performance
- [ ] Clear separation of concerns
- [ ] Comprehensive error handling
- [ ] No undefined behavior
- [ ] Platform-independent code (Windows/Linux/macOS)

### Functional Requirements Checklist
- [ ] Full MCP specification compliance
- [ ] Stdio transport works with external processes
- [ ] HTTP transport supports SSE bidirectional communication
- [ ] Client can call all server capabilities
- [ ] Server can handle all client requests
- [ ] Proper capability negotiation
- [ ] Resource subscription/unsubscription works
- [ ] Progress notifications function correctly
- [ ] Sampling integration with LLM providers
- [ ] Graceful error recovery and cleanup

### Performance Requirements
- [ ] Sub-millisecond message serialization/deserialization
- [ ] Thousands of concurrent transport connections supported
- [ ] Minimal CPU usage when idle
- [ ] Memory usage scales linearly with connections
- [ ] No blocking operations on main threads

## Build Integration

Ensure implementations work with the existing CMake structure:
```cmake
# Example additions needed
target_sources(cplusplus-sdk PRIVATE
    Source/Private/Communication/Transport/Stdio/StdioTransport.cpp
    Source/Private/Communication/Transport/StreamableHTTP/StreamableHTTPTransport.cpp
    Source/Private/Client/Core/MCPClient.cpp
    Source/Private/Server/Core/MCPServer.cpp
    Source/Private/Core/MCPProtocol.cpp
    Source/Private/SimpleMCPAPI.cpp
)

target_link_libraries(cplusplus-sdk PRIVATE 
    Poco::Foundation 
    Poco::Net 
    Poco::Process
)
```

## Success Criteria
The implementation is complete when:
1. All header declarations have corresponding implementations
2. Full MCP specification compliance achieved  
3. Both stdio and HTTP transports function correctly
4. Client and server can communicate bidirectionally
5. Simple API provides easy integration path
6. Code passes all memory safety and thread safety checks
7. Performance meets or exceeds specification requirements
8. Documentation enables third-party integration
9. Examples demonstrate all major use cases
10. Error handling is robust and informative

Generate production-ready, specification-compliant implementations that provide an excellent developer experience for C++ MCP integration. 