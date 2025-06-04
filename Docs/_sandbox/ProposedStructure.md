# C++ MCP SDK - Proposed Structure

## High-Level Folder Structure

```
Source/Public/
├── Core/                # Core types, protocols, and base functionality
│   ├── Types/           # Fundamentals + functional entities
│   ├── Schemas/         # Static data structures
│   ├── Features/        # MCP domain features
│   │   └── ${Concepts}/ # MCP concepts (Tools, Resources, Prompts, etc.)
│   └── Protocols/       # Protocol implementations
├── Communication/       # All transport and messaging functionality
│   ├── Transport/
│   ├── Serialization/
│   ├── Protocols/
│   └── Utilities/
├── Authentication/      # Authentication and authorization
│   ├── Types/
│   ├── Providers/
│   ├── Client/
│   ├── Server/
│   └── Utilities/
├── Client/             # Client-side SDK functionality
│   ├── Core/
│   ├── Handlers/
│   ├── Sessions/
│   └── Utilities/
├── Server/             # Server-side SDK functionality
│   ├── Core/
│   ├── Handlers/
│   ├── Sessions/
│   └── Utilities/
└── Utilities/          # General utilities and CLI
    ├── CLI/
    ├── JSON/
    ├── URI/
    ├── Validation/
    └── Lux/
```

## Detailed Breakdown by Functional Area

### Core
Core infrastructure that everything else builds upon. Contains fundamental types, base interfaces, MCP domain features, and protocol definitions that are domain-agnostic.

### Communication  
All networking, transport protocols, message serialization, and communication utilities. Focused specifically on the "how" of message exchange.

### Authentication
Complete authentication and authorization system, organized to support both client and server scenarios with consistent internal structure.

### Client & Server
Dedicated areas for client-side and server-side specific functionality, each with their own complete set of core components, handlers, and utilities.

### Utilities
Development utilities, CLI tools, validation, and helper functions.

---

## Rationale for Structure

**Functional Grouping**: Each major folder represents a distinct functional domain with clear responsibilities and minimal cross-dependencies.

**Fractal Organization**: Each major area follows a consistent internal structure (Types, Core, Handlers, Utilities, etc.) making navigation predictable.

**Clear Separation of Concerns**: 
- Core = basic building blocks, protocols, and MCP domain features
- Communication = message transport
- Authentication = security
- Client/Server = role-specific logic  
- Utilities = helper functionality

**Scalability**: New features can be added within existing functional areas following established patterns.

---

## Detailed File Organization

### Core/
**Purpose**: Fundamental types, base interfaces, core MCP features, and protocol infrastructure.

#### Core/Types/
- `Common.hpp` *(moved from Core/Common.hpp)* - Basic common types and includes
- `Constants.hpp` *(moved from Core/Types/Constants.hpp)* - System-wide constants
- `Error.hpp` *(moved from Core/Types/Error.hpp)* - Base error types
- `Result.hpp` *(moved from Core/Types/Result.hpp)* - Result/Option type utilities
- `Implementation.hpp` *(moved from Core/Types/Implementation.hpp)* - Implementation metadata
- `JSON_RPC.hpp` *(moved from Core/Types/JSON_RPC.hpp)* - JSON-RPC protocol definitions
- `Request.hpp` *(moved from Core/Types/Request.hpp)* - Request functional entities
- `Notification.hpp` *(moved from Core/Types/Notification.hpp)* - Notification functional entities

#### Core/Schemas/
- `Capabilities.hpp` *(moved from Core/Types/Capabilities.hpp)* - Capability data structures

