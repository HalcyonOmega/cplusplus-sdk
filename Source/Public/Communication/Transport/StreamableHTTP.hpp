#pragma once

// TODO: Fix External Ref: HTTP server implementation (IncomingMessage, ServerResponse equivalents)
// TODO: Fix External Ref: Transport base class
// TODO: Fix External Ref: JSON-RPC message types and validation
// TODO: Fix External Ref: AuthInfo type

#include "../Core/Common.hpp"

namespace MCP::Transport {

// TODO: Fix External Ref: HTTP request/response types
struct IncomingMessage {
    string method;
    unordered_map<string, string> headers;
    string body;
    optional<AuthInfo> auth;
};

struct ServerResponse {
    function<void(int, const unordered_map<string, string>&)> writeHead;
    function<void(const string&)> end;
    function<bool(const string&)> write;
    function<void()> flushHeaders;
    function<void(function<void()>)> on;
    bool closed = false;
};

const string MAXIMUM_MESSAGE_SIZE = "4mb";

/**
 * Interface for resumability support via event storage
 */
class EventStore {
public:
    virtual ~EventStore() = default;

    /**
     * Stores an event for later retrieval
     * @param StreamID ID of the stream the event belongs to
     * @param message The JSON-RPC message to store
     * @returns The generated event ID for the stored event
     */
    virtual future<EventID> storeEvent(const StreamID& StreamID, const JSON_RPC_Message& message) = 0;

    virtual future<StreamID> replayEventsAfter(const EventID& lastEventID,
        function<future<void>(const EventID&, const JSON_RPC_Message&)> send) = 0;
};

/**
 * Configuration options for StreamableHTTPServerTransport
 */
struct StreamableHTTPServerTransportOptions {
    /**
     * Function that generates a session ID for the transport.
     * The session ID SHOULD be globally unique and cryptographically secure (e.g., a securely generated UUID, a JWT, or a cryptographic hash)
     *
     * Return empty optional to disable session management.
     */
    optional<function<string()>> SessionIDGenerator;

    /**
     * A callback for session initialization events
     * This is called when the server initializes a new session.
     * Useful in cases when you need to register multiple mcp sessions
     * and need to keep track of them.
     * @param SessionID The generated session ID
     */
    optional<function<void(const string&)>> onsessioninitialized;

    /**
     * If true, the server will return JSON responses instead of starting an SSE stream.
     * This can be useful for simple request/response scenarios without streaming.
     * Default is false (SSE streams are preferred).
     */
    bool enableJsonResponse = false;

    /**
     * Event store for resumability support
     * If provided, resumability will be enabled, allowing clients to reconnect and resume messages
     */
    shared_ptr<EventStore> eventStore;
};

/**
 * Server transport for Streamable HTTP: this implements the MCP Streamable HTTP transport specification.
 * It supports both SSE streaming and direct HTTP responses.
 *
 * Usage example:
 *
 * ```cpp
 * // Stateful mode - server sets the session ID
 * StreamableHTTPServerTransportOptions statefulOptions;
 * statefulOptions.SessionIDGenerator = []() { return generateUUID(); };
 * auto statefulTransport = make_unique<StreamableHTTPServerTransport>(statefulOptions);
 *
 * // Stateless mode - explicitly set session ID generator to nullopt
 * StreamableHTTPServerTransportOptions statelessOptions;
 * statelessOptions.SessionIDGenerator = nullopt;
 * auto statelessTransport = make_unique<StreamableHTTPServerTransport>(statelessOptions);
 * ```
 *
 * In stateful mode:
 * - Session ID is generated and included in response headers
 * - Session ID is always included in initialization responses
 * - Requests with invalid session IDs are rejected with 404 Not Found
 * - Non-initialization requests without a session ID are rejected with 400 Bad Request
 * - State is maintained in-memory (connections, message history)
 *
 * In stateless mode:
 * - No Session ID is included in any responses
 * - No session validation is performed
 */
class StreamableHTTPServerTransport : public Transport {
private:
    // when SessionID is not set (nullopt), it means the transport is in stateless mode
    optional<function<string()>> SessionIDGenerator;
    bool _started = false;
    unordered_map<string, shared_ptr<ServerResponse>> _streamMapping;
    unordered_map<RequestID, string> _requestToStreamMapping;
    unordered_map<RequestID, JSON_RPC_Message> _requestResponseMap;
    bool _initialized = false;
    bool _enableJsonResponse = false;
    string _standaloneSseStreamID = "_GET_stream";
    shared_ptr<EventStore> _eventStore;
    optional<function<void(const string&)>> _onsessioninitialized;

public:
    optional<string> SessionID;

