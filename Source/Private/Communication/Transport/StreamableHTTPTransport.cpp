#include "Communication/Transport/StreamableHTTPTransport.h"

#include "Communication/Utilities/TransportUtilities.h"
#include "Core.h"
#include "Core/Constants/TransportConstants.h"

MCP_NAMESPACE_BEGIN

future<void> StreamableHTTPServerTransport::Start() {
    if (_started) { throw runtime_error("Transport already started"); }
    _started = true;
    return async(launch::deferred, []() {});
}

future<void>
StreamableHTTPServerTransport::HandleRequest(const HTTP_Request& req, shared_ptr<HTTP_Response> res,
                                             const optional<JSON>& parsedBody = nullopt) {
    if (req.method == MTHD_POST) {
        return handlePostRequest(req, res, parsedBody);
    } else if (req.method == MTHD_GET) {
        return handleGetRequest(req, res);
    } else if (req.method == MTHD_DELETE) {
        return handleDeleteRequest(req, res);
    } else {
        return handleUnsupportedRequest(res);
    }
}

future<void> StreamableHTTPServerTransport::HandleGetRequest(const RequestBase& req,
                                                             shared_ptr<ResponseBase> res) {
    return async(launch::async, [this, req, res]() {
        // The client MUST include an Accept header, listing text/event-stream as a supported
        // content type.
        auto acceptIt = req.headers.find(TSPT_ACCEPT);
        if (acceptIt == req.headers.end()
            || acceptIt->second.find(TSPT_TEXT_EVENT_STREAM) == string::npos) {
            JSON errorResponse = {
                {MSG_JSON_RPC, MSG_JSON_RPC_VERSION},
                {MSG_ERROR,
                 {{MSG_CODE, Errors::CONNECTION_CLOSED},
                  {MSG_MESSAGE, "Not Acceptable: Client must accept text/event-stream"}}},
                {MSG_ID, nullptr}};
            res->writeHead(406, {});
            res->end(errorResponse.dump());
            return;
        }

        // If an Mcp-Session-Id is returned by the server during initialization,
        // clients using the Streamable HTTP transport MUST include it
        // in the Mcp-Session-Id header on all of their subsequent HTTP requests.
        if (!validateSession(req, res)) { return; }

        // Handle resumability: check for Last-Event-ID header
        if (_eventStore) {
            auto lastEventIDIt = req.headers.find("last-event-id");
            if (lastEventIDIt != req.headers.end()) {
                replayEvents(lastEventIDIt->second, res).wait();
                return;
            }
        }

        // The server MUST either return Content-Type: text/event-stream in response to this
        // HTTP GET, or else return HTTP HTTPStatus::MethodNotAllowed Method Not Allowed
        unordered_map<string, string> headers = {{TSPT_CONTENT_TYPE, TSPT_TEXT_EVENT_STREAM},
                                                 {"Cache-Control", "no-cache, no-transform"},
                                                 {"Connection", "keep-alive"}};

        // After initialization, always include the session ID if we have one
        if (SessionID()) { headers["mcp-session-id"] = SessionID.value(); }

        // Check if there's already an active standalone SSE stream for this session
        if (_streamMapping.find(_standaloneSseStreamID) != _streamMapping.end()) {
            // Only one GET SSE stream is allowed per session
            JSON errorResponse = {
                {MSG_JSON_RPC, MSG_JSON_RPC_VERSION},
                {MSG_ERROR,
                 {{MSG_CODE, Errors::ConnectionClosed},
                  {MSG_MESSAGE, "Conflict: Only one SSE stream is allowed per session"}}},
                {MSG_ID, nullptr}};
            res->writeHead(409, {});
            res->end(errorResponse.dump());
            return;
        }

        // We need to send headers immediately as messages will arrive much later,
        // otherwise the client will just wait for the first message
        res->writeHead(HTTPStatus::Ok, headers);
        res->flushHeaders();

        // Assign the response to the standalone SSE stream
        _streamMapping[_standaloneSseStreamID] = res;
        // Set up close handler for client disconnects
        res->on([this]() { _streamMapping.erase(_standaloneSseStreamID); });
    });
}

