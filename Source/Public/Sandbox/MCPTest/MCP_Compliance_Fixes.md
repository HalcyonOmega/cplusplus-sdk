# MCP C++ SDK Complete Compliance Implementation Guide

## Agent Instructions

You are an expert C++ developer tasked with implementing the complete functionality for a Model Context Protocol (MCP) SDK based on the provided framework declarations. Your goal is to create production-ready implementations that are fully compliant with the MCP specification (2025-03-26), memory-safe, thread-safe, and performant.

### Code Generation Guidelines
- **Language**: Modern C++20 with coroutines, concepts, and ranges
- **Libraries**: Only Poco C++ Libraries and nlohmann/json
- **Memory Management**: Use RAII, smart pointers, no raw pointers for ownership
- **Error Handling**: Exception-safe code with proper error propagation
- **Thread Safety**: All public APIs must be thread-safe
- **Performance**: Optimize for low latency and high throughput
- **Maintainability**: Clean, readable, well-documented code

### Naming Conventions (CRITICAL)
- **PascalCase** for everything (classes, functions, variables, files)
- **Acronyms stay ALL CAPS**: `HTTP`, `JSON`, `API`, `MCP`, `XML`, `RPC`
- **Private/Protected member variables**: Use `m_` prefix
- **Method parameters**: `In` prefix for inputs, `Out` prefix for outputs
- **No snake_case, camelCase, or kebab-case**

---

## CRITICAL ISSUES (BLOCKING COMPLIANCE) - MUST IMPLEMENT

### 1. HTTP Transport GET Endpoint Implementation ❌ CRITICAL
**MCP Spec Reference**: Transport Layer - StreamableHTTP with Server-Sent Events
**Status**: MISSING - Prevents StreamableHTTP transport compliance

**Implementation Required**:
```cpp
// In HTTPTransport.h - Add method declaration
MCPTaskVoid HandleGetMessageEndpoint(Poco::Net::HTTPServerRequest& InRequest, 
                                   Poco::Net::HTTPServerResponse& InResponse);

// In HTTPTransport.cpp - Implement GET /message endpoint
MCPTaskVoid HTTPTransportServer::HandleGetMessageEndpoint(
    Poco::Net::HTTPServerRequest& InRequest, 
    Poco::Net::HTTPServerResponse& InResponse) {
    
    // Set SSE headers
    InResponse.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
    InResponse.setContentType("text/event-stream");
    InResponse.set("Cache-Control", "no-cache");
    InResponse.set("Connection", "keep-alive");
    InResponse.set("Access-Control-Allow-Origin", "*");
    InResponse.set("Access-Control-Allow-Headers", "Content-Type");
    
    // Generate unique client ID
    std::string clientID = GenerateUniqueClientID();
    
    // Register SSE client for message streaming
    RegisterSSEClient(clientID, InResponse);
    
    // Keep connection alive and stream messages
    co_await StreamMessagesToClient(clientID);
}
```

**Files to Modify**:
- `HTTPTransport.h` - Add GET endpoint handler declaration
- `HTTPTransport.cpp` - Implement GET endpoint logic with SSE support
- Add SSE client management system
- Implement message streaming capabilities

**Requirements**:
- Support for Server-Sent Events (SSE) protocol
- Client connection management
- Message queuing and streaming
- Proper CORS headers
- Connection lifecycle management

### 2. Resource Pagination Complete Implementation ❌ CRITICAL
**MCP Spec Reference**: Resources - List Resources with Cursor Support
**Status**: PARTIAL - Structure exists but logic incomplete

