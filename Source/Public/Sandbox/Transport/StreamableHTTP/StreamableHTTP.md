# StreamableHTTP Architecture

This document defines the method requirements for the MCP Streamable HTTP transport implementation according to the [MCP Specification](../../../References/Model%20Context%20Protocol/Spec/Basic/Transports.mdx).

## Transport Boundary

The StreamableHTTP transport takes **MessageBase** types (MCP contract data) and converts them to/from HTTP format. The transport handles all HTTP-specific operations while maintaining the MCP interface that only inputs/outputs MessageBase types.

## Base Class Methods (StreamableHTTPBase)

The base class handles **ALL** HTTP-related operations, serialization, and transport mechanics.

### Core Transport Interface (ITransport)
- `Connect()` - Establishes transport connection
- `Disconnect()` - Closes transport connection  
- `SendMessage(const MessageBase&)` - Sends a single JSON-RPC message via HTTP
- `SendBatch(const JSONRPCBatch&)` - Sends batched JSON-RPC messages via HTTP
- `SetMessageHandler(function<void(const MessageBase&)>)` - Sets callback for incoming messages
- `SetErrorHandler(function<void(const string&)>)` - Sets callback for errors
- `IsConnected() const` - Returns connection status
- `GetTransportType() const` - Returns TransportType::HTTP

### HTTP Operations (All HTTP mechanics)
- `CreateHTTPRequest(Method, const string& Endpoint)` - Creates HTTP request for any method
- `SendHTTPRequest(const HTTPRequest&, const optional<MessageBase>&)` - Sends HTTP request with optional message body
- `HandleHTTPResponse(const HTTPResponse&)` - Processes any HTTP response according to MCP spec
- `ProcessIncomingHTTPRequest(const HTTPRequest&, HTTPResponse&)` - Processes incoming HTTP requests (server side)

### Message Serialization/Deserialization
- `SerializeMessageToHTTP(const MessageBase&)` - Converts MessageBase to HTTP request/response body
- `DeserializeHTTPToMessage(const string& HTTPBody)` - Converts HTTP body to MessageBase
- `DetermineMessageType(const MessageBase&)` - Determines if message contains requests, notifications, or responses

### Header Management (All header operations)
- `AddRequiredHeaders(HTTPRequest&)` - Adds all MCP-required headers (Accept, etc.)
- `AddSessionHeader(HTTPRequest&)` - Adds Mcp-Session-Id if session exists
- `AddResumabilityHeaders(HTTPRequest&)` - Adds Last-Event-ID and other resumability headers
- `ExtractHeadersFromRequest(const HTTPRequest&)` - Extracts relevant headers from incoming requests
- `ValidateRequiredHeaders(const HTTPRequest&)` - Validates incoming requests have required headers

### Session Management (All session operations)
- `CreateSession(const string& SessionID)` - Creates new session with ID
- `InvalidateSession()` - Marks current session as invalid
- `UpdateSessionActivity()` - Updates last activity timestamp
- `HasValidSession() const` - Checks if session exists and is active
- `GetSessionID() const` - Returns current session ID (optional)
- `ValidateSessionFromRequest(const HTTPRequest&)` - Validates session from incoming request
- `IsStatefulMode() const` - Checks if transport is in stateful mode

### SSE Stream Management (All SSE operations)
- `FormatSSEEvent(const MessageBase&, const optional<string>& EventID)` - Formats MessageBase as SSE event
- `ParseSSEEvent(const string& SSEData)` - Parses SSE data back to MessageBase
- `StartSSEStream(HTTPResponse&)` - Initiates SSE stream response
- `WriteMessageToSSEStream(HTTPResponse&, const MessageBase&)` - Writes MessageBase to SSE stream
- `ProcessSSEStreamData(const string& StreamData)` - Processes incoming SSE stream data

### Error Handling (Generic HTTP error handling)
- `HandleHTTPError(const HTTPResponse&)` - Generic HTTP error handler
- `HandleTransportError(const string& ErrorMessage)` - Generic transport error handler
- `CallErrorHandler(const string&)` - Safely invokes configured error handler
- `CallMessageHandler(const MessageBase&)` - Safely invokes configured message handler