    explicit StreamableHTTPServerTransport(const StreamableHTTPServerTransportOptions& options)
        : SessionIDGenerator(options.SessionIDGenerator)
        , _enableJsonResponse(options.enableJsonResponse)
        , _eventStore(options.eventStore)
        , _onsessioninitialized(options.onsessioninitialized) {
    }

    /**
     * Starts the transport. This is required by the Transport interface but is a no-op
     * for the Streamable HTTP transport as connections are managed per-request.
     */
    future<void> start() override {
        if (_started) {
            throw runtime_error("Transport already started");
        }
        _started = true;
        return async(launch::deferred, [](){});
    }

    /**
     * Handles an incoming HTTP request, whether GET or POST
     */
    future<void> handleRequest(const IncomingMessage& req, shared_ptr<ServerResponse> res,
                                  const optional<JSON>& parsedBody = nullopt) {
        if (req.method == "POST") {
            return handlePostRequest(req, res, parsedBody);
        } else if (req.method == "GET") {
            return handleGetRequest(req, res);
        } else if (req.method == "DELETE") {
            return handleDeleteRequest(req, res);
        } else {
            return handleUnsupportedRequest(res);
        }
    }

private:
    /**
     * Handles GET requests for SSE stream
     */
    future<void> handleGetRequest(const IncomingMessage& req, shared_ptr<ServerResponse> res) {
        return async(launch::async, [this, req, res]() {
            // The client MUST include an Accept header, listing text/event-stream as a supported content type.
            auto acceptIt = req.headers.find("accept");
            if (acceptIt == req.headers.end() || acceptIt->second.find("text/event-stream") == string::npos) {
                JSON errorResponse = {
                    {"jsonrpc", "2.0"},
                    {"error", {
                        {"code", -32000},
                        {"message", "Not Acceptable: Client must accept text/event-stream"}
                    }},
                    {"id", nullptr}
                };
                res->writeHead(406, {});
                res->end(errorResponse.dump());
                return;
            }

            // If an Mcp-Session-Id is returned by the server during initialization,
            // clients using the Streamable HTTP transport MUST include it
            // in the Mcp-Session-Id header on all of their subsequent HTTP requests.
            if (!validateSession(req, res)) {
                return;
            }

            // Handle resumability: check for Last-Event-ID header
            if (_eventStore) {
                auto lastEventIDIt = req.headers.find("last-event-id");
                if (lastEventIDIt != req.headers.end()) {
                    replayEvents(lastEventIDIt->second, res).wait();
                    return;
                }
            }

            // The server MUST either return Content-Type: text/event-stream in response to this HTTP GET,
            // or else return HTTP 405 Method Not Allowed
            unordered_map<string, string> headers = {
                {"Content-Type", "text/event-stream"},
                {"Cache-Control", "no-cache, no-transform"},
                {"Connection", "keep-alive"}
            };

            // After initialization, always include the session ID if we have one
            if (SessionID.has_value()) {
                headers["mcp-session-id"] = SessionID.value();
            }

            // Check if there's already an active standalone SSE stream for this session
            if (_streamMapping.find(_standaloneSseStreamID) != _streamMapping.end()) {
                // Only one GET SSE stream is allowed per session
                JSON errorResponse = {
                    {"jsonrpc", "2.0"},
                    {"error", {
                        {"code", -32000},
                        {"message", "Conflict: Only one SSE stream is allowed per session"}
                    }},
                    {"id", nullptr}
                };
                res->writeHead(409, {});
                res->end(errorResponse.dump());
                return;
            }

            // We need to send headers immediately as messages will arrive much later,
            // otherwise the client will just wait for the first message
            res->writeHead(200, headers);
            res->flushHeaders();

            // Assign the response to the standalone SSE stream
            _streamMapping[_standaloneSseStreamID] = res;
            // Set up close handler for client disconnects
            res->on([this](){ _streamMapping.erase(_standaloneSseStreamID); });
        });
    }

    /**
     * Replays events that would have been sent after the specified event ID
     * Only used when resumability is enabled
     */
    future<void> replayEvents(const string& lastEventID, shared_ptr<ServerResponse> res) {
        return async(launch::async, [this, lastEventID, res]() {
            if (!_eventStore) {
                return;
            }
            try {
                unordered_map<string, string> headers = {
                    {"Content-Type", "text/event-stream"},
                    {"Cache-Control", "no-cache, no-transform"},
                    {"Connection", "keep-alive"}
                };

                if (SessionID.has_value()) {
                    headers["mcp-session-id"] = SessionID.value();
                }
                res->writeHead(200, headers);
                res->flushHeaders();

                auto StreamIDFuture = _eventStore->replayEventsAfter(lastEventID,
                    [this, res](const EventID& EventID, const JSON_RPC_Message& message) -> future<void> {
                        return async(launch::async, [this, res, EventID, message]() {
                            if (!writeSSEEvent(res, message, EventID)) {
                                if (onerror) {
                                    onerror(runtime_error("Failed replay events"));
                                }
                                res->end("");
                            }
                        });
                    });

                auto StreamID = StreamIDFuture.get();
                _streamMapping[StreamID] = res;
            } catch (const exception& error) {
                if (onerror) {
                    onerror(error);
                }
            }
        });
    }