**Implementation Required**:
```cpp
// In MCPProtocol.h - Add pagination helper methods
class MCPServer {
    // ... existing code ...
    
private:
    std::string EncodeCursor(size_t InIndex) const;
    size_t DecodeCursor(const std::string& InCursor) const;
    static constexpr size_t DEFAULT_PAGE_SIZE = 100;
};

// In MCPProtocol.cpp - Implement complete pagination logic
MCPTask<ListResourcesResult> MCPServer::HandleListResourcesRequest(
    const ListResourcesRequest& InRequest) {
    
    ListResourcesResult result;
    size_t startIndex = 0;
    size_t pageSize = DEFAULT_PAGE_SIZE;
    
    // Decode cursor if provided
    if (InRequest.RequestParams.Cursor.has_value()) {
        try {
            startIndex = DecodeCursor(*InRequest.RequestParams.Cursor);
        } catch (const std::exception& e) {
            throw MCPException(MCPErrorCodes::INVALID_PARAMS, 
                             "Invalid cursor format");
        }
    }
    
    // Get all available resources
    auto allResources = GetAllRegisteredResources();
    auto totalResources = allResources.size();
    auto endIndex = std::min(startIndex + pageSize, totalResources);
    
    // Extract page of resources
    result.Resources.assign(allResources.begin() + startIndex, 
                           allResources.begin() + endIndex);
    
    // Set next cursor if more resources available
    if (endIndex < totalResources) {
        result.NextCursor = EncodeCursor(endIndex);
    }
    
    co_return result;
}

std::string MCPServer::EncodeCursor(size_t InIndex) const {
    // Use base64 encoding for cursor
    std::string indexStr = std::to_string(InIndex);
    // Implementation with base64 encoding
    return Base64Encode(indexStr);
}

size_t MCPServer::DecodeCursor(const std::string& InCursor) const {
    try {
        std::string decoded = Base64Decode(InCursor);
        return std::stoull(decoded);
    } catch (const std::exception&) {
        throw std::invalid_argument("Invalid cursor format");
    }
}
```

**Files to Modify**:
- `MCPProtocol.h` - Add pagination helper methods
- `MCPProtocol.cpp` - Complete pagination implementation
- Add cursor encoding/decoding utilities
- Implement resource collection management

**Requirements**:
- Base64 cursor encoding/decoding
- Configurable page sizes
- Error handling for invalid cursors
- Thread-safe resource collection access

### 3. JSON Schema Validation Enforcement ❌ CRITICAL
**MCP Spec Reference**: Tools - Input Schema Validation
**Status**: MISSING - Schema structures exist but validation not enforced

**Implementation Required**:
```cpp
// In JSONSchemaValidator.h - Complete validation API
class JSONSchemaValidator {
public:
    static bool ValidateAgainstSchema(const nlohmann::json& InData, 
                                     const JSONSchema& InSchema);
    static std::string GetValidationErrors(const nlohmann::json& InData,
                                         const JSONSchema& InSchema);
    
private:
    static bool ValidateObjectSchema(const nlohmann::json& InData,
                                   const JSONSchema& InSchema);
    static bool ValidateArraySchema(const nlohmann::json& InData,
                                  const JSONSchema& InSchema);
    static bool ValidatePropertyType(const nlohmann::json& InValue,
                                   const nlohmann::json& InTypeSpec);
};

// In MCPProtocol.cpp - Enforce validation in tool calls
MCPTask<CallToolResult> MCPServer::HandleCallToolRequest(
    const CallToolRequest& InRequest) {
    
    auto toolName = InRequest.RequestParams.Name;
    auto toolIter = m_RegisteredTools.find(toolName);
    
    if (toolIter == m_RegisteredTools.end()) {
        throw MCPException(MCPErrorCodes::TOOL_NOT_FOUND, 
                         "Tool not found: " + toolName);
    }
    
    const auto& tool = toolIter->second;
    
    // CRITICAL: Validate arguments against schema
    if (InRequest.RequestParams.Arguments.has_value()) {
        nlohmann::json argsJson = *InRequest.RequestParams.Arguments;
        
        if (!JSONSchemaValidator::ValidateAgainstSchema(argsJson, tool.InputSchema)) {
            std::string errors = JSONSchemaValidator::GetValidationErrors(
                argsJson, tool.InputSchema);
            throw MCPException(MCPErrorCodes::SCHEMA_VALIDATION_ERROR,
                             "Tool arguments validation failed: " + errors);
        }
    } else if (!tool.InputSchema.Required.value_or(std::vector<std::string>{}).empty()) {
        throw MCPException(MCPErrorCodes::SCHEMA_VALIDATION_ERROR,
                         "Required arguments missing for tool: " + toolName);
    }
    
    // Execute tool with validated arguments
    co_return co_await ExecuteToolSafely(tool, InRequest.RequestParams.Arguments);
}
```

**Files to Modify**:
- `JSONSchemaValidator.h` - Complete validation interface
- `JSONSchemaValidator.cpp` - Implement comprehensive validation logic
- `MCPProtocol.cpp` - Integrate validation in tool execution path
- Add validation for prompt arguments as well