### Resumability Support (EventStore integration)
- `SetEventStore(shared_ptr<EventStore>)` - Sets event store for resumability
- `StoreEventForResumability(const MessageBase&)` - Stores event with ID for replay
- `ReplayEventsAfter(const string& LastEventID)` - Replays stored events for resumability

### HTTP Method Handling (Generic method support)
- `HandlePOSTRequest(const HTTPRequest&, HTTPResponse&)` - Handles POST with JSON-RPC content
- `HandleGETRequest(const HTTPRequest&, HTTPResponse&)` - Handles GET for SSE streams
- `HandleDELETERequest(const HTTPRequest&, HTTPResponse&)` - Handles DELETE for session termination
- `HandleUnsupportedMethod(HTTPResponse&)` - Returns 405 for unsupported methods

### HTTP Status Code Management
- `SetHTTPStatus(HTTPResponse&, StatusCode, const string& Message)` - Sets HTTP status and message
- `HandleStatusCode(StatusCode)` - Processes status codes according to MCP spec (202, 404, 405, etc.)

### Reconnection Support (Client-side)
- `ShouldAttemptReconnection()` - Determines if reconnection should be attempted
- `CalculateReconnectionDelay(int Attempts)` - Calculates exponential backoff delay
- `AttemptReconnection()` - Performs reconnection attempt

### CORS and Security
- `AddCORSHeaders(HTTPResponse&, const HTTPRequest&)` - Adds CORS headers if enabled
- `ValidateOrigin(const string& Origin)` - Validates origin against allowed list
- `ValidateRequestSecurity(const HTTPRequest&)` - Performs security validation

### Utility Methods
- `GenerateEventID()` - Generates unique event ID for SSE
- `GenerateSessionID()` - Generates session ID using configured generator
- `IsInitializationMessage(const MessageBase&)` - Checks if message is MCP initialize
- `ConvertBatchToMessage(const JSONRPCBatch&)` - Converts batch to MessageBase

## Client Class Methods (StreamableHTTPClient)

The client class handles **ONLY** client-specific transport behavior.

### Client-Specific Operations
- `InitializeConnection()` - Client-specific connection initialization sequence
- `StartMessageReceiving()` - Starts receiving messages from server (GET SSE stream)
- `StopMessageReceiving()` - Stops receiving messages from server

### Client Connection Management
- `ManageClientSession()` - Handles client session lifecycle
- `HandleConnectionLoss()` - Client-specific reconnection logic
- `TerminateClientSession()` - Client-initiated session termination

## Server Class Methods (StreamableHTTPServer)

The server class handles **ONLY** server-specific transport behavior.

### Server-Specific Operations
- `StartHTTPServer()` - Starts HTTP server to accept connections
- `StopHTTPServer()` - Stops HTTP server
- `HandleIncomingConnection()` - Server-specific connection handling

### Server Connection Management
- `ManageServerSessions()` - Handles multiple client sessions
- `HandleClientDisconnection()` - Server-side cleanup when client disconnects
- `BroadcastToClients(const MessageBase&)` - Sends message to connected clients

### Server Request Routing
- `RouteIncomingRequest(const HTTPRequest&, HTTPResponse&)` - Routes requests to base class handlers
- `DetermineResponseType(const MessageBase&)` - Decides between JSON response or SSE stream

## MCP Specification Compliance

### Required HTTP Headers
- `Accept: application/json, text/event-stream` (Client requests)
- `Accept: text/event-stream` (Client GET requests)
- `Mcp-Session-Id` (Session management)
- `Last-Event-ID` (Resumability)

### Required HTTP Status Codes
- `200 OK` - Successful response or SSE stream
- `202 Accepted` - Accepted notification/response
- `400 Bad Request` - Invalid request or missing session
- `404 Not Found` - Invalid session
- `405 Method Not Allowed` - Unsupported method

### Required Behaviors
- **Message Type Handling**: Differentiate requests (→ SSE/JSON) vs notifications/responses (→ 202)
- **Session Management**: Optional stateful mode with session validation
- **Resumability**: Event replay via EventStore
- **Stream Management**: No message broadcasting across streams
- **Reconnection**: Exponential backoff for clients

### Transport Modes
- **Stateful**: Session ID required, validation enforced
- **Stateless**: No session requirements, simplified operation