    /**
     * Writes an event to the SSE stream with proper formatting
     */
    bool writeSSEEvent(shared_ptr<ServerResponse> res, const JSON_RPC_Message& message,
                      const optional<string>& EventID = nullopt) {
        string eventData = "event: message\n";
        // Include event ID if provided - this is important for resumability
        if (EventID.has_value()) {
            eventData += "id: " + EventID.value() + "\n";
        }
        eventData += "data: " + message.data.dump() + "\n\n";

        return res->write(eventData);
    }

    /**
     * Handles unsupported requests (PUT, PATCH, etc.)
     */
    future<void> handleUnsupportedRequest(shared_ptr<ServerResponse> res) {
        return async(launch::async, [res]() {
            JSON errorResponse = {
                {"jsonrpc", "2.0"},
                {"error", {
                    {"code", -32000},
                    {"message", "Method not allowed."}
                }},
                {"id", nullptr}
            };
            unordered_map<string, string> headers = {{"Allow", "GET, POST, DELETE"}};
            res->writeHead(405, headers);
            res->end(errorResponse.dump());
        });
    }

    /**
     * Handles POST requests containing JSON-RPC messages
     */
    future<void> handlePostRequest(const IncomingMessage& req, shared_ptr<ServerResponse> res,
                                      const optional<JSON>& parsedBody = nullopt) {
        return async(launch::async, [this, req, res, parsedBody]() {
            try {
                // Validate the Accept header
                auto acceptIt = req.headers.find("accept");
                // The client MUST include an Accept header, listing both application/json and text/event-stream as supported content types.
                if (acceptIt == req.headers.end() ||
                    acceptIt->second.find("application/json") == string::npos ||
                    acceptIt->second.find("text/event-stream") == string::npos) {
                    JSON errorResponse = {
                        {"jsonrpc", "2.0"},
                        {"error", {
                            {"code", -32000},
                            {"message", "Not Acceptable: Client must accept both application/json and text/event-stream"}
                        }},
                        {"id", nullptr}
                    };
                    res->writeHead(406, {});
                    res->end(errorResponse.dump());
                    return;
                }

                auto ctIt = req.headers.find("content-type");
                if (ctIt == req.headers.end() || ctIt->second.find("application/json") == string::npos) {
                    JSON errorResponse = {
                        {"jsonrpc", "2.0"},
                        {"error", {
                            {"code", -32000},
                            {"message", "Unsupported Media Type: Content-Type must be application/json"}
                        }},
                        {"id", nullptr}
                    };
                    res->writeHead(415, {});
                    res->end(errorResponse.dump());
                    return;
                }

                optional<AuthInfo> authInfo = req.auth;

                JSON rawMessage;
                if (parsedBody.has_value()) {
                    rawMessage = parsedBody.value();
                } else {
                    try {
                        rawMessage = JSON::parse(req.body);
                    } catch (const JSON::parse_error& e) {
                        throw runtime_error("JSON parse error: " + string(e.what()));
                    }
                }

                vector<JSON_RPC_Message> messages;

                // handle batch and single messages
                if (rawMessage.is_array()) {
                    for (const auto& msg : rawMessage) {
                        // TODO: Fix External Ref: JSON_RPC_MessageSchema validation
                        messages.emplace_back(msg);
                    }
                } else {
                    // TODO: Fix External Ref: JSON_RPC_MessageSchema validation
                    messages.emplace_back(rawMessage);
                }

                // Check if this is an initialization request
                // TODO: Fix External Ref: isInitializeRequest function
                bool isInitializationRequest = false;
                for (const auto& msg : messages) {
                    // Placeholder check - would need actual implementation
                    if (msg.data.contains("method") && msg.data["method"] == "initialize") {
                        isInitializationRequest = true;
                        break;
                    }
                }

                if (isInitializationRequest) {
                    // If it's a server with session management and the session ID is already set we should reject the request
                    // to avoid re-initialization.
                    if (_initialized && SessionID.has_value()) {
                        JSON errorResponse = {
                            {"jsonrpc", "2.0"},
                            {"error", {
                                {"code", -32600},
                                {"message", "Invalid Request: Server already initialized"}
                            }},
                            {"id", nullptr}
                        };
                        res->writeHead(400, {});
                        res->end(errorResponse.dump());
                        return;
                    }
                    if (messages.size() > 1) {
                        JSON errorResponse = {
                            {"jsonrpc", "2.0"},
                            {"error", {
                                {"code", -32600},
                                {"message", "Invalid Request: Only one initialization request is allowed"}
                            }},
                            {"id", nullptr}
                        };
                        res->writeHead(400, {});
                        res->end(errorResponse.dump());
                        return;
                    }
                    if (SessionIDGenerator.has_value()) {
                        SessionID = SessionIDGenerator.value()();
                    }
                    _initialized = true;

                    // If we have a session ID and an onsessioninitialized handler, call it immediately
                    // This is needed in cases where the server needs to keep track of multiple sessions
                    if (SessionID.has_value() && _onsessioninitialized.has_value()) {
                        _onsessioninitialized.value()(SessionID.value());
                    }
                }

                // If an Mcp-Session-Id is returned by the server during initialization,
                // clients using the Streamable HTTP transport MUST include it
                // in the Mcp-Session-Id header on all of their subsequent HTTP requests.
                if (!isInitializationRequest && !validateSession(req, res)) {
                    return;
                }

                // check if it contains requests
                // TODO: Fix External Ref: isJSON_RPC_Request function
                bool hasRequests = false;
                for (const auto& msg : messages) {
                    // Placeholder check - would need actual implementation
                    if (msg.data.contains("method") && msg.data.contains("id")) {
                        hasRequests = true;
                        break;
                    }
                }

                if (!hasRequests) {
                    // if it only contains notifications or responses, return 202
                    res->writeHead(202, {});
                    res->end("");

                    // handle each message
                    for (const auto& message : messages) {
                        if (onmessage) {
                            onmessage(message, authInfo);
                        }
                    }
                } else if (hasRequests) {
                    // The default behavior is to use SSE streaming
                    // but in some cases server will return JSON responses
                    string StreamID = generateUUID();
                    if (!_enableJsonResponse) {
                        unordered_map<string, string> headers = {
                            {"Content-Type", "text/event-stream"},
                            {"Cache-Control", "no-cache"},
                            {"Connection", "keep-alive"}
                        };

                        // After initialization, always include the session ID if we have one
                        if (SessionID.has_value()) {
                            headers["mcp-session-id"] = SessionID.value();
                        }

                        res->writeHead(200, headers);
                    }
                    // Store the response for this request to send messages back through this connection
                    // We need to track by request ID to maintain the connection
                    for (const auto& message : messages) {
                        // TODO: Fix External Ref: isJSON_RPC_Request function
                        if (message.data.contains("method") && message.data.contains("id")) {
                            _streamMapping[StreamID] = res;
                            _requestToStreamMapping[message.data["id"]] = StreamID;
                        }
                    }
                    // Set up close handler for client disconnects
                    res->on([this, StreamID](){ _streamMapping.erase(StreamID); });

                    // handle each message
                    for (const auto& message : messages) {
                        if (onmessage) {
                            onmessage(message, authInfo);
                        }
                    }
                    // The server SHOULD NOT close the SSE stream before sending all JSON-RPC responses
                    // This will be handled by the send() method when responses are ready
                }
            } catch (const exception& error) {
                // return JSON-RPC formatted error
                JSON errorResponse = {
                    {"jsonrpc", "2.0"},
                    {"error", {
                        {"code", -32700},
                        {"message", "Parse error"},
                        {"data", error.what()}
                    }},
                    {"id", nullptr}
                };
                res->writeHead(400, {});
                res->end(errorResponse.dump());
                if (onerror) {
                    onerror(error);
                }
            }
        });
    }