future<void> StreamableHTTPServerTransport::ReplayEvents(const string& lastEventID,
                                                         shared_ptr<HTTP_Response> res) {
    return async(launch::async, [this, lastEventID, res]() {
        if (!_eventStore) { return; }
        try {
            unordered_map<string, string> headers = {{TSPT_CONTENT_TYPE, TSPT_TEXT_EVENT_STREAM},
                                                     {"Cache-Control", "no-cache, no-transform"},
                                                     {"Connection", "keep-alive"}};

            if (SessionID()) { headers["mcp-session-id"] = SessionID.value(); }
            res->writeHead(HTTPStatus::Ok, headers);
            res->flushHeaders();

            auto StreamIDFuture = _eventStore->replayEventsAfter(
                lastEventID,
                [this, res](const EventID& EventID, const MessageBase& message) -> future<void> {
                    return async(launch::async, [this, res, EventID, message]() {
                        if (!writeSSEEvent(res, message, EventID)) {
                            if (onerror) { onerror(runtime_error("Failed replay events")); }
                            res->end(MSG_EMPTY);
                        }
                    });
                });

            auto StreamID = StreamIDFuture.get();
            _streamMapping[StreamID] = res;
        } catch (const exception& error) {
            if (onerror) { onerror(error); }
        }
    });
}

bool StreamableHTTPServerTransport::WriteSSEEvent(shared_ptr<HTTP_Response> res,
                                                  const MessageBase& message,
                                                  const optional<string>& EventID = nullopt) {
    string eventData = "event: message\n";
    // Include event ID if provided - this is important for resumability
    if (EventID()) { eventData += "id: " + EventID.value() + "\n"; }
    eventData += "data: " + message.data.dump() + "\n\n";

    return res->write(eventData);
}

future<void>
StreamableHTTPServerTransport::HandleUnsupportedRequest(shared_ptr<HTTP_Response> res) {
    return async(launch::async, [res]() {
        JSON errorResponse = {
            {MSG_JSON_RPC, MSG_JSON_RPC_VERSION},
            {MSG_ERROR,
             {{MSG_CODE, Errors::ConnectionClosed}, {MSG_MESSAGE, "Method not allowed."}}},
            {MSG_ID, nullptr}};
        unordered_map<string, string> headers = {{"Allow", "GET, POST, DELETE"}};
        res->writeHead(HTTPStatus::MethodNotAllowed, headers);
        res->end(errorResponse.dump());
    });
}

