# MCP C++ SDK - Critical Issues Fix Plan

## Error Categories Summary

### 1. Missing Type Definitions & Forward Declarations (Critical) - [Moderate]
- `MCPClientInfo` and `MCPServerInfo` types are used but not defined
- Various message response types have incomplete definitions
- Missing handler type aliases and function signatures
- Forward declaration issues across headers

### 2. Include Dependencies (Critical) - [Minor]
- Missing includes for standard library headers (`<unordered_map>`, `<functional>`, etc.)
- Circular include dependencies between headers
- Missing includes for coroutine support headers
- JSONProxy.h include path issues

### 3. Coroutine Implementation Issues (Critical) - [Moderate]
- MCPTask coroutine implementation has design flaws in promise handling
- Missing `co_await` syntax compliance
- Incorrect template specialization for void tasks
- Promise lifetime management issues

### 4. Member Variable Access Violations (Critical) - [Moderate]
- Private member variables accessed without proper scope
- Inconsistent member variable naming (some missing `m_` prefix)
- Base class member access issues in derived implementations

### 5. Constructor/Factory Method Mismatches (High)
- SimpleMCPAPI constructors don't match factory method signatures
- MCPServer constructor parameter mismatches
- Transport factory method inconsistencies

### 6. JSON Serialization Issues (High)
- JKEY macro usage inconsistencies
- Missing JSON serialization for complex types
- Template instantiation issues with JSONProxy

### 7. Transport Layer Issues (Medium)
- HTTPTransport implementation has threading issues
- Missing proper cleanup in transport destructors
- State management inconsistencies

---

## Priority Fix Plan

### Phase 1: Foundation Types (CRITICAL - DO FIRST)
**Goal**: Resolve all missing type definitions to enable basic compilation

1. **Create Missing Type Definitions**
   - Define `MCPClientInfo` and `MCPServerInfo` structs in appropriate headers
   - Add missing response type definitions for tool/prompt/resource operations
   - Define all handler function signatures consistently

2. **Fix Include Dependencies**
   - Add missing standard library includes across all headers
   - Resolve circular dependencies by moving implementations to source files
   - Fix JSONProxy.h include paths and dependencies

3. **Forward Declarations Cleanup** 
   - Add proper forward declarations for all referenced types
   - Remove unnecessary includes in headers, use forward declarations instead
   - Ensure consistent namespace usage throughout

### Phase 2: Core Implementation Fixes (CRITICAL)
**Goal**: Fix fundamental implementation issues blocking compilation

4. **MCPTask Coroutine Redesign**
   - Fix promise_type implementation for proper RAII and exception handling
   - Correct template specialization for void return types  
   - Implement proper `co_await` and `co_return` compliance
   - Fix promise lifetime and result storage issues

5. **Member Access & Naming Fixes** ✅ **COMPLETED**
   - ✅ Added missing `m_` prefixes to private/protected member variables 
   - ✅ Fixed all member variable access violations in implementations
   - ✅ Ensured proper encapsulation and access patterns
   - ✅ Added missing member variables to MCPServer.h (m_IsRunning, handler maps, mutexes)
   - ✅ Verified all existing member variables follow proper m_ naming convention

6. **Constructor/Factory Alignment**
   - Update SimpleMCPAPI constructors to match factory method signatures
   - Fix MCPServer constructor parameter mismatches  
   - Standardize transport creation patterns

### Phase 3: Integration & Serialization (HIGH)
**Goal**: Ensure proper data serialization and transport integration

7. **JSON Serialization Standardization**
   - Fix all JKEY macro usage inconsistencies
   - Implement missing JSON serialization for complex message types
   - Resolve template instantiation issues with custom types

8. **Message System Completion**
   - Complete all request/response message type definitions
   - Implement proper message routing and handling
   - Fix notification system implementation

### Phase 4: Transport & Threading (MEDIUM)
**Goal**: Stabilize transport layer and handle concurrency properly

9. **Transport Implementation Fixes**
   - Fix HTTPTransport threading and lifecycle issues
   - Implement proper cleanup and resource management
   - Standardize error handling across all transports

10. **State Management & Concurrency**
    - Fix transport state management inconsistencies
    - Resolve mutex usage and thread safety issues  
    - Implement proper async/await patterns

---

## Implementation Notes

- **Focus on Phase 1-2 first** - These contain the blocking compilation errors
- **Phase 3-4 address integration issues** that will surface after basic compilation works
- **Each phase builds on the previous** - complete in order for best results
- **Test compilation after each major fix** to catch cascading issues early

## Files Requiring Immediate Attention

### Most Critical:
- `Source/Public/API/SimpleMCPAPI.h` - Missing type definitions, constructor signatures
- `Source/Public/Utilities/Async/MCPTask.h` - Coroutine implementation flaws  
- `Source/Public/CoreSDK/Core/IMCP.h` - Forward declarations, includes
- `Source/Private/API/SimpleMCPAPI.cpp` - Undefined types, member access

### High Priority:
- `Source/Public/CoreSDK/Messages/MCPMessages.h` - Message type completions
- `Source/Private/CoreSDK/Core/MCPServer.cpp` - Implementation fixes
- `Source/Public/Proxies/JSONProxy.h` - Template and macro issues
- Transport implementation files - Resource cleanup, threading