    /**
     * Handles DELETE requests to terminate sessions
     */
    future<void> handleDeleteRequest(const IncomingMessage& req, shared_ptr<ServerResponse> res) {
        return async(launch::async, [this, req, res]() {
            if (!validateSession(req, res)) {
                return;
            }
            close().wait();
            res->writeHead(200, {});
            res->end("");
        });
    }

    /**
     * Validates session ID for non-initialization requests
     * Returns true if the session is valid, false otherwise
     */
    bool validateSession(const IncomingMessage& req, shared_ptr<ServerResponse> res) {
        if (!SessionIDGenerator.has_value()) {
            // If the SessionIDGenerator ID is not set, the session management is disabled
            // and we don't need to validate the session ID
            return true;
        }
        if (!_initialized) {
            // If the server has not been initialized yet, reject all requests
            JSON errorResponse = {
                {"jsonrpc", "2.0"},
                {"error", {
                    {"code", -32000},
                    {"message", "Bad Request: Server not initialized"}
                }},
                {"id", nullptr}
            };
            res->writeHead(400, {});
            res->end(errorResponse.dump());
            return false;
        }

        auto SessionIDIt = req.headers.find("mcp-session-id");

        if (SessionIDIt == req.headers.end()) {
            // Non-initialization requests without a session ID should return 400 Bad Request
            JSON errorResponse = {
                {"jsonrpc", "2.0"},
                {"error", {
                    {"code", -32000},
                    {"message", "Bad Request: Mcp-Session-Id header is required"}
                }},
                {"id", nullptr}
            };
            res->writeHead(400, {});
            res->end(errorResponse.dump());
            return false;
        } else if (SessionIDIt->second != SessionID.value_or("")) {
            // Reject requests with invalid session ID with 404 Not Found
            JSON errorResponse = {
                {"jsonrpc", "2.0"},
                {"error", {
                    {"code", -32001},
                    {"message", "Session not found"}
                }},
                {"id", nullptr}
            };
            res->writeHead(404, {});
            res->end(errorResponse.dump());
            return false;
        }

        return true;
    }

public:
    future<void> close() override {
        return async(launch::async, [this]() {
            // Close all SSE connections
            for (auto& [id, response] : _streamMapping) {
                response->end("");
            }
            _streamMapping.clear();

            // Clear any pending responses
            _requestResponseMap.clear();
            if (onclose) {
                onclose();
            }
        });
    }

