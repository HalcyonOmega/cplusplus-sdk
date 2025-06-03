# Phase 2: Protocol Core Implementation

## 1. JSON-RPC 2.0 Engine

- [ ] Implement robust JSON-RPC 2.0 message parsing, validation, and dispatch
    - [ ] Parse and validate all incoming JSON-RPC messages (requests, responses, notifications, batches)
    - [ ] Enforce JSON-RPC 2.0 structure: `jsonrpc`, `id`, `method`, `params`, `result`, `error`
    - [ ] Support UTF-8 encoding for all messages
    - [ ] Handle newline-delimited messages for stdio transport
    - [ ] Implement batch message handling (arrays of requests/notifications)
    - [ ] Support all standard JSON-RPC error codes and custom codes > -32000
    - [ ] Provide robust error handling and reporting (invalid requests, parse errors, etc.)
    - [ ] Ensure thread safety and reentrancy where required
    - [ ] Integrate with schema validation for message payloads

## 2. Transport Abstractions

- [ ] Implement stdio transport (process pipes)
    - [ ] Launch server as subprocess and connect via stdin/stdout
    - [ ] Read/write newline-delimited JSON-RPC messages
    - [ ] Handle process lifecycle and error reporting
    - [ ] Support logging via stderr
- [ ] Implement HTTP/Streamable HTTP transport (with SSE support)
    - [ ] Implement HTTP POST endpoint for JSON-RPC messages (single and batch)
    - [ ] Support `Accept: application/json, text/event-stream` headers
    - [ ] Implement SSE (Server-Sent Events) for streaming responses/notifications
    - [ ] Support HTTP GET for opening SSE streams
    - [ ] Handle session management via `Mcp-Session-Id` header
    - [ ] Enforce security: validate `Origin` header, bind to localhost for local servers
    - [ ] Support authentication hooks (for future OAuth 2.1 integration)
    - [ ] Handle resumability and redelivery using SSE `id` and `Last-Event-ID`
- [ ] Design extensible transport interfaces for future protocols
    - [ ] Abstract transport layer to allow easy addition of new protocols
    - [ ] Document connection and message exchange patterns for custom transports

## 3. Lifecycle Management

- [ ] Implement initialization handshake (`initialize`, `initialized`)
    - [ ] Client sends `initialize` request with protocol version, capabilities, and client info
    - [ ] Server responds with `InitializeResult` (protocol version, capabilities, server info)
    - [ ] Client sends `initialized` notification after successful handshake
    - [ ] Enforce: `initialize` request MUST NOT be part of a batch
    - [ ] Restrict requests before initialization is complete (except `ping`)
- [ ] Support version and capability negotiation
    - [ ] Exchange `ClientCapabilities` and `ServerCapabilities` during initialization
    - [ ] Negotiate protocol version; disconnect if no compatible version
    - [ ] Dynamically enable/disable features based on negotiated capabilities
- [ ] Handle clean shutdown and timeouts
    - [ ] Implement clean shutdown for stdio (close stdin, send SIGTERM/SIGKILL if needed)
    - [ ] Implement clean shutdown for HTTP (close connections, send HTTP DELETE with session ID)
    - [ ] Set and enforce configurable timeouts for all requests
    - [ ] Issue `notifications/cancelled` if a request times out
    - [ ] Allow timeouts to be reset by `notifications/progress`, but enforce a maximum
    - [ ] Document shutdown and timeout behavior for all transports 