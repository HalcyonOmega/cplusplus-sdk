# MCP SDK TypeScript to C++ Conversion Map

## Directory Structure Mapping

### TypeScript SDK Structure
```
src/
├── client/           # Client-side implementation
├── server/           # Server-side implementation
├── shared/           # Shared utilities and types
├── examples/         # Example implementations
├── __mocks__/        # Test mocks
├── integration-tests/# Integration test suite
├── types.ts          # Core type definitions
├── types.test.ts     # Type tests
├── inMemory.ts       # In-memory implementation
├── inMemory.test.ts  # In-memory tests
└── cli.ts            # Command-line interface
```

### C++ SDK Structure (Current)
```
source/
├── Public/           # Public API headers
│   ├── Communication/# Communication interfaces
│   ├── Core/         # Core functionality
│   ├── Schemas/      # Schema definitions
│   └── [Header files]# Individual component headers
└── Private/          # Implementation details
    └── Communication/# Communication implementations
```

## Component Mapping

### Core Types and Interfaces

| TypeScript | C++ | Notes |
|------------|-----|-------|
| `types.ts` | `Public/Schemas/` | Split into multiple header files for better organization |
| `shared/` | `Public/Core/` | Core utilities and shared functionality |

### Client Implementation

| TypeScript | C++ | Notes |
|------------|-----|-------|
| `client/` | `Public/MCP_Client.h` + `Private/Client/` | Client implementation split into public interface and private implementation |

### Server Implementation

| TypeScript | C++ | Notes |
|------------|-----|-------|
| `server/` | `Public/MCP_Server.h` + `Private/Server/` | Server implementation split into public interface and private implementation |

### Communication Layer

| TypeScript | C++ | Notes |
|------------|-----|-------|
| `shared/transport/` | `Public/Communication/` + `Private/Communication/` | Transport layer implementation |
| `shared/protocol/` | `Public/Core/Protocol/` | Protocol definitions and utilities |
| `inMemory.ts` | `Private/Client/InMemoryClient.hpp` | In-memory client implementation |
| `inMemory.ts` | `Private/Server/InMemoryServer.hpp` | In-memory server implementation |


## Implementation Guidelines

1. **Header Organization**
   - Public headers in `Public/` directory
   - Implementation details in `Private/` directory
   - Use forward declarations where possible
   - Keep headers minimal and focused

2. **Modern C++ Features**
   - Use C++17/20 features where available INSTEAD of external libraries
   - Leverage RAII for resource management
   - Use smart pointers for memory management
   - Implement move semantics where appropriate

3. **External Dependencies**
   - Minimize external dependencies
   - Use standard library where possible
   - We ARE using:
     - nlohmann/json for JSON handling

4. **Developer Experience**
   - Clear and consistent API design
   - Comprehensive documentation
   - Strong type safety
   - Meaningful error messages
   - Easy to use and understand

5. **Spec Compliance**
   - Strict adherence to MCP specification
   - Complete implementation of all required features
   - Proper error handling and reporting
   - Support for all transport methods

## File Creation Priority

1. Core Types and Interfaces
   - Schema definitions
   - Basic message types
   - Protocol structures

2. Communication Layer
   - Transport implementations
   - Protocol handlers
   - Message serialization

3. Client Implementation
   - Client interface
   - Client implementation
   - In-memory client

4. Server Implementation
   - Server interface
   - Server implementation
   - In-memory server

5. Examples and Tests
   - Basic examples
   - Integration tests
   - Unit tests

## Notes

- All new files should be created with proper header guards
- Implementation files should be in the Private directory
- Public interfaces should be well-documented
- Follow C++ best practices and coding standards
- Maintain backward compatibility where possible
- Consider platform-specific implementations where needed