    future<void> send(const JSON_RPC_Message& message, const optional<RequestID>& relatedRequestID = nullopt) override {
        return async(launch::async, [this, message, relatedRequestID]() {
            optional<RequestID> RequestID = relatedRequestID;

            // TODO: Fix External Ref: isJSON_RPC_Response and isJSON_RPC_Error functions
            bool isResponse = message.data.contains("result") || message.data.contains("error");
            if (isResponse) {
                // If the message is a response, use the request ID from the message
                if (message.data.contains("id")) {
                    RequestID = message.data["id"];
                }
            }

            // Check if this message should be sent on the standalone SSE stream (no request ID)
            // Ignore notifications from tools (which have relatedRequestID set)
            // Those will be sent via dedicated response SSE streams
            if (!RequestID.has_value()) {
                // For standalone SSE streams, we can only send requests and notifications
                if (isResponse) {
                    throw runtime_error("Cannot send a response on a standalone SSE stream unless resuming a previous client request");
                }
                auto standaloneSseIt = _streamMapping.find(_standaloneSseStreamID);
                if (standaloneSseIt == _streamMapping.end()) {
                    // The spec says the server MAY send messages on the stream, so it's ok to discard if no stream
                    return;
                }

                // Generate and store event ID if event store is provided
                optional<string> EventID;
                if (_eventStore) {
                    // Stores the event and gets the generated event ID
                    auto EventIDFuture = _eventStore->storeEvent(_standaloneSseStreamID, message);
                    EventID = EventIDFuture.get();
                }

                // Send the message to the standalone SSE stream
                writeSSEEvent(standaloneSseIt->second, message, EventID);
                return;
            }

            // Get the response for this request
            auto StreamIDIt = _requestToStreamMapping.find(RequestID.value());
            if (StreamIDIt == _requestToStreamMapping.end()) {
                throw runtime_error("No connection established for request ID: " + RequestID.value());
            }

            auto responseIt = _streamMapping.find(StreamIDIt->second);
            if (responseIt == _streamMapping.end()) {
                throw runtime_error("No connection established for request ID: " + RequestID.value());
            }

            auto response = responseIt->second;

            if (!_enableJsonResponse) {
                // For SSE responses, generate event ID if event store is provided
                optional<string> EventID;

                if (_eventStore) {
                    auto EventIDFuture = _eventStore->storeEvent(StreamIDIt->second, message);
                    EventID = EventIDFuture.get();
                }
                if (response) {
                    // Write the event to the response stream
                    writeSSEEvent(response, message, EventID);
                }
            }

            if (isResponse) {
                _requestResponseMap[RequestID.value()] = message;

                vector<RequestID> relatedIds;
                for (const auto& [id, StreamID] : _requestToStreamMapping) {
                    if (_streamMapping[StreamID] == response) {
                        relatedIds.push_back(id);
                    }
                }

                // Check if we have responses for all requests using this connection
                bool allResponsesReady = true;
                for (const auto& id : relatedIds) {
                    if (_requestResponseMap.find(id) == _requestResponseMap.end()) {
                        allResponsesReady = false;
                        break;
                    }
                }

                if (allResponsesReady) {
                    if (!response) {
                        throw runtime_error("No connection established for request ID: " + RequestID.value());
                    }
                    if (_enableJsonResponse) {
                        // All responses ready, send as JSON
                        unordered_map<string, string> headers = {
                            {"Content-Type", "application/json"}
                        };
                        if (SessionID.has_value()) {
                            headers["mcp-session-id"] = SessionID.value();
                        }

                        vector<JSON> responses;
                        for (const auto& id : relatedIds) {
                            responses.push_back(_requestResponseMap[id].data);
                        }

                        response->writeHead(200, headers);
                        if (responses.size() == 1) {
                            response->end(responses[0].dump());
                        } else {
                            JSON responseArray = responses;
                            response->end(responseArray.dump());
                        }
                    } else {
                        // End the SSE stream
                        response->end("");
                    }
                    // Clean up
                    for (const auto& id : relatedIds) {
                        _requestResponseMap.erase(id);
                        _requestToStreamMapping.erase(id);
                    }
                }
            }
        });
    }

private:
    /**
     * Generates a UUID-like string for request/stream IDs
     */
    string generateUUID() {
        static random_device rd;
        static mt19937 gen(rd());
        static uniform_int_distribution<> dis(0, 15);
        static uniform_int_distribution<> dis2(8, 11);

        stringstream ss;
        ss << hex;
        for (int i = 0; i < 8; i++) {
            ss << dis(gen);
        }
        ss << "-";
        for (int i = 0; i < 4; i++) {
            ss << dis(gen);
        }
        ss << "-4";
        for (int i = 0; i < 3; i++) {
            ss << dis(gen);
        }
        ss << "-";
        ss << dis2(gen);
        for (int i = 0; i < 3; i++) {
            ss << dis(gen);
        }
        ss << "-";
        for (int i = 0; i < 12; i++) {
            ss << dis(gen);
        }
        return ss.str();
    }
};


// ##########################################################
// ##########################################################
// ===================== CLIENT SECTION =====================
// ##########################################################
// ##########################################################

// Default reconnection options for StreamableHTTP connections
struct StreamableHTTPReconnectionOptions {
    int maxReconnectionDelay = 30000;      // 30 seconds
    int initialReconnectionDelay = 1000;   // 1 second
    double reconnectionDelayGrowFactor = 1.5;
    int maxRetries = 2;
};

const StreamableHTTPReconnectionOptions DEFAULT_STREAMABLE_HTTP_RECONNECTION_OPTIONS = {
    1000,   // initialReconnectionDelay
    30000,  // maxReconnectionDelay
    1.5,    // reconnectionDelayGrowFactor
    2       // maxRetries
};

class StreamableHTTPError : public exception {
private:
    optional<int> code_;
    string message_;
    string full_message_;

public:
    StreamableHTTPError(optional<int> code, const string& message)
        : code_(code), message_(message) {
        full_message_ = "Streamable HTTP error: " + message;
    }