future<void>
StreamableHTTPServerTransport::HandlePostRequest(const HTTP_Request& req,
                                                 shared_ptr<HTTP_Response> res,
                                                 const optional<JSON>& parsedBody = nullopt) {
    return async(launch::async, [this, req, res, parsedBody]() {
        try {
            // Validate the Accept header
            auto acceptIt = req.headers.find(TSPT_ACCEPT);
            // The client MUST include an Accept header, listing both application/json and
            // text/event-stream as supported content types.
            if (acceptIt == req.headers.end()
                || acceptIt->second.find(TSPT_APP_JSON) == string::npos
                || acceptIt->second.find(TSPT_TEXT_EVENT_STREAM) == string::npos) {
                JSON errorResponse = {{MSG_JSON_RPC, MSG_JSON_RPC_VERSION},
                                      {MSG_ERROR,
                                       {{MSG_CODE, Errors::ConnectionClosed},
                                        {MSG_MESSAGE, "Not Acceptable: Client must accept both "
                                                      "application/json and text/event-stream"}}},
                                      {MSG_ID, nullptr}};
                res->writeHead(406, {});
                res->end(errorResponse.dump());
                return;
            }

            auto ctIt = req.headers.find(TSPT_CONTENT_TYPE);
            if (ctIt == req.headers.end() || ctIt->second.find(TSPT_APP_JSON) == string::npos) {
                JSON errorResponse = {
                    {MSG_JSON_RPC, MSG_JSON_RPC_VERSION},
                    {MSG_ERROR,
                     {{MSG_CODE, Errors::ConnectionClosed},
                      {MSG_MESSAGE,
                       "Unsupported Media Type: Content-Type must be application/json"}}},
                    {MSG_ID, nullptr}};
                res->writeHead(415, {});
                res->end(errorResponse.dump());
                return;
            }

            optional<AuthInfo> authInfo = req.auth;

            JSON rawMessage;
            if (parsedBody()) {
                rawMessage = parsedBody.value();
            } else {
                try {
                    rawMessage = JSON::parse(req.body);
                } catch (const JSON::parse_error& e) {
                    throw runtime_error("JSON parse error: " + string(e.what()));
                }
            }

            vector<MessageBase> messages;

            // handle batch and single messages
            if (rawMessage.is_array()) {
                for (const auto& msg : rawMessage) {
                    // TODO: Fix External Ref: MessageBaseSchema validation
                    messages.emplace_back(msg);
                }
            } else {
                // TODO: Fix External Ref: MessageBaseSchema validation
                messages.emplace_back(rawMessage);
            }

            // Check if this is an initialization request
            // TODO: Fix External Ref: isInitializeRequest function
            bool isInitializationRequest = false;
            for (const auto& msg : messages) {
                // Placeholder check - would need actual implementation
                if (msg.data.contains(MSG_METHOD) && msg.data[MSG_METHOD] == MTHD_INITIALIZE) {
                    isInitializationRequest = true;
                    break;
                }
            }

            if (isInitializationRequest) {
                // If it's a server with session management and the session ID is already set we
                // should reject the request to avoid re-initialization.
                if (_initialized && SessionID()) {
                    JSON errorResponse = {
                        {MSG_JSON_RPC, MSG_JSON_RPC_VERSION},
                        {MSG_ERROR,
                         {{MSG_CODE, Errors::InvalidRequest},
                          {MSG_MESSAGE, "Invalid Request: Server already initialized"}}},
                        {MSG_ID, nullptr}};
                    res->writeHead(HTTPStatus::BadRequestPStatus::BadRequest, {});
                    res->end(errorResponse.dump());
                    return;
                }
                if (messages.size() > 1) {
                    JSON errorResponse = {
                        {MSG_JSON_RPC, MSG_JSON_RPC_VERSION},
                        {MSG_ERROR,
                         {{MSG_CODE, Errors::InvalidRequest},
                          {MSG_MESSAGE,
                           "Invalid Request: Only one initialization request is allowed"}}},
                        {MSG_ID, nullptr}};
                    ErrorBase Response = ErrorBase(
                        RequestID = nullptr, Code = Errors::InvalidRequest,
                        Message = "Invalid Request: Only one initialization request is allowed");
                    res->writeHead(HTTPStatus::BadRequest, {});
                    res->end(Response.Serialize());
                    return;
                }
                if (SessionIDGenerator()) { SessionID = SessionIDGenerator.value()(); }
                _initialized = true;

                // If we have a session ID and an onsessioninitialized handler, call it
                // immediately This is needed in cases where the server needs to keep track of
                // multiple sessions
                if (SessionID() && _onsessioninitialized()) {
                    _onsessioninitialized.value()(SessionID.value());
                }
            }

            // If an Mcp-Session-Id is returned by the server during initialization,
            // clients using the Streamable HTTP transport MUST include it
            // in the Mcp-Session-Id header on all of their subsequent HTTP requests.
            if (!isInitializationRequest && !validateSession(req, res)) { return; }

            // check if it contains requests
            bool hasRequests = false;
            for (const auto& msg : messages) {
                if (IsRequestBase(msg)) {
                    hasRequests = true;
                    break;
                }
            }

            if (!hasRequests) {
                // if it only contains notifications or responses, return 202
                res->writeHead(202, {});
                res->end(MSG_EMPTY);

                // handle each message
                for (const auto& message : messages) {
                    if (onmessage) { onmessage(message, authInfo); }
                }
            } else if (hasRequests) {
                // The default behavior is to use SSE streaming
                // but in some cases server will return JSON responses
                string StreamID = GenerateUUID();
                if (!_enableJsonResponse) {
                    unordered_map<string, string> headers = {
                        {TSPT_CONTENT_TYPE, TSPT_TEXT_EVENT_STREAM},
                        {"Cache-Control", "no-cache"},
                        {"Connection", "keep-alive"}};

                    // After initialization, always include the session ID if we have one
                    if (SessionID()) { headers["mcp-session-id"] = SessionID.value(); }

                    res->writeHead(HTTPStatus::Ok, headers);
                }
                // Store the response for this request to send messages back through this
                // connection We need to track by request ID to maintain the connection
                for (const auto& message : messages) {
                    if (IsRequestBase(message)) {
                        _streamMapping[StreamID] = res;
                        _requestToStreamMapping[message.data[MSG_ID]] = StreamID;
                    }
                }
                // Set up close handler for client disconnects
                res->on([this, StreamID]() { _streamMapping.erase(StreamID); });

                // handle each message
                for (const auto& message : messages) {
                    if (onmessage) { onmessage(message, authInfo); }
                }
                // The server SHOULD NOT close the SSE stream before sending all JSON-RPC
                // responses This will be handled by the send() method when responses are ready
            }
        } catch (const exception& error) {
            // return JSON-RPC formatted error
            JSON errorResponse = {{MSG_JSON_RPC, MSG_JSON_RPC_VERSION},
                                  {MSG_ERROR,
                                   {{MSG_CODE, Errors::ParseError},
                                    {MSG_MESSAGE, "Parse error"},
                                    {MSG_DATA, error.what()}}},
                                  {MSG_ID, nullptr}};
            res->writeHead(HTTPStatus::BadRequest, {});
            res->end(errorResponse.dump());
            if (onerror) { onerror(error); }
        }
    });
}

