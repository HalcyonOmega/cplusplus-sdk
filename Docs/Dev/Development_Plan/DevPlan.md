# SDK Development Plan

## Development Prompt

---
**User Guidance:**
We are creating a C++ version of the typescript MCP SDK with the goal of achieving equivalent functionality with as wide compatibility as possible using C++ modern best practices. Developer experience when using our SDK is of paramount importance, only slightly behind complete compliance with the MCP spec. You WILL be graded meticulously for quality, compliance with the spec, and end-developer experience. Code should be kept clean where possible.

**IMPORTANT - File/Folder Creation Protocol:**
1. Before creating ANY new file or folder for the C++ SDK:
   - Ask the user where they want to locate it
   - Ask about any structural preferences or requirements
   - Wait for explicit user approval before proceeding
2. If you find an existing file that needs modification:
   - Ask for user permission before making changes
3. If you need to create a new file/folder structure:
   - Propose the structure to the user
   - Explain the reasoning behind the structure
   - Get user approval before implementation
4. For any structural decisions:
   - Present options to the user
   - Explain trade-offs
   - Get user input before proceeding

**Requirements:**
- Use nested GitHub-style checklists for clarity and developer usability (see Implementation Phases section)
- Reference protocol, schema, and SDK documentation as needed for accuracy
- Prioritize developer experience, robust error handling, extensibility, and comprehensive documentation throughout (see Implementation Guidelines section)
- Do not include any timelines or estimates for phases or tasks
- As you complete tasks, update the corresponding phase tracker, marking off completed items
- Provide short, critical feedback summaries for any issues, blockers, or important observations
- Stay organized and follow user guidance explicitly
- If you encounter any ambiguity or missing information, ask for clarification rather than making assumptions
- Use clear, concise questions for any required user input
- Incorporate feedback and iterate on checklists as needed

**References:**
- You may reference the `SDK_FullReference.md` for more information
- The Development Plan is in the `Docs/Dev/Development_Plan/` directory, please reference it for more information as needed
- The project files are in the `Source/` directory, please reference it for more information as needed
- A previous version of the development effort, translated directly from the Typescript version of the MCP SDK, is available in the `Reference/` directory
- The full model context protocol specification is available in the `Docs/Model Context Protocol/Spec/` directory
---

## Project Structure

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

## Implementation Phases

### Phase 1: Core Types and Interfaces
- [ ] Schema Definitions
  - [ ] Convert TypeScript types to C++ classes
  - [ ] Implement JSON serialization/deserialization
  - [ ] Add validation and error handling
  - [ ] Document public interfaces

- [ ] Basic Message Types
  - [ ] Request/Response structures
  - [ ] Notification handling
  - [ ] Error types and handling

- [ ] Protocol Structures
  - [ ] Message format definitions
  - [ ] Protocol version handling
  - [ ] Capability negotiation

### Phase 2: Communication Layer
- [ ] Transport Implementations
  - [ ] stdio transport
  - [ ] HTTP transport
  - [ ] Custom transport support

- [ ] Protocol Handlers
  - [ ] Message routing
  - [ ] Request/response handling
  - [ ] Notification dispatching

- [ ] Message Serialization
  - [ ] JSON-RPC 2.0 compliance
  - [ ] UTF-8 encoding
  - [ ] Batch message support

### Phase 3: Client Implementation
- [ ] Client Interface
  - [ ] Public API design
  - [ ] Error handling
  - [ ] Resource management

- [ ] Client Implementation
  - [ ] Connection management
  - [ ] Message handling
  - [ ] State management

- [ ] In-memory Client
  - [ ] Testing support
  - [ ] Mock capabilities
  - [ ] Debug features

### Phase 4: Server Implementation
- [ ] Server Interface
  - [ ] Public API design
  - [ ] Error handling
  - [ ] Resource management

- [ ] Server Implementation
  - [ ] Connection management
  - [ ] Message handling
  - [ ] State management

- [ ] In-memory Server
  - [ ] Testing support
  - [ ] Mock capabilities
  - [ ] Debug features

## Implementation Guidelines

### 1. Header Organization
- [ ] Public headers in `Public/` directory
- [ ] Implementation details in `Private/` directory
- [ ] Use forward declarations where possible
- [ ] Keep headers minimal and focused

### 2. Modern C++ Features
- [ ] Use C++17/20 features where available INSTEAD of external libraries
- [ ] Leverage RAII for resource management
- [ ] Use smart pointers for memory management
- [ ] Implement move semantics where appropriate

### 3. External Dependencies
- [ ] Minimize external dependencies
- [ ] Use standard library where possible
- [ ] We ARE using:
  - [ ] nlohmann/json for JSON handling

### 4. Developer Experience
- [ ] Clear and consistent API design
- [ ] Comprehensive documentation
- [ ] Strong type safety
- [ ] Meaningful error messages
- [ ] Easy to use and understand

### 5. Spec Compliance
- [ ] Strict adherence to MCP specification
- [ ] Complete implementation of all required features
- [ ] Proper error handling and reporting
- [ ] Support for all transport methods

## Notes
- All new files should be created with proper header guards
- Implementation files should be in the Private directory
- Public interfaces should be well-documented
- Follow C++ best practices and coding standards
- Maintain backward compatibility where possible
- Consider platform-specific implementations where needed
