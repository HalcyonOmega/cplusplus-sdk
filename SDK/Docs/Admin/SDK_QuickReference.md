# MCP SDK Quick Reference (C++ Focus)

This document provides a high-level overview of the Model Context Protocol (MCP) to guide the development of a C++ SDK.

## Core Architecture

MCP employs a client-server model:

*   **Hosts**: LLM applications (e.g., IDEs, Claude Desktop) that initiate connections.
*   **Clients**: Reside within host applications, maintaining 1:1 connections with servers.
*   **Servers**: Provide context, tools, and prompts to clients.

**Key Components:**

*   **Protocol Layer**: Manages message framing, request/response linking, and high-level communication.
    *   Core classes to implement: `Protocol`, `Client`, `Server` (or C++ equivalents).
*   **Transport Layer**: Handles actual data exchange between clients and servers. All transports use JSON-RPC 2.0.
    *   **Stdio Transport**: Uses standard input/output. Ideal for local processes.
    *   **HTTP with SSE Transport**: Server-Sent Events for server-to-client, HTTP POST for client-to-server.

**Message Types (JSON-RPC 2.0 based):**

1.  **Requests**: Expect a response. Contain `method` and optional `params`.
2.  **Results**: Successful responses to requests.
3.  **Errors**: Indicate failed requests. Include `code`, `message`, and optional `data`.
    *   Standard JSON-RPC error codes: `ParseError (-32700)`, `InvalidRequest (-32600)`, `MethodNotFound (-32601)`, `InvalidParams (-32602)`, `InternalError (-32603)`. Custom codes > -32000.
4.  **Notifications**: One-way messages. Contain `method` and optional `params`.

**Connection Lifecycle:**

1.  **Initialization**:
    *   Client sends `initialize` request (protocol version, capabilities).
    *   Server responds with its version and capabilities.
    *   Client sends `initialized` notification.
2.  **Message Exchange**: Request-Response and Notifications.
3.  **Termination**: Via `close()`, transport disconnection, or error.

## Key MCP Features

### Resources

*   **Purpose**: Expose data/content from servers to clients for LLM context. Application-controlled.
*   **Identification**: Unique URIs (e.g., `file:///`, `postgres://`, custom schemes).
*   **Types**:
    *   **Text Resources**: UTF-8 encoded (source code, logs, JSON).
    *   **Binary Resources**: Base64 encoded (images, PDFs).
*   **Discovery**:
    *   `resources/list`: Returns a list of `Resource` objects (`uri`, `name`, `description`, `mimeType`).
    *   **Resource Templates**: URI templates (RFC 6570) for dynamic resources.
*   **Reading**: `resources/read` request with URI. Response contains one or more resource `contents` (`uri`, `mimeType`, `text` or `blob`).
*   **Updates**:
    *   `notifications/resources/list_changed`: For changes in available resources.
    *   `resources/subscribe` & `resources/unsubscribe`: For content changes of specific resources. Server sends `notifications/resources/updated`.

### Prompts

*   **Purpose**: Define reusable, user-controlled prompt templates and workflows.
*   **Structure**: `name` (unique ID), `description`, optional `arguments` (list of `name`, `description`, `required`).
*   **Discovery**: `prompts/list` request. Response contains a list of `Prompt` objects.
*   **Usage**: `prompts/get` request with `name` and `arguments`. Response includes `description` and `messages` (conversation history for LLM).
*   **Dynamic Nature**: Can embed resource context and define multi-step workflows.
*   **Updates**: Server capability `prompts.listChanged`, notification `notifications/prompts/list_changed`.

### Tools

*   **Purpose**: Enable LLMs to perform actions through the server (model-controlled, with human approval).
*   **Structure**: `name` (unique ID), `description`, `inputSchema` (JSON Schema for parameters), optional `annotations` (UX hints like `title`, `readOnlyHint`, `destructiveHint`, `idempotentHint`, `openWorldHint`).
*   **Discovery**: `tools/list` request. Response contains a list of `Tool` objects.
*   **Invocation**: `tools/call` request with `name` and `arguments`. Server executes and returns results.
    *   Results: `content` array (text, image, or embedded resource).
    *   Errors: Reported within the result object (`isError: true`, error details in `content`), not as protocol-level errors.
*   **Updates**: `notifications/tools/list_changed`.

### Sampling

*   **Purpose**: Allows servers to request LLM completions through the client (human-in-the-loop). *Note: Not yet supported in Claude Desktop.*
*   **Flow**:
    1.  Server sends `sampling/createMessage` request.
    2.  Client reviews/modifies, samples from LLM.
    3.  Client reviews/modifies completion, returns result to server.
*   **Request Format**:
    *   `messages`: Conversation history (role: `user`|`assistant`, content: text or image).
    *   `modelPreferences`: `hints` (name suggestions), `costPriority`, `speedPriority`, `intelligencePriority`.
    *   `systemPrompt` (optional).
    *   `includeContext`: `"none" | "thisServer" | "allServers"`. Client controls actual inclusion.
    *   Sampling params: `temperature`, `maxTokens`, `stopSequences`, `metadata`.
*   **Response Format**: `model` (name used), `stopReason`, `role`, `content`.

### Roots

*   **Purpose**: URIs suggested by clients to servers, defining operational boundaries (often filesystem paths or URLs).
*   **Function**: Guidance for servers, clarity for workspaces, organization for multiple resources.
*   **Mechanism**:
    *   Client declares `roots` capability.
    *   Client provides a list of suggested roots (e.g., `uri`, `name`).
    *   Client notifies server of changes (if supported).
*   **Server Behavior**: Respect roots, use them for resource location, prioritize operations within root boundaries.

### Transports (Recap)

*   **JSON-RPC 2.0**: Wire format for all messages.
*   **Stdio**: Standard input/output.
    *   Client can spawn server process.
*   **SSE (Server-Sent Events)**: SSE for server-to-client, HTTP POST for client-to-server.
    *   **Security**: Validate `Origin` headers, bind to `localhost` (127.0.0.1) locally, use authentication to prevent DNS rebinding.
*   **Custom Transports**: Can be implemented by conforming to the `Transport` interface (send, receive, close, error handling).

## C++ SDK Considerations

*   **Asynchronous Operations**: MCP is inherently asynchronous. Leverage C++ asynchronous programming models (e.g., `std::async`, `futures`, libraries like Boost.Asio or platform-specific APIs).
*   **JSON Handling**: A robust JSON library will be crucial for serialization/deserialization of JSON-RPC messages and schemas (e.g., RapidJSON, nlohmann/json).
*   **URI Parsing**: A library for URI parsing and manipulation.
*   **Type Safety**: Implement strong typing for message parameters and schemas where possible.
*   **Error Handling**: Consistent and comprehensive error reporting.
*   **Extensibility**: Design for adding custom transports, resource types, etc.
*   **Memory Management**: Careful attention to memory ownership and lifetimes, especially with asynchronous callbacks.
*   **Concurrency**: Thread safety if the SDK will be used in multi-threaded environments.
*   **Build System**: CMake is common for C++ projects.
*   **Testing**: Thorough unit and integration tests.

This quick reference should serve as a starting point. The full MCP specification will provide the detailed message formats, sequencing, and schemas necessary for implementation.