**Requirements**:
- Object, array, string, number, boolean validation
- Required field validation
- Type constraint checking
- Detailed error reporting
- Performance-optimized validation

### 4. Progress Reporting Integration ❌ CRITICAL
**MCP Spec Reference**: Progress Notifications
**Status**: PARTIAL - Structures exist but not integrated with operations

**Implementation Required**:
```cpp
// In MCPProtocol.h - Add ProgressTracker class
class ProgressTracker {
public:
    ProgressTracker(const std::string& InRequestID, 
                   std::shared_ptr<ITransport> InTransport);
    
    MCPTaskVoid UpdateProgress(double InProgress, 
                              std::optional<int64_t> InTotal = {});
    MCPTaskVoid CompleteProgress();
    
private:
    std::string m_RequestID;
    std::shared_ptr<ITransport> m_Transport;
    std::atomic<bool> m_IsComplete{false};
};

// In MCPProtocol.cpp - Integrate with long-running operations
MCPTask<CallToolResult> MCPServer::ExecuteToolWithProgress(
    const Tool& InTool,
    const std::optional<std::unordered_map<std::string, nlohmann::json>>& InArguments,
    const std::string& InRequestID) {
    
    auto progressTracker = std::make_shared<ProgressTracker>(InRequestID, m_Transport);
    
    try {
        // Update progress at 0%
        co_await progressTracker->UpdateProgress(0.0);
        
        // Execute tool with progress callback
        auto result = co_await ExecuteToolWithProgressCallback(
            InTool, InArguments, progressTracker);
        
        // Complete progress
        co_await progressTracker->CompleteProgress();
        
        co_return result;
    } catch (const std::exception& e) {
        // Ensure progress is marked complete even on error
        co_await progressTracker->CompleteProgress();
        throw;
    }
}
```

**Files to Modify**:
- `MCPProtocol.h` - Add ProgressTracker class
- `MCPProtocol.cpp` - Implement progress reporting
- Integrate with tool execution, resource operations
- Add progress callback mechanisms

**Requirements**:
- Thread-safe progress tracking
- Integration with coroutine operations
- Error handling with progress cleanup
- Configurable progress reporting intervals

### 5. Resource Subscription Management ❌ CRITICAL
**MCP Spec Reference**: Resources - Subscriptions and Notifications
**Status**: PARTIAL - Message structures exist but subscription lifecycle incomplete

**Implementation Required**:
```cpp
// In MCPProtocol.h - Add subscription management
class MCPServer {
    // ... existing code ...
    
private:
    std::unordered_map<std::string, std::set<std::string>> m_ResourceSubscriptions;
    std::mutex m_SubscriptionsMutex;
    
    MCPTaskVoid NotifyResourceSubscribers(const std::string& InURI);
    void AddResourceSubscription(const std::string& InURI, const std::string& InClientID);
    void RemoveResourceSubscription(const std::string& InURI, const std::string& InClientID);
};

// In MCPProtocol.cpp - Implement subscription lifecycle
MCPTask<EmptyResult> MCPServer::HandleSubscribeRequest(
    const SubscribeRequest& InRequest) {
    
    std::string uri = InRequest.RequestParams.URI;
    std::string clientID = GetCurrentClientID();
    
    // Validate resource exists
    if (!ResourceExists(uri)) {
        throw MCPException(MCPErrorCodes::RESOURCE_NOT_FOUND,
                         "Resource not found: " + uri);
    }
    
    // Add subscription
    {
        std::lock_guard<std::mutex> lock(m_SubscriptionsMutex);
        m_ResourceSubscriptions[uri].insert(clientID);
    }
    
    co_return EmptyResult{};
}

MCPTaskVoid MCPServer::NotifyResourceSubscribers(const std::string& InURI) {
    std::set<std::string> subscribers;
    
    {
        std::lock_guard<std::mutex> lock(m_SubscriptionsMutex);
        auto iter = m_ResourceSubscriptions.find(InURI);
        if (iter != m_ResourceSubscriptions.end()) {
            subscribers = iter->second;
        }
    }
    
    if (!subscribers.empty()) {
        ResourceUpdatedNotification notification;
        notification.NotificationParams.URI = InURI;
        
        for (const auto& clientID : subscribers) {
            co_await SendNotificationToClient(clientID, notification);
        }
    }
}
```