future<void> StreamableHTTPServerTransport::HandleDeleteRequest(const HTTP_Request& req,
                                                                shared_ptr<HTTP_Response> res) {
    return async(launch::async, [this, req, res]() {
        if (!validateSession(req, res)) { return; }
        close().wait();
        res->writeHead(HTTPStatus::Ok, {});
        res->end(MSG_EMPTY);
    });
}

bool StreamableHTTPServerTransport::ValidateSession(const HTTP_Request& req,
                                                    shared_ptr<HTTP_Response> res) {
    if (!SessionIDGenerator()) {
        // If the SessionIDGenerator ID is not set, the session management is disabled
        // and we don't need to validate the session ID
        return true;
    }
    if (!_initialized) {
        // If the server has not been initialized yet, reject all requests
        JSON errorResponse = {{MSG_JSON_RPC, MSG_JSON_RPC_VERSION},
                              {MSG_ERROR,
                               {{MSG_CODE, Errors::ConnectionClosed},
                                {MSG_MESSAGE, "Bad Request: Server not initialized"}}},
                              {MSG_ID, nullptr}};
        res->writeHead(HTTPStatus::BadRequest, {});
        res->end(errorResponse.dump());
        return false;
    }

    auto SessionIDIt = req.headers.find("mcp-session-id");

    if (SessionIDIt == req.headers.end()) {
        // Non-initialization requests without a session ID should return HTTPStatus::BadRequest Bad
        // Request
        JSON errorResponse = {{MSG_JSON_RPC, MSG_JSON_RPC_VERSION},
                              {MSG_ERROR,
                               {{MSG_CODE, Errors::ConnectionClosed},
                                {MSG_MESSAGE, "Bad Request: Mcp-Session-Id header is required"}}},
                              {MSG_ID, nullptr}};
        res->writeHead(HTTPStatus::BadRequest, {});
        res->end(errorResponse.dump());
        return false;
    } else if (SessionIDIt->second != SessionID.value_or(MSG_EMPTY)) {
        // Reject requests with invalid session ID with HTTPStatus::NotFound
        JSON errorResponse = {
            {MSG_JSON_RPC, MSG_JSON_RPC_VERSION},
            {MSG_ERROR, {{MSG_CODE, Errors::RequestTimeout}, {MSG_MESSAGE, "Session not found"}}},
            {MSG_ID, nullptr}};
        res->writeHead(HTTPStatus::NotFound, {});
        res->end(errorResponse.dump());
        return false;
    }

    return true;
}

future<void> StreamableHTTPServerTransport::Close() override {
    return async(launch::async, [this]() {
        // Close all SSE connections
        for (auto& [id, response] : _streamMapping) { response->end(MSG_EMPTY); }
        _streamMapping.clear();

        // Clear any pending responses
        _requestResponseMap.clear();
        if (onclose) { onclose(); }
    });
}