    const char* what() const noexcept override {
        return full_message_.c_str();
    }

    optional<int> getCode() const { return code_; }
    const string& getMessage() const { return message_; }
};

/**
 * Options for starting or authenticating an SSE connection
 */
struct StartSSEOptions {
    optional<string> resumptionToken;
    function<void(const string&)> onresumptiontoken;
    optional<string> replayMessageId; // Can be string or number
};

/**
 * Configuration options for the `StreamableHTTPClientTransport`.
 */
struct StreamableHTTPClientTransportOptions {
    shared_ptr<OAuthClientProvider> authProvider;
    map<string, string> requestHeaders; // RequestInit equivalent
    StreamableHTTPReconnectionOptions reconnectionOptions = DEFAULT_STREAMABLE_HTTP_RECONNECTION_OPTIONS;
    optional<string> SessionID;
};

/**
 * Client transport for Streamable HTTP: this implements the MCP Streamable HTTP transport specification.
 * It will connect to a server using HTTP POST for sending messages and HTTP GET with Server-Sent Events
 * for receiving messages.
 */
class StreamableHTTPClientTransport {
private:
    atomic<bool> abort_requested_;
    string url_;
    optional<string> resource_metadata_url_;
    map<string, string> request_headers_;
    shared_ptr<OAuthClientProvider> auth_provider_;
    optional<string> session_id_;
    StreamableHTTPReconnectionOptions reconnection_options_;

public:
    function<void()> onclose;
    function<void(const exception&)> onerror;
    function<void(const JSON_RPC_Message&)> onmessage;