**Files to Modify**:
- `MCPProtocol.h` - Add subscription management
- `MCPProtocol.cpp` - Implement subscription lifecycle
- Add client identification system
- Implement notification delivery

**Requirements**:
- Thread-safe subscription management
- Client identification and tracking
- Resource change detection
- Notification delivery guarantees

---

## MAJOR ISSUES (SHOULD FIX) - IMPORTANT FOR PRODUCTION

### 6. Protocol Version Validation ⚠️ IMPORTANT
**MCP Spec Reference**: Lifecycle - Initialize Request/Response
**Status**: MISSING - No validation of supported protocol versions

**Implementation Required**:
```cpp
// In MCPProtocol.h - Add version management
class MCPServer {
    // ... existing code ...
    
private:
    static const std::vector<std::string> SUPPORTED_PROTOCOL_VERSIONS;
    void ValidateProtocolVersion(const std::string& InVersion) const;
};

// In MCPProtocol.cpp - Implement version validation
const std::vector<std::string> MCPServer::SUPPORTED_PROTOCOL_VERSIONS = {
    "2024-11-05", "2025-03-26"
};

void MCPServer::ValidateProtocolVersion(const std::string& InVersion) const {
    auto iter = std::find(SUPPORTED_PROTOCOL_VERSIONS.begin(),
                         SUPPORTED_PROTOCOL_VERSIONS.end(), InVersion);
    
    if (iter == SUPPORTED_PROTOCOL_VERSIONS.end()) {
        throw MCPException(MCPErrorCodes::PROTOCOL_VERSION_MISMATCH,
                         "Unsupported protocol version: " + InVersion + 
                         ". Supported versions: " + JoinVersions());
    }
}

MCPTask<InitializeResult> MCPServer::HandleInitializeRequest(
    const InitializeRequest& InRequest) {
    
    // CRITICAL: Validate protocol version first
    ValidateProtocolVersion(InRequest.RequestParams.ProtocolVersion);
    
    // Continue with normal initialization...
    InitializeResult result;
    result.ProtocolVersion = SelectBestSupportedVersion(
        InRequest.RequestParams.ProtocolVersion);
    // ... rest of initialization
    
    co_return result;
}
```

**Requirements**:
- Support for multiple protocol versions
- Version negotiation logic
- Clear error messages for unsupported versions
- Backward compatibility handling

### 7. Authorization Framework Implementation ⚠️ IMPORTANT
**MCP Spec Reference**: Authorization - OAuth 2.1 Framework
**Status**: INCOMPLETE - Basic structure present but OAuth framework missing

**Implementation Required**:
```cpp
// In Auth/ directory - Create comprehensive authorization system
class MCPAuthProvider {
public:
    virtual ~MCPAuthProvider() = default;
    virtual MCPTask<bool> ValidateToken(const std::string& InToken) = 0;
    virtual MCPTask<AuthResult> AuthorizeRequest(const std::string& InMethod,
                                               const std::string& InToken) = 0;
};

class OAuth2AuthProvider : public MCPAuthProvider {
public:
    OAuth2AuthProvider(const OAuth2Config& InConfig);
    
    MCPTask<bool> ValidateToken(const std::string& InToken) override;
    MCPTask<AuthResult> AuthorizeRequest(const std::string& InMethod,
                                       const std::string& InToken) override;
    
private:
    OAuth2Config m_Config;
    std::shared_ptr<Poco::Net::HTTPSClientSession> m_AuthSession;
};

// Integrate with HTTP transport
class HTTPTransportServer {
    // ... existing code ...
    
private:
    std::shared_ptr<MCPAuthProvider> m_AuthProvider;
    MCPTask<bool> ValidateRequest(Poco::Net::HTTPServerRequest& InRequest);
};
```

**Requirements**:
- OAuth 2.1 token validation
- Bearer token support
- RBAC integration
- Configurable auth providers
- Security best practices

### 8. Comprehensive Error Recovery ⚠️ IMPORTANT
**MCP Spec Reference**: Error Handling Best Practices
**Status**: PARTIAL - Basic error handling present but recovery mechanisms incomplete

