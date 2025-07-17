# Model Context Protocol (MCP) - SDK Development Reference

This document serves as a comprehensive reference for developing a C++ SDK for the Model Context Protocol (MCP). It distills key information from the official MCP specification files.

## 1. Core Concepts

The Model Context Protocol (MCP) enables integration between LLM applications and external data sources/tools.

### 1.1. Architecture
MCP follows a **client-host-server** architecture:
-   **Host**: The main application process that manages client instances, security, and overall coordination.
-   **Clients**: Reside within the host, each maintaining an isolated, stateful session with a single server. Responsible for protocol negotiation, message routing, and managing subscriptions.
-   **Servers**: Independent processes or services that provide specialized context, resources, tools, and prompts. They operate with focused responsibilities and must respect security constraints imposed by the host/client.

*Reference*: `Docs/Model Context Protocol/Spec/Architecture/Index.mdx`

### 1.2. Communication Protocol
-   All messages **MUST** follow the **JSON-RPC 2.0** specification.
-   Messages are UTF-8 encoded.
-   **Requests**: Client-to-server or server-to-client to initiate an operation.
    -   `jsonrpc: "2.0"`
    -   `id: string | number` (MUST NOT be `null`, MUST be unique per session for the requestor)
    -   `method: string`
    -   `params?: object`
-   **Responses**: Replies to requests.
    -   `jsonrpc: "2.0"`
    -   `id: string | number` (same as request)
    -   `result?: object` OR `error?: object` (MUST include one, not both)
    -   `error` object: `{ code: number, message: string, data?: unknown }`
-   **Notifications**: One-way messages, no response expected.
    -   `jsonrpc: "2.0"`
    -   `method: string`
    -   `params?: object`
    -   MUST NOT include an `id`.
-   **Batching**: Implementations **MAY** support sending batches (arrays of requests/notifications) and **MUST** support receiving them.

*Reference*: `Docs/Model Context Protocol/Spec/Basic/Index.mdx`

### 1.3. Design Principles
1.  **Servers should be easy to build**: Hosts handle complex orchestration.
2.  **Servers should be highly composable**: Focused functionality, interoperable.
3.  **Servers should not see the whole conversation or other servers**: Isolation enforced by the host.
4.  **Progressive feature adoption**: Core protocol is minimal; additional capabilities are negotiated.

*Reference*: `Docs/Model Context Protocol/Spec/Architecture/Index.mdx`

### 1.4. Schema
-   The authoritative definition of the protocol is the **TypeScript schema**: `Docs/Model Context Protocol/Spec/Schema/Schema.ts`.
-   A **JSON Schema** is auto-generated from the TypeScript schema: `Docs/Model Context Protocol/Spec/Schema/Schema.json`.
    These schemas are the source of truth for all message structures and data types.

*Reference*: `Docs/Model Context Protocol/Spec/Basic/Index.mdx`

## 2. Lifecycle Management

Connections follow a defined lifecycle: Initialization, Operation, Shutdown.

### 2.1. Initialization
The first interaction between client and server.
1.  Client sends `initialize` request:
    -   `protocolVersion`: Latest supported by client.
    -   `capabilities`: `ClientCapabilities` object.
    -   `clientInfo`: `Implementation` object (`name`, `version`).
    -   The `initialize` request **MUST NOT** be part of a JSON-RPC batch.