    StreamableHTTPClientTransport(const string& url,
                                 const StreamableHTTPClientTransportOptions& opts = {})
        : abort_requested_(false), url_(url), request_headers_(opts.requestHeaders),
          auth_provider_(opts.authProvider), session_id_(opts.SessionID),
          reconnection_options_(opts.reconnectionOptions) {
    }

private:
    future<void> authThenStart() {
        return async(launch::async, [this]() {
            if (!auth_provider_) {
                throw UnauthorizedError("No auth provider");
            }

            try {
                // TODO: Fix External Ref: auth function
                // AuthResult result = auth(auth_provider_, {url_, resource_metadata_url_});
                // if (result != "AUTHORIZED") {
                //     throw UnauthorizedError();
                // }
            } catch (const exception& error) {
                if (onerror) onerror(error);
                throw;
            }

            return startOrAuthSse({nullopt, nullptr, nullopt});
        });
    }

    future<map<string, string>> commonHeaders() {
        return async(launch::async, [this]() {
            map<string, string> headers;

            if (auth_provider_) {
                // TODO: Fix External Ref: tokens() method
                // auto tokens = auth_provider_->tokens();
                // if (tokens) {
                //     headers["Authorization"] = "Bearer " + tokens->access_token;
                // }
            }

            if (session_id_) {
                headers["mcp-session-id"] = *session_id_;
            }

            // Merge with request headers
            for (const auto& [key, value] : request_headers_) {
                headers[key] = value;
            }

            return headers;
        });
    }

    future<void> startOrAuthSse(const StartSSEOptions& options) {
        return async(launch::async, [this, options]() {
            try {
                auto headers = commonHeaders().get();
                headers["Accept"] = "text/event-stream";

                if (options.resumptionToken) {
                    headers["last-event-id"] = *options.resumptionToken;
                }

                // TODO: Fix External Ref: HTTP GET implementation
                // auto response = httpGet(url_, headers, abort_requested_);

                // if (!response.ok) {
                //     if (response.status == 401 && auth_provider_) {
                //         return authThenStart().get();
                //     }
                //     if (response.status == 405) {
                //         return; // Server doesn't support SSE
                //     }
                //     throw StreamableHTTPError(response.status,
                //         "Failed to open SSE stream: " + response.statusText);
                // }

                // handleSseStream(response.body, options);
            } catch (const exception& error) {
                if (onerror) onerror(error);
                throw;
            }
        });
    }

    int getNextReconnectionDelay(int attempt) {
        int initialDelay = reconnection_options_.initialReconnectionDelay;
        double growFactor = reconnection_options_.reconnectionDelayGrowFactor;
        int maxDelay = reconnection_options_.maxReconnectionDelay;

        return min(static_cast<int>(initialDelay * pow(growFactor, attempt)), maxDelay);
    }

    void scheduleReconnection(const StartSSEOptions& options, int attemptCount = 0) {
        int maxRetries = reconnection_options_.maxRetries;

        if (maxRetries > 0 && attemptCount >= maxRetries) {
            if (onerror) {
                string msg = "Maximum reconnection attempts (" + to_string(maxRetries) + ") exceeded.";
                onerror(runtime_error(msg));
            }
            return;
        }

        int delay = getNextReconnectionDelay(attemptCount);

        thread([this, options, attemptCount, delay]() {
            this_thread::sleep_for(chrono::milliseconds(delay));

            try {
                startOrAuthSse(options).get();
            } catch (const exception& error) {
                if (onerror) {
                    string msg = "Failed to reconnect SSE stream: " + string(error.what());
                    onerror(runtime_error(msg));
                }
                scheduleReconnection(options, attemptCount + 1);
            }
        }).detach();
    }

    void handleSseStream(const string& streamData, const StartSSEOptions& options) {
        // TODO: Fix External Ref: SSE stream parsing
        optional<string> lastEventID;

        try {
            // TODO: Implement SSE parsing equivalent to EventSourceParserStream
            // This would need to parse Server-Sent Events format
            // and extract id, event, and data fields

            // Simplified parsing placeholder:
            // auto events = parseSSEStream(streamData);
            // for (const auto& event : events) {
            //     if (!event.id.empty()) {
            //         lastEventID = event.id;
            //         if (options.onresumptiontoken) {
            //             options.onresumptiontoken(event.id);
            //         }
            //     }
            //
            //     if (event.event.empty() || event.event == "message") {
            //         try {
            //             // TODO: Fix External Ref: JSON_RPC_MessageSchema validation
            //             auto message = JSON::parse(event.data);
            //             if (options.replayMessageId && isJSON_RPC_Response(message)) {
            //                 message["id"] = *options.replayMessageId;
            //             }
            //             if (onmessage) onmessage(message);
            //         } catch (const exception& error) {
            //             if (onerror) onerror(error);
            //         }
            //     }
            // }
        } catch (const exception& error) {
            if (onerror) {
                string msg = "SSE stream disconnected: " + string(error.what());
                onerror(runtime_error(msg));
            }

            if (!abort_requested_ && lastEventID) {
                try {
                    StartSSEOptions reconnectOptions = {
                        *lastEventID,
                        options.onresumptiontoken,
                        options.replayMessageId
                    };
                    scheduleReconnection(reconnectOptions, 0);
                } catch (const exception& reconnectError) {
                    if (onerror) {
                        string msg = "Failed to reconnect: " + string(reconnectError.what());
                        onerror(runtime_error(msg));
                    }
                }
            }
        }
    }

public:
    future<void> start() {
        return async(launch::async, [this]() {
            if (abort_requested_) {
                throw runtime_error(
                    "StreamableHTTPClientTransport already started! If using Client class, note that connect() calls start() automatically."
                );
            }
            abort_requested_ = false;
        });
    }