**Implementation Required**:
```cpp
// In MCPProtocol.h - Add error recovery mechanisms
class MCPServer {
    // ... existing code ...
    
private:
    std::atomic<bool> m_IsInRecoveryMode{false};
    std::mutex m_RecoveryMutex;
    
    MCPTaskVoid HandleTransportError(const std::exception& InError);
    MCPTaskVoid AttemptConnectionRecovery();
    MCPTaskVoid CleanupFailedOperations();
};

// Implement circuit breaker pattern for external resources
class CircuitBreaker {
public:
    enum class State { Closed, Open, HalfOpen };
    
    template<typename TResult>
    MCPTask<TResult> Execute(std::function<MCPTask<TResult>()> InOperation);
    
private:
    State m_State = State::Closed;
    std::atomic<size_t> m_FailureCount{0};
    std::chrono::steady_clock::time_point m_LastFailureTime;
    static constexpr size_t FAILURE_THRESHOLD = 5;
    static constexpr auto RECOVERY_TIMEOUT = std::chrono::seconds(30);
};
```

**Requirements**:
- Circuit breaker pattern for external calls
- Automatic retry mechanisms
- Graceful degradation
- Connection recovery
- Resource cleanup on failures

---

## MINOR ISSUES (COULD FIX) - ENHANCEMENTS

### 9. Performance Optimizations ⚠️ ENHANCEMENT
**Implementation Required**:
- Message pooling for high-throughput scenarios
- Connection pooling for HTTP transport
- Async I/O optimizations
- Memory usage optimization
- Caching strategies for frequently accessed resources

### 10. Extended JSON Schema Support ⚠️ ENHANCEMENT
**Implementation Required**:
- Support for advanced JSON Schema features (anyOf, oneOf, allOf)
- Custom validation rules
- Schema composition and inheritance
- Performance-optimized validation caching

### 11. Comprehensive Logging Integration ⚠️ ENHANCEMENT
**Implementation Required**:
- Structured logging with correlation IDs
- Configurable log levels per component
- Log aggregation support
- Performance metrics collection

### 12. Batch Request Processing ⚠️ ENHANCEMENT
**MCP Spec Reference**: JSON-RPC Batching Support
**Implementation Required**:
- Support for JSON-RPC batch requests
- Parallel processing of batch items
- Partial failure handling in batches
- Performance optimization for large batches

---

## IMPLEMENTATION PRIORITY

### Phase 1
1. HTTP Transport GET Endpoint Implementation
2. Resource Pagination Complete Implementation
3. JSON Schema Validation Enforcement

### Phase 2
4. Progress Reporting Integration
5. Resource Subscription Management

### Phase 3
6. Protocol Version Validation
7. Authorization Framework Implementation
8. Comprehensive Error Recovery

### Phase 4
9. Performance Optimizations
10. Extended JSON Schema Support
11. Comprehensive Logging Integration
12. Batch Request Processing

---

## SUCCESS CRITERIA

Each implementation must meet these requirements:

### ✅ Functional Requirements
- [ ] 100% MCP Specification (2025-03-26) compliance
- [ ] All message types properly handled
- [ ] Complete transport layer support (Stdio + StreamableHTTP)
- [ ] Full feature coverage (Tools, Prompts, Resources, Sampling)

### ✅ Quality Requirements
- [ ] Thread-safe implementation
- [ ] Memory-safe (no leaks, proper RAII)
- [ ] Exception-safe with proper error propagation
- [ ] Performance optimized (low latency, high throughput)
- [ ] Comprehensive unit test coverage

### ✅ Code Quality Requirements
- [ ] Follows naming conventions exactly
- [ ] Modern C++20 features used appropriately
- [ ] Clean, maintainable, well-documented code
- [ ] Consistent code style throughout
- [ ] Proper const-correctness and type safety

---

## FINAL NOTES

**CRITICAL**: Each implementation must be complete, production-ready, and fully tested. Partial implementations that break existing functionality are not acceptable. 

**TESTING**: All implementations must include comprehensive unit tests demonstrating functionality and error handling.

**DOCUMENTATION**: Each major component must include clear usage examples and API documentation.

**PERFORMANCE**: Implementations should be benchmarked and optimized for production use cases.

Remember: This SDK will be used in production environments. Quality, reliability, and performance are paramount.