future<void> StreamableHTTPServerTransport::Send(
    const MessageBase& InMessage,
    const optional<RequestID>& InRelatedRequestID = nullopt) override {
    return async(launch::async, [this, InMessage, InRelatedRequestID]() {
        optional<RequestID> RequestID = InRelatedRequestID;

        bool isResponse = IsResponseBase(InMessage);
        if (isResponse) {
            // If the message is a response, use the request ID from the message
            if (InMessage.data.contains(MSG_ID)) { RequestID = InMessage.data[MSG_ID]; }
        }

        // Check if this message should be sent on the standalone SSE stream (no request ID)
        // Ignore notifications from tools (which have relatedRequestID set)
        // Those will be sent via dedicated response SSE streams
        if (!RequestID()) {
            // For standalone SSE streams, we can only send requests and notifications
            if (isResponse) {
                throw runtime_error("Cannot send a response on a standalone SSE stream unless "
                                    "resuming a previous client request");
            }
            auto standaloneSseIt = _streamMapping.find(_standaloneSseStreamID);
            if (standaloneSseIt == _streamMapping.end()) {
                // The spec says the server MAY send messages on the stream, so it's ok to
                // discard if no stream
                return;
            }

            // Generate and store event ID if event store is provided
            optional<string> EventID;
            if (_eventStore) {
                // Stores the event and gets the generated event ID
                auto EventIDFuture = _eventStore->storeEvent(_standaloneSseStreamID, InMessage);
                EventID = EventIDFuture.get();
            }

            // Send the message to the standalone SSE stream
            writeSSEEvent(standaloneSseIt->second, InMessage, EventID);
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
                auto EventIDFuture = _eventStore->storeEvent(StreamIDIt->second, InMessage);
                EventID = EventIDFuture.get();
            }
            if (response) {
                // Write the event to the response stream
                writeSSEEvent(response, InMessage, EventID);
            }
        }

        if (isResponse) {
            _requestResponseMap[RequestID.value()] = InMessage;

            vector<RequestID> relatedIds;
            for (const auto& [id, StreamID] : _requestToStreamMapping) {
                if (_streamMapping[StreamID] == response) { relatedIds.push_back(id); }
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
                    throw runtime_error("No connection established for request ID: "
                                        + RequestID.value());
                }
                if (_enableJsonResponse) {
                    // All responses ready, send as JSON
                    unordered_map<string, string> headers = {{TSPT_CONTENT_TYPE, TSPT_APP_JSON}};
                    if (SessionID()) { headers["mcp-session-id"] = SessionID.value(); }

                    vector<JSON> responses;
                    for (const auto& id : relatedIds) {
                        responses.push_back(_requestResponseMap[id].data);
                    }

                    response->writeHead(HTTPStatus::Ok, headers);
                    if (responses.size() == 1) {
                        response->end(responses[0].dump());
                    } else {
                        JSON responseArray = responses;
                        response->end(responseArray.dump());
                    }
                } else {
                    // End the SSE stream
                    response->end(MSG_EMPTY);
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

// ##########################################################
// ##########################################################
// ===================== CLIENT SECTION =====================
// ##########################################################
// ##########################################################

future<void> StreamableHTTPClientTransport::authThenStart() {
    return async(launch::async, [this]() {
        if (!auth_provider_) { throw UnauthorizedError("No auth provider"); }

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

future<map<string, string>> StreamableHTTPClientTransport::commonHeaders() {
    return async(launch::async, [this]() {
        map<string, string> headers;

        if (auth_provider_) {
            // TODO: Fix External Ref: tokens() method
            // auto tokens = auth_provider_->tokens();
            // if (tokens) {
            //     headers["Authorization"] = "Bearer " + tokens->access_token;
            // }
        }

        if (session_id_) { headers["mcp-session-id"] = *session_id_; }

        // Merge with request headers
        for (const auto& [key, value] : request_headers_) { headers[key] = value; }

        return headers;
    });
}

future<void> StreamableHTTPClientTransport::startOrAuthSse(const TransportSendOptions& options) {
    return async(launch::async, [this, options]() {
        try {
            auto headers = commonHeaders().get();
            headers[TSPT_ACCEPT] = TSPT_TEXT_EVENT_STREAM;

            if (options.resumptionToken) { headers["last-event-id"] = *options.resumptionToken; }

            // TODO: Fix External Ref: HTTP GET implementation
            // auto response = httpGet(url_, headers, abort_requested_);

            // if (!response.ok) {
            //     if (response.status == HTTPStatus::Unauthorized && auth_provider_) {
            //         return authThenStart().get();
            //     }
            //     if (response.status == HTTPStatus::MethodNotAllowed) {
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

int StreamableHTTPClientTransport::getNextReconnectionDelay(int attempt) {
    int initialDelay = reconnection_options_.initialReconnectionDelay;
    double growFactor = reconnection_options_.reconnectionDelayGrowFactor;
    int maxDelay = reconnection_options_.maxReconnectionDelay;

    return min(static_cast<int>(initialDelay * pow(growFactor, attempt)), maxDelay);
}

void StreamableHTTPClientTransport::scheduleReconnection(const TransportSendOptions& options,
                                                         int attemptCount = 0) {
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

void StreamableHTTPClientTransport::handleSseStream(const string& streamData,
                                                    const TransportSendOptions& options) {
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
        //     if (event.event.empty() || event.event == MSG_MESSAGE) {
        //         try {
        //             // TODO: Fix External Ref: MessageBaseSchema validation
        //             auto message = JSON::parse(event.data);
        //             if (options.replayRequestID && IsResponseBase(message)) {
        //                 message[MSG_ID] = *options.replayRequestID;
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
                TransportSendOptions reconnectOptions = {*lastEventID, options.onresumptiontoken,
                                                         options.replayRequestID};
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

future<void> StreamableHTTPClientTransport::start() {
    return async(launch::async, [this]() {
        if (abort_requested_) {
            throw runtime_error(
                "StreamableHTTPClientTransport already started! If using Client class, note "
                "that connect() calls start() automatically.");
        }
        abort_requested_ = false;
    });
}

future<void> StreamableHTTPClientTransport::finishAuth(const string& authorizationCode) {
    return async(launch::async, [this, authorizationCode]() {
        if (!auth_provider_) { throw UnauthorizedError("No auth provider"); }

        // TODO: Fix External Ref: auth function with authorizationCode
        // auto result = auth(auth_provider_, {url_, authorizationCode,
        // resource_metadata_url_}); if (result != "AUTHORIZED") {
        //     throw UnauthorizedError("Failed to authorize");
        // }
    });
}

future<void> StreamableHTTPClientTransport::close() {
    return async(launch::async, [this]() {
        abort_requested_ = true;
        if (onclose) onclose();
    });
}

future<void> StreamableHTTPClientTransport::send(const MessageBase& message,
                                                 const SendOptions& options = {}) {
    return async(launch::async, [this, message, options]() {
        try {
            if (options.resumptionToken) {
                TransportSendOptions sseOptions = {
                    options.resumptionToken, options.onresumptiontoken,
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
            headers[TSPT_CONTENT_TYPE] = TSPT_APP_JSON;
            headers[TSPT_ACCEPT] = TSPT_APP_JSON_AND_EVENT_STREAM;

            // TODO: Fix External Ref: HTTP POST implementation
            // string body = JSON::stringify(message);
            // auto response = httpPost(url_, headers, body, abort_requested_);

            // Handle session ID received during initialization
            // if (response.headers.count("mcp-session-id")) {
            //     session_id_ = response.headers["mcp-session-id"];
            // }

            // if (!response.ok) {
            //     if (response.status == HTTPStatus::Unauthorized && auth_provider_) {
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

future<void> StreamableHTTPClientTransport::send(const vector<MessageBase>& messages,
                                                 const SendOptions& options = {}) {
    return async(launch::async, [this, messages, options]() {
        // TODO: Implement batch message sending
        // Similar to single message but handle array serialization
    });
}

future<void> StreamableHTTPClientTransport::terminateSession() {
    return async(launch::async, [this]() {
        if (!session_id_) {
            return; // No session to terminate
        }

        try {
            auto headers = commonHeaders().get();

            // TODO: Fix External Ref: HTTP DELETE implementation
            // auto response = httpDelete(url_, headers, abort_requested_);

            // Handle HTTPStatus::MethodNotAllowed as valid response per spec
            // if (!response.ok && response.status != HTTPStatus::MethodNotAllowed) {
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

MCP_NAMESPACE_END