    /**
     * Call this method after the user has finished authorizing via their user agent and is redirected back to the MCP client application.
     */
    future<void> finishAuth(const string& authorizationCode) {
        return async(launch::async, [this, authorizationCode]() {
            if (!auth_provider_) {
                throw UnauthorizedError("No auth provider");
            }

            // TODO: Fix External Ref: auth function with authorizationCode
            // auto result = auth(auth_provider_, {url_, authorizationCode, resource_metadata_url_});
            // if (result != "AUTHORIZED") {
            //     throw UnauthorizedError("Failed to authorize");
            // }
        });
    }

    future<void> close() {
        return async(launch::async, [this]() {
            abort_requested_ = true;
            if (onclose) onclose();
        });
    }

    struct SendOptions {
        optional<string> resumptionToken;
        function<void(const string&)> onresumptiontoken;
    };

    future<void> send(const JSON_RPC_Message& message, const SendOptions& options = {}) {
        return async(launch::async, [this, message, options]() {
            try {
                if (options.resumptionToken) {
                    StartSSEOptions sseOptions = {
                        options.resumptionToken,
                        options.onresumptiontoken,
                        nullopt // TODO: Extract message ID if it's a request
                    };

                    async(launch::async, [this, sseOptions]() {
                        try {
                            startOrAuthSse(sseOptions).get();
                        } catch (const exception& err) {
                            if (onerror) onerror(err);
                        }
                    });
                    return;
                }

                auto headers = commonHeaders().get();
                headers["content-type"] = "application/json";
                headers["accept"] = "application/json, text/event-stream";

                // TODO: Fix External Ref: HTTP POST implementation
                // string body = JSON::stringify(message);
                // auto response = httpPost(url_, headers, body, abort_requested_);

                // Handle session ID received during initialization
                // if (response.headers.count("mcp-session-id")) {
                //     session_id_ = response.headers["mcp-session-id"];
                // }

                // if (!response.ok) {
                //     if (response.status == 401 && auth_provider_) {
                //         resource_metadata_url_ = extractResourceMetadataUrl(response);
                //         auto result = auth(auth_provider_, {url_, resource_metadata_url_});
                //         if (result != "AUTHORIZED") {
                //             throw UnauthorizedError();
                //         }
                //         return send(message, options).get();
                //     }
                //     throw runtime_error("Error POSTing to endpoint (HTTP " +
                //         to_string(response.status) + "): " + response.text);
                // }

                // Handle different response types based on status and content
                // if (response.status == 202) {
                //     if (isInitializedNotification(message)) {
                //         async(launch::async, [this]() {
                //             try {
                //                 startOrAuthSse({nullopt, nullptr, nullopt}).get();
                //             } catch (const exception& err) {
                //                 if (onerror) onerror(err);
                //             }
                //         });
                //     }
                //     return;
                // }

                // TODO: Handle streaming vs JSON responses based on content-type

            } catch (const exception& error) {
                if (onerror) onerror(error);
                throw;
            }
        });
    }

    future<void> send(const vector<JSON_RPC_Message>& messages, const SendOptions& options = {}) {
        return async(launch::async, [this, messages, options]() {
            // TODO: Implement batch message sending
            // Similar to single message but handle array serialization
        });
    }

    optional<string> getSessionID() const {
        return session_id_;
    }

    /**
     * Terminates the current session by sending a DELETE request to the server.
     */
    future<void> terminateSession() {
        return async(launch::async, [this]() {
            if (!session_id_) {
                return; // No session to terminate
            }

            try {
                auto headers = commonHeaders().get();

                // TODO: Fix External Ref: HTTP DELETE implementation
                // auto response = httpDelete(url_, headers, abort_requested_);

                // Handle 405 as valid response per spec
                // if (!response.ok && response.status != 405) {
                //     throw StreamableHTTPError(response.status,
                //         "Failed to terminate session: " + response.statusText);
                // }

                session_id_ = nullopt;
            } catch (const exception& error) {
                if (onerror) onerror(error);
                throw;
            }
        });
    }
};

MCP_NAMESPACE_END