#### Core/Features/Concepts/
- `Tool.hpp` *(moved from Core/Types/Tool.hpp)* - MCP Tool concept
- `Resource.hpp` *(moved from Core/Types/Resource.hpp)* - MCP Resource concept
- `Prompt.hpp` *(moved from Core/Types/Prompt.hpp)* - MCP Prompt concept
- `Content.hpp` *(moved from Core/Types/Content.hpp)* - MCP Content concept
- `Log.hpp` *(moved from Core/Types/Log.hpp)* - MCP Log concept
- `Sampling.hpp` *(moved from Core/Types/Sampling.hpp)* - MCP Sampling concept
- `Root.hpp` *(moved from Core/Types/Root.hpp)* - MCP Root concept

#### Core/Protocols/
- `Protocol.hpp` *(split from Core/Protocol.hpp)* - Base protocol interface definition
- `MCPProtocol.hpp` *(extracted from Core/Protocol.hpp)* - MCP-specific protocol implementation

### Communication/
**Purpose**: All networking, transport, and message serialization functionality.

#### Communication/Transport/
- `Transport.hpp` *(moved from Transport/Transport.hpp)* - Base transport interface
- `InMemory.hpp` *(moved from Transport/InMemory.hpp)* - In-memory transport
- `Stdio.hpp` *(moved from Transport/Stdio.hpp)* - Standard I/O transport
- `Websocket.hpp` *(moved from Transport/Websocket.hpp)* - WebSocket transport
- `SSE.hpp` *(moved from Transport/SSE.hpp)* - Server-Sent Events transport
- `StreamableHTTP.hpp` *(moved from Transport/StreamableHTTP.hpp)* - HTTP streaming transport

#### Communication/Serialization/
- `JSON.hpp` *(moved from Utilities/JSON/JSON.hpp)* - JSON serialization
- `JSON_FWD.hpp` *(moved from Utilities/JSON/JSON_FWD.hpp)* - JSON forward declarations

#### Communication/Protocols/
- `JSON_RPC_Utilities.hpp` *(moved from Utilities/JSON/JSON_RPC_Utilities.hpp)* - JSON-RPC protocol utilities

#### Communication/Utilities/
- `URI_Template.hpp` *(moved from Utilities/URI/URI_Template.hpp)* - URI template utilities

### Authentication/
**Purpose**: Complete authentication and authorization system.

#### Authentication/Types/
- `Auth.hpp` *(moved from Auth/Auth.hpp)* - Basic auth types

#### Authentication/Providers/
- `Provider.hpp` *(moved from Auth/Auth_Server/Provider.hpp)* - Base auth provider
- `ProxyProvider.hpp` *(moved from Auth/Auth_Server/providers/ProxyProvider.hpp)* - Proxy authentication provider

#### Authentication/Client/
- `AuthClient.hpp` *(moved from Auth/Auth_Client.hpp)* - Client-side authentication

#### Authentication/Server/
- `AuthServer.hpp` *(consolidated from Auth/Auth_Server/ main files)* - Server-side authentication
- `Router.hpp` *(moved from Auth/Auth_Server/Router.hpp)* - Authentication routing
- `Clients.hpp` *(moved from Auth/Auth_Server/Clients.hpp)* - Client management
- `AuthErrors.hpp` *(moved from Auth/Auth_Server/AuthErrors.hpp)* - Auth-specific errors

#### Authentication/Utilities/
- `Handlers.hpp` *(consolidated from Auth/Auth_Server/handlers/)* - Auth handlers (Token, Revoke, Register, Metadata, Authorize)
- `Middleware.hpp` *(consolidated from Auth/Auth_Server/middleware/)* - Auth middleware (ClientAuth_MW, AllowedMethods, BearerAuth)

### Client/
**Purpose**: Client-side SDK functionality.

#### Client/Core/
- `MCPClient.hpp` *(moved from Core/Client/Client.hpp)* - Main client class
- `ClientCapabilities.hpp` *(extracted from Core/Schemas/Capabilities.hpp for client-specific parts)*

### Server/
**Purpose**: Server-side SDK functionality.

#### Server/Core/
- `MCPServer.hpp` *(moved from Core/Server/ServerMCP.hpp)* - Main MCP server class  
- `Server.hpp` *(moved from Core/Server/Server.hpp)* - Base server class
- `ServerCapabilities.hpp` *(extracted from Core/Schemas/Capabilities.hpp for server-specific parts)*

### Utilities/
**Purpose**: General utilities, CLI tools, and helper functions.

#### Utilities/CLI/
- `CLI.hpp` *(moved from CLI.hpp)* - Main CLI interface

#### Utilities/JSON/
- Documentation files *(ZodToJSON.md, NLOHMANN_JSON_README.md, LICENSE.md remain)*

#### Utilities/URI/
- *(Currently empty after URI_Template.hpp moved to Communication/Utilities/)*

#### Utilities/Validation/
- *(New folder for validation implementations)*

#### Utilities/Lux/
- *(README.md remains, appears to be placeholder)*

#### Utilities/ (root level)
- `Completable.hpp` *(moved from Utilities/Completable.hpp)* - Completion utilities

---

## File Movement Summary

### Files to Move (No Content Changes)
- **Transport files**: `Transport/` → `Communication/Transport/`
- **JSON utilities**: `Utilities/JSON/JSON.hpp`, `JSON_FWD.hpp` → `Communication/Serialization/`
- **JSON-RPC utilities**: `Utilities/JSON/JSON_RPC_Utilities.hpp` → `Communication/Protocols/`
- **URI utilities**: `Utilities/URI/URI_Template.hpp` → `Communication/Utilities/`
- **Auth files**: Various `Auth/` restructuring to `Authentication/` subfolders
- **Client/Server**: `Core/Client/`, `Core/Server/` → `Client/Core/`, `Server/Core/`
- **MCP concepts**: `Core/Types/Tool.hpp`, `Resource.hpp`, etc. → `Core/Features/Concepts/`
- **CLI**: `CLI.hpp` → `Utilities/CLI/CLI.hpp`

### Files to Split/Refactor
- **`Core/Protocol.hpp`** (897 lines) → Split into:
  - `Core/Protocols/Protocol.hpp` (base interface)
  - `Core/Protocols/MCPProtocol.hpp` (implementation)
- **`Core/Types/Capabilities.hpp`** → Split into:
  - `Core/Schemas/Capabilities.hpp` (base capability structures)
  - `Client/Core/ClientCapabilities.hpp` (client-specific)
  - `Server/Core/ServerCapabilities.hpp` (server-specific)

### Files to Consolidate
- **`Auth/Auth_Server/handlers/`** (Token.hpp, Revoke.hpp, Register.hpp, Metadata.hpp, Authorize.hpp) → 
  - `Authentication/Utilities/Handlers.hpp`
- **`Auth/Auth_Server/middleware/`** (ClientAuth_MW.hpp, AllowedMethods.hpp, BearerAuth.hpp) → 
  - `Authentication/Utilities/Middleware.hpp`

### Files to Keep in Place (Minimal Changes)
- Most `Core/Types/` fundamental files just move to new locations
- Transport implementations remain largely unchanged
- Utilities/JSON/ documentation files remain
- Test files (*.test.ts) remain in current locations (not part of Public API)

---

## Migration Benefits

### Improved Navigation
- **Functional clarity**: Each folder has a single, clear purpose
- **MCP concepts centralized**: All core MCP domain concepts in one discoverable location
- **Logical grouping**: Related functionality is co-located

### Better Maintainability
- **Conservative changes**: Most files just move locations, minimal content changes
- **Clear domain separation**: MCP concepts, communication, authentication clearly separated
- **Reduced file size**: Large files split into focused, manageable pieces

### Enhanced Developer Experience
- **Self-documenting**: Structure immediately communicates organization
- **Acronym consistency**: Proper capitalization throughout (MCP, JSON, URI, etc.)
- **Extensible design**: Easy to add new MCP concepts, auth providers, transport types, etc.