2.  Server responds with `InitializeResult`:
    -   `protocolVersion`: Version server wants to use (may differ from client's request).
    -   `capabilities`: `ServerCapabilities` object.
    -   `serverInfo`: `Implementation` object.
    -   `instructions?`: Optional string for client/LLM hints.
3.  Client sends `initialized` notification after successful initialization.

Client **SHOULD NOT** send requests (other than `ping`) before server responds to `initialize`.
Server **SHOULD NOT** send requests (other than `ping` and logging) before receiving `initialized` notification.

*Reference*: `Docs/Model Context Protocol/Spec/Basic/Lifecycle.mdx`

### 2.2. Version Negotiation
-   Client sends its latest supported `protocolVersion`.
-   If server supports it, responds with the same version.
-   Else, server responds with another (latest) version it supports.
-   If client doesn't support server's chosen version, it **SHOULD** disconnect.
    The current latest protocol version is **2025-03-26**.

*Reference*: `Docs/Model Context Protocol/Spec/Basic/Lifecycle.mdx`

### 2.3. Capability Negotiation
-   Clients and servers exchange `ClientCapabilities` and `ServerCapabilities` objects during initialization.
-   These objects define which optional protocol features are available for the session.
-   Examples: `roots`, `sampling` (client); `prompts`, `resources`, `tools`, `logging`, `completions` (server).
-   Sub-capabilities like `listChanged` (for prompts, resources, tools) and `subscribe` (for resources) indicate support for specific notifications or features.

*Reference*: `Docs/Model Context Protocol/Spec/Basic/Lifecycle.mdx`, `Docs/Model Context Protocol/Spec/Schema/Schema.ts` (for capability structures)

### 2.4. Operation
-   Client and server exchange messages according to negotiated capabilities and protocol version.

### 2.5. Shutdown
-   Clean termination of the connection, usually client-initiated.
-   No specific MCP messages; uses underlying transport mechanism.
    -   **stdio**: Client closes `stdin` to server, then `SIGTERM`/`SIGKILL` if needed. Server may exit and close `stdout`.
    -   **HTTP**: Close the associated HTTP connection(s). For Streamable HTTP with sessions, client **SHOULD** send HTTP DELETE with `Mcp-Session-Id`.

*Reference*: `Docs/Model Context Protocol/Spec/Basic/Lifecycle.mdx`

### 2.6. Timeouts
-   Implementations **SHOULD** set timeouts for all sent requests.
-   If no response, sender **SHOULD** issue a `notifications/cancelled` and stop waiting.
-   SDKs **SHOULD** allow configurable timeouts.
-   Timeouts **MAY** be reset on receiving a `notifications/progress`, but a maximum timeout **SHOULD** still be enforced.

*Reference*: `Docs/Model Context Protocol/Spec/Basic/Lifecycle.mdx`

## 3. Transports

MCP messages (JSON-RPC) are UTF-8 encoded.

### 3.1. stdio
-   Client launches server as a subprocess.
-   Server reads JSON-RPC from `stdin`, writes to `stdout`.
-   Messages are newline-delimited, **MUST NOT** contain embedded newlines.
-   Server **MAY** use `stderr` for UTF-8 log strings.
-   Clients **SHOULD** support stdio.

*Reference*: `Docs/Model Context Protocol/Spec/Basic/Transports.mdx`

### 3.2. Streamable HTTP
-   Replaces the older HTTP+SSE transport.
-   Server operates as an independent process.
-   Single HTTP endpoint path (MCP endpoint) for POST and GET.
-   **Security**:
    -   Servers **MUST** validate `Origin` header.
    -   Local servers **SHOULD** bind to localhost (127.0.0.1).
    -   Servers **SHOULD** implement authentication.

#### 3.2.1. Sending Messages to Server (Client -> Server)
-   Client **MUST** use HTTP POST to MCP endpoint.
-   Client **MUST** include `Accept: application/json, text/event-stream`.
-   POST body: single JSON-RPC message or a batch.
-   If input is only responses/notifications: server returns HTTP 202 Accepted (no body) on success, or error status.
-   If input contains requests: server returns `Content-Type: text/event-stream` (SSE) or `application/json` (single object). Client **MUST** support both.
-   If SSE stream:
    -   **SHOULD** eventually include one JSON-RPC response per request.
    -   **MAY** send requests/notifications before responses (related to original client request).
    -   **SHOULD NOT** close stream before all responses sent (unless session expires).
    -   **SHOULD** close stream after all responses sent.

#### 3.2.2. Listening for Messages from Server (Server -> Client)
-   Client **MAY** issue HTTP GET to MCP endpoint to open an SSE stream.
-   Client **MUST** include `Accept: text/event-stream`.
-   Server **MUST** return `Content-Type: text/event-stream` or HTTP 405 Method Not Allowed.
-   If SSE stream:
    -   Server **MAY** send requests/notifications (usually unrelated to concurrent client requests).
    -   **MUST NOT** send responses unless resuming a stream.
    -   Server/Client **MAY** close stream anytime.

#### 3.2.3. Multiple Connections
-   Client **MAY** have multiple SSE streams.
-   Server **MUST** send each message on only one stream (no broadcasting).

#### 3.2.4. Resumability and Redelivery (SSE)
-   Servers **MAY** attach an `id` field to SSE events (globally unique per session/client).
-   Client **SHOULD** use `Last-Event-ID` header on GET to resume.
-   Server **MAY** replay messages for that stream.

#### 3.2.5. Session Management (Streamable HTTP)
-   Server **MAY** assign session ID via `Mcp-Session-Id` header in `InitializeResult` response.
    -   Session ID: globally unique, secure, visible ASCII.
-   Client **MUST** include `Mcp-Session-Id` header in subsequent requests if received.
-   Server **MAY** terminate session (respond 404 to requests with that ID). Client **MUST** re-initialize.
-   Client **SHOULD** send HTTP DELETE to MCP endpoint with `Mcp-Session-Id` to terminate session. Server **MAY** respond 405.

*Reference*: `Docs/Model Context Protocol/Spec/Basic/Transports.mdx`

### 3.3. Custom Transports
-   Allowed, **MUST** preserve JSON-RPC format and lifecycle.
-   **SHOULD** document connection and message exchange patterns.

*Reference*: `Docs/Model Context Protocol/Spec/Basic/Transports.mdx`

## 4. Authorization (for HTTP-based transports)

Authorization is **OPTIONAL**. Based on OAuth 2.1.
-   HTTP-based transports **SHOULD** conform.
-   STDIO transports **SHOULD NOT** (use environment variables for credentials).

### 4.1. Overview
1.  **OAuth 2.1**: Implementations **MUST** use OAuth 2.1.
2.  **Dynamic Client Registration (RFC7591)**: Implementations **SHOULD** support.
3.  **Server Metadata (RFC8414)**: Servers **SHOULD** and clients **MUST** implement. Servers not supporting metadata **MUST** use default URI schema.

### 4.2. OAuth Grant Types
-   Servers **SHOULD** support grants like:
    -   **Authorization Code**: For user-acting clients (e.g., agent calls SaaS tool).
    -   **Client Credentials**: For application clients (e.g., agent checks store inventory).

### 4.3. Example Flow (Authorization Code Grant with PKCE)
1.  Client makes MCP request.
2.  Server responds HTTP 401 Unauthorized.
3.  Client generates `code_verifier` and `code_challenge`.
4.  Client opens browser with authorization URL + `code_challenge`.
5.  User logs in and authorizes on Authorization Server.
6.  Auth Server redirects to client's callback URL with auth `code`.
7.  Client exchanges `code` + `code_verifier` for Access Token (and possibly Refresh Token) with Auth Server's token endpoint.
8.  Client retries MCP request with Access Token in `Authorization: Bearer <token>` header.

*Reference*: `Docs/Model Context Protocol/Spec/Basic/Authorization.mdx`

### 4.4. Server Metadata Discovery
-   Clients **MUST** follow RFC8414. Servers **SHOULD**.
-   Discovery endpoint: `/.well-known/oauth-authorization-server` relative to the **authorization base URL**.
-   Authorization Base URL: MCP server URL with path component discarded (e.g., `https://api.example.com/v1/mcp` -> `https://api.example.com`).
-   Clients **SHOULD** send `MCP-Protocol-Version` header during discovery.
-   **Fallback for servers without metadata**: Clients **MUST** use default paths relative to auth base URL:
    -   Authorization Endpoint: `/authorize`
    -   Token Endpoint: `/token`
    -   Registration Endpoint: `/register`

*Reference*: `Docs/Model Context Protocol/Spec/Basic/Authorization.mdx`

### 4.5. Dynamic Client Registration (RFC7591)
-   Clients and servers **SHOULD** support this for automatic client ID acquisition.
-   Alternative for servers not supporting it: hardcoded IDs or manual user registration UI.

*Reference*: `Docs/Model Context Protocol/Spec/Basic/Authorization.mdx`

### 4.6. Access Token Usage
-   **MUST** conform to OAuth 2.1 Section 5.
-   Client **MUST** use `Authorization: Bearer <access-token>` header.
-   Tokens **MUST NOT** be in URI query string.
-   Servers **MUST** validate tokens; respond HTTP 401 for invalid/expired.

*Reference*: `Docs/Model Context Protocol/Spec/Basic/Authorization.mdx`

### 4.7. Security Considerations (Auth)
-   Secure token storage by clients.
-   Token expiration and rotation by servers.
-   HTTPS for all auth endpoints.
-   Redirect URI validation (must be localhost or HTTPS).
-   PKCE is **REQUIRED** for all clients.

*Reference*: `Docs/Model Context Protocol/Spec/Basic/Authorization.mdx`

### 4.8. Third-Party Authorization Flow
-   MCP server **MAY** delegate auth to a third-party auth server.
-   MCP server acts as OAuth client to third-party and OAuth server to MCP client.
-   Requires secure mapping and lifecycle management of tokens.

*Reference*: `Docs/Model Context Protocol/Spec/Basic/Authorization.mdx`

## 5. Server Features

Features offered by servers to clients.

### 5.1. Resources
Provide contextual data (files, DB schemas, etc.). Each resource identified by a URI. Application-driven.

#### 5.1.1. Capabilities
-   Server declares `resources` capability:
    -   `subscribe?: boolean`: Support for individual resource change notifications.
    -   `listChanged?: boolean`: Support for notifications when the list of available resources changes.

*Reference*: `Docs/Model Context Protocol/Spec/Server/Resources.mdx`

#### 5.1.2. Operations
-   **List Resources**: `resources/list` request (paginated).
    -   Params: `cursor?: string`.
    -   Result: `{ resources: Resource[], nextCursor?: string }`.
-   **Read Resource**: `resources/read` request.
    -   Params: `{ uri: string }`.
    -   Result: `{ contents: (TextResourceContents | BlobResourceContents)[] }`.
-   **List Resource Templates**: `resources/templates/list` request (paginated).
    -   Params: `cursor?: string`.
    -   Result: `{ resourceTemplates: ResourceTemplate[], nextCursor?: string }`.
-   **Subscribe to Resource**: `resources/subscribe` request (if `subscribe` capable).
    -   Params: `{ uri: string }`.
    -   Result: Empty.
-   **Unsubscribe from Resource**: `resources/unsubscribe` request.
    -   Params: `{ uri: string }`.
    -   Result: Empty.

*Reference*: `Docs/Model Context Protocol/Spec/Server/Resources.mdx`, `Docs/Model Context Protocol/Spec/Schema/Schema.ts`

#### 5.1.3. Notifications (Server to Client)
-   **Resource List Changed**: `notifications/resources/list_changed` (if `listChanged` capable).
-   **Resource Updated**: `notifications/resources/updated` (if subscribed).
    -   Params: `{ uri: string }`.

*Reference*: `Docs/Model Context Protocol/Spec/Server/Resources.mdx`

#### 5.1.4. Data Types
-   `Resource`: `{ uri: string, name: string, description?: string, mimeType?: string, annotations?: Annotations, size?: number }`.
-   `ResourceTemplate`: `{ uriTemplate: string, name: string, description?: string, mimeType?: string, annotations?: Annotations }`.
-   `TextResourceContents`: `{ uri: string, mimeType?: string, text: string }`.
-   `BlobResourceContents`: `{ uri: string, mimeType?: string, blob: string (base64) }`.
-   Common URI schemes: `https://`, `file://`, `git://`. Custom schemes allowed.

*Reference*: `Docs/Model Context Protocol/Spec/Server/Resources.mdx`, `Docs/Model Context Protocol/Spec/Schema/Schema.ts`

### 5.2. Prompts
Pre-defined templates or instructions for LLM interactions. User-controlled.

#### 5.2.1. Capabilities
-   Server declares `prompts` capability:
    -   `listChanged?: boolean`: Support for notifications when the prompt list changes.

*Reference*: `Docs/Model Context Protocol/Spec/Server/Prompts.mdx`

#### 5.2.2. Operations
-   **List Prompts**: `prompts/list` request (paginated).
    -   Params: `cursor?: string`.
    -   Result: `{ prompts: Prompt[], nextCursor?: string }`.
-   **Get Prompt**: `prompts/get` request.
    -   Params: `{ name: string, arguments?: { [key: string]: string } }`.
    -   Result: `{ description?: string, messages: PromptMessage[] }`. Arguments can be auto-completed via `completion/complete`.

*Reference*: `Docs/Model Context Protocol/Spec/Server/Prompts.mdx`, `Docs/Model Context Protocol/Spec/Schema/Schema.ts`

#### 5.2.3. Notifications (Server to Client)
-   **Prompt List Changed**: `notifications/prompts/list_changed` (if `listChanged` capable).

*Reference*: `Docs/Model Context Protocol/Spec/Server/Prompts.mdx`

#### 5.2.4. Data Types
-   `Prompt`: `{ name: string, description?: string, arguments?: PromptArgument[] }`.
-   `PromptArgument`: `{ name: string, description?: string, required?: boolean }`.
-   `PromptMessage`: `{ role: Role, content: TextContent | ImageContent | AudioContent | EmbeddedResource }`.
    -   `Role`: `"user" | "assistant"`.
    -   `TextContent`: `{ type: "text", text: string, annotations?: Annotations }`.
    -   `ImageContent`: `{ type: "image", data: string (base64), mimeType: string, annotations?: Annotations }`.
    -   `AudioContent`: `{ type: "audio", data: string (base64), mimeType: string, annotations?: Annotations }`.
    -   `EmbeddedResource`: `{ type: "resource", resource: TextResourceContents | BlobResourceContents, annotations?: Annotations }`.
-   `Annotations`: `{ audience?: Role[], priority?: number (0-1) }`.

*Reference*: `Docs/Model Context Protocol/Spec/Server/Prompts.mdx`, `Docs/Model Context Protocol/Spec/Schema/Schema.ts`

### 5.3. Tools
Executable functions for LLMs to interact with external systems. Model-controlled. Human-in-the-loop for approval is **STRONGLY RECOMMENDED**.

#### 5.3.1. Capabilities
-   Server declares `tools` capability:
    -   `listChanged?: boolean`: Support for notifications when the tool list changes.

*Reference*: `Docs/Model Context Protocol/Spec/Server/Tools.mdx`

#### 5.3.2. Operations
-   **List Tools**: `tools/list` request (paginated).
    -   Params: `cursor?: string`.
    -   Result: `{ tools: Tool[], nextCursor?: string }`.
-   **Call Tool**: `tools/call` request.
    -   Params: `{ name: string, arguments?: { [key: string]: unknown } }`.
    -   Result: `CallToolResult`: `{ content: (TextContent | ImageContent | AudioContent | EmbeddedResource)[], isError?: boolean }`.
        -   Tool execution errors reported in `CallToolResult` (`isError: true`).
        -   Protocol errors (e.g., unknown tool) reported as JSON-RPC errors.

*Reference*: `Docs/Model Context Protocol/Spec/Server/Tools.mdx`, `Docs/Model Context Protocol/Spec/Schema/Schema.ts`

#### 5.3.3. Notifications (Server to Client)
-   **Tool List Changed**: `notifications/tools/list_changed` (if `listChanged` capable).

*Reference*: `Docs/Model Context Protocol/Spec/Server/Tools.mdx`

#### 5.3.4. Data Types
-   `Tool`: `{ name: string, description?: string, inputSchema: JSONSchemaObject, annotations?: ToolAnnotations }`.
    -   `inputSchema`: `{ type: "object", properties?: { [key: string]: object }, required?: string[] }`.
-   `ToolAnnotations`: Hints about tool behavior (e.g., `title`, `readOnlyHint`, `destructiveHint`, `idempotentHint`, `openWorldHint`). Clients **MUST** consider these untrusted unless from a trusted server.

*Reference*: `Docs/Model Context Protocol/Spec/Server/Tools.mdx`, `Docs/Model Context Protocol/Spec/Schema/Schema.ts`

## 6. Client Features

Features offered by clients to servers.

### 6.1. Roots
Expose filesystem "roots" (directories/files) to servers, defining access boundaries.

#### 6.1.1. Capabilities
-   Client declares `roots` capability:
    -   `listChanged?: boolean`: Client emits notifications if root list changes.

*Reference*: `Docs/Model Context Protocol/Spec/Client/Roots.mdx`

#### 6.1.2. Operations (Server to Client)
-   **List Roots**: `roots/list` request.
    -   Result: `{ roots: Root[] }`.

*Reference*: `Docs/Model Context Protocol/Spec/Client/Roots.mdx`, `Docs/Model Context Protocol/Spec/Schema/Schema.ts`

#### 6.1.3. Notifications (Client to Server)
-   **Root List Changed**: `notifications/roots/list_changed` (if `listChanged` capable).

*Reference*: `Docs/Model Context Protocol/Spec/Client/Roots.mdx`

#### 6.1.4. Data Types
-   `Root`: `{ uri: string (must be file://), name?: string }`.

*Reference*: `Docs/Model Context Protocol/Spec/Client/Roots.mdx`, `Docs/Model Context Protocol/Spec/Schema/Schema.ts`

### 6.2. Sampling
Allows servers to request LLM sampling (completions/generations) via clients. Client controls model access. Human-in-the-loop for approval is **STRONGLY RECOMMENDED**.

#### 6.2.1. Capabilities
-   Client declares `sampling` capability (e.g., `sampling: {}`).

*Reference*: `Docs/Model Context Protocol/Spec/Client/Sampling.mdx`

#### 6.2.2. Operations (Server to Client)
-   **Create Message (Request Sampling)**: `sampling/createMessage` request.
    -   Params: `{ messages: SamplingMessage[], modelPreferences?: ModelPreferences, systemPrompt?: string, includeContext?: "none" | "thisServer" | "allServers", temperature?: number, maxTokens: number, stopSequences?: string[], metadata?: object }`.
    -   Result: `CreateMessageResult`: `{ role: Role, content: TextContent | ImageContent | AudioContent, model: string, stopReason?: string }`.

*Reference*: `Docs/Model Context Protocol/Spec/Client/Sampling.mdx`, `Docs/Model Context Protocol/Spec/Schema/Schema.ts`

#### 6.2.3. Data Types
-   `SamplingMessage`: `{ role: Role, content: TextContent | ImageContent | AudioContent }`.
-   `ModelPreferences`: `{ hints?: ModelHint[], costPriority?: number (0-1), speedPriority?: number (0-1), intelligencePriority?: number (0-1) }`.
    -   Hints are advisory substrings for model names (e.g., "claude-3-sonnet", "claude"). Client makes final selection.
-   `ModelHint`: `{ name?: string }`.

*Reference*: `Docs/Model Context Protocol/Spec/Client/Sampling.mdx`, `Docs/Model Context Protocol/Spec/Schema/Schema.ts`

## 7. Utilities

Cross-cutting protocol features.

### 7.1. Ping
-   Optional mechanism to verify connection liveness.
-   Either side sends `ping` request (no params).
-   Receiver **MUST** respond promptly with an empty `result: {}`.
-   Timeout may indicate stale connection.

*Reference*: `Docs/Model Context Protocol/Spec/Basic/Utilities/Ping.mdx`

### 7.2. Progress Tracking
-   Optional for long-running operations.
-   Requestor includes `_meta: { progressToken: string | number }` in request params. Token **MUST** be unique for active requests.
-   Responder **MAY** send `notifications/progress` notifications:
    -   Params: `{ progressToken: string | number, progress: number, total?: number, message?: string }`.
    -   `progress` value **MUST** increase. `total` is optional.

*Reference*: `Docs/Model Context Protocol/Spec/Basic/Utilities/Progress.mdx`

### 7.3. Cancellation
-   Optional cancellation of in-progress requests.
-   Sender issues `notifications/cancelled`:
    -   Params: `{ requestId: string | number, reason?: string }`.
    -   `requestId` **MUST** be for a previously issued, in-progress request in the same direction.
    -   `initialize` request **MUST NOT** be cancelled.
-   Receiver **SHOULD** stop processing, free resources, and **NOT** send a response for the cancelled request.
-   Race conditions are possible; handle gracefully.

*Reference*: `Docs/Model Context Protocol/Spec/Basic/Utilities/Cancellation.mdx`

### 7.4. Logging (Server to Client)
-   Standardized way for servers to send structured logs.
-   Server declares `logging` capability.
-   Log levels follow syslog severity (debug, info, notice, warning, error, critical, alert, emergency).
-   Client **MAY** send `logging/setLevel` request:
    -   Params: `{ level: LoggingLevel }`. Server sends logs at this level and higher.
-   Server sends `notifications/message` (log message):
    -   Params: `{ level: LoggingLevel, logger?: string, data: unknown (JSON-serializable) }`.

*Reference*: `Docs/Model Context Protocol/Spec/Server/Utilities/Logging.mdx`

### 7.5. Completion (Argument Autocomplete)
-   Server offers argument autocompletion for prompts and resource URIs.
-   Server declares `completions` capability.
-   Client sends `completion/complete` request:
    -   Params: `{ ref: PromptReference | ResourceReference, argument: { name: string, value: string } }`.
        -   `PromptReference`: `{ type: "ref/prompt", name: string }`.
        -   `ResourceReference`: `{ type: "ref/resource", uri: string (URI template) }`.
    -   Result: `{ completion: { values: string[] (max 100), total?: number, hasMore?: boolean } }`. Values sorted by relevance.

*Reference*: `Docs/Model Context Protocol/Spec/Server/Utilities/Completion.mdx`

### 7.6. Pagination
-   For list operations that may return large result sets.
-   Opaque cursor-based. Server determines page size.
-   Initial list request has no `cursor`.
-   Server response includes results and `nextCursor?: string`.
-   Client sends subsequent list requests with the received `cursor` in params.
-   Missing `nextCursor` means end of results.
-   Cursors are opaque and **MUST NOT** be parsed or persisted across sessions by client.
-   Supported by: `resources/list`, `resources/templates/list`, `prompts/list`, `tools/list`.

*Reference*: `Docs/Model Context Protocol/Spec/Server/Utilities/Pagination.mdx`

## 8. Security and Trust & Safety

Implementors **MUST** address security and trust carefully.

### 8.1. Key Principles
1.  **User Consent and Control**: Explicit consent for data access and operations. Clear UIs.
2.  **Data Privacy**: Consent before exposing user data. Protect with access controls.
3.  **Tool Safety**: Treat tool execution with caution. User consent before invoking tools. Descriptions/annotations from untrusted servers are untrusted.
4.  **LLM Sampling Controls**: User approval for sampling requests, prompt content, and result visibility.

### 8.2. Implementation Guidelines
-   Build robust consent/authorization flows.
-   Document security implications.
-   Implement access controls and data protections.
-   Follow security best practices.
-   Consider privacy in feature design.

*Reference*: `Docs/Model Context Protocol/Spec/Index.mdx` (Security and Trust & Safety section)

---

This reference should provide a solid foundation for planning and developing a C++ SDK for MCP. Always refer to the specific .mdx files in `Docs/Model Context Protocol/Spec/` and the `Schema.ts` / `Schema.json` for authoritative details.
