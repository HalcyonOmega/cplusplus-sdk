#include "Communication/Transport/StreamableHTTPTransport.h"

#include "Communication/Utilities/TransportUtilities.h"
#include "Core.h"
#include "Core/Constants/TransportConstants.h"

MCP_NAMESPACE_BEGIN

future<void> StreamableHTTPServerTransport::Start() {
    if (m_Started) { CallOnError("Transport already started"); }
    m_Started = true;
    return async(launch::deferred, []() {});
}

future<void>
StreamableHTTPServerTransport::HandleRequest(const HTTP::Request& InRequest,
                                             shared_ptr<HTTP::Response> InResponse,
                                             const optional<JSON>& ParsedBody = nullopt) {
    if (InRequest.Method == MTHD_POST) {
        return HandlePostRequest(InRequest, InResponse, ParsedBody);
    } else if (InRequest.Method == MTHD_GET) {
        return HandleGetRequest(InRequest, InResponse);
    } else if (InRequest.Method == MTHD_DELETE) {
        return HandleDeleteRequest(InRequest, InResponse);
    } else {
        return HandleUnsupportedRequest(InResponse);
    }
}

future<void>
StreamableHTTPServerTransport::HandleGetRequest(const HTTP::Request& InRequest,
                                                shared_ptr<HTTP::Response> InResponse) {
    return async(launch::async, [this, InRequest, InResponse]() {
        // The client MUST include an Accept header, listing text/event-stream as a supported
        // content type.
        auto acceptIt = InRequest.Headers.find(TSPT_ACCEPT);
        if (acceptIt == InRequest.Headers.end()
            || acceptIt->second.find(TSPT_TEXT_EVENT_STREAM) == string::npos) {
            // TODO: Below -> Error should have {MSG_ID, nullptr}};
            ErrorBase Error(Errors::ConnectionClosed,
                            "Not Acceptable: Client must accept text/event-stream", nullptr);
            InResponse->WriteHead(HTTP::Status::NotAcceptable, {});
            InResponse->End(Error.Serialize());
            return;
        }

        // If an Mcp-Session-Id is returned by the server during initialization,
        // clients using the Streamable HTTP transport MUST include it
        // in the Mcp-Session-Id header on all of their subsequent HTTP requests.
        if (!ValidateSession(InRequest, InResponse)) { return; }

        // Handle resumability: check for Last-Event-ID header
        if (m_EventStore) {
            auto lastEventIDIt = InRequest.Headers.find("last-event-id");
            if (lastEventIDIt != InRequest.Headers.end()) {
                ReplayEvents(lastEventIDIt->second, InResponse).wait();
                return;
            }
        }

        // The server MUST either return Content-Type: text/event-stream in response to this
        // HTTP GET, or else return HTTP HTTP::Status::MethodNotAllowed Method Not Allowed
        HTTP::Headers Headers = {{TSPT_CONTENT_TYPE, TSPT_TEXT_EVENT_STREAM},
                                 {"Cache-Control", "no-cache, no-transform"},
                                 {"Connection", "keep-alive"}};

        // After initialization, always include the session ID if we have one
        if (SessionID()) { Headers["mcp-session-id"] = SessionID.value(); }

        // Check if there's already an active standalone SSE stream for this session
        if (m_StreamMapping.find(m_StandaloneSSEStreamID) != m_StreamMapping.end()) {
            // Only one GET SSE stream is allowed per session
            ErrorBase Error(Errors::ConnectionClosed,
                            "Conflict: Only one SSE stream is allowed per session", nullptr);
            InResponse->WriteHead(HTTP::Status::Conflict, {});
            InResponse->End(Error.Serialize());
            return;
        }

        // We need to send headers immediately as messages will arrive much later,
        // otherwise the client will just wait for the first message
        InResponse->WriteHead(HTTP::Status::Ok, Headers);
        InResponse->FlushHeaders();

        // Assign the response to the standalone SSE stream
        m_StreamMapping[m_StandaloneSSEStreamID] = InResponse;
        // Set up close handler for client disconnects
        InResponse->On([this]() { m_StreamMapping.erase(m_StandaloneSSEStreamID); });
    });
}

future<void> StreamableHTTPServerTransport::ReplayEvents(const string& InLastEventID,
                                                         shared_ptr<HTTP::Response> InResponse) {
    return async(launch::async, [this, InLastEventID, InResponse]() {
        if (!m_EventStore) { return; }
        try {
            HTTP::Headers Headers = {{TSPT_CONTENT_TYPE, TSPT_TEXT_EVENT_STREAM},
                                     {"Cache-Control", "no-cache, no-transform"},
                                     {"Connection", "keep-alive"}};

            if (SessionID()) { Headers["mcp-session-id"] = SessionID.value(); }
            InResponse->WriteHead(HTTP::Status::Ok, Headers);
            InResponse->FlushHeaders();

            auto StreamIDFuture = m_EventStore->ReplayEventsAfter(
                InLastEventID,
                [this, InResponse](const EventID& InEventID,
                                   const MessageBase& InMessage) -> future<void> {
                    return async(launch::async, [this, InResponse, InEventID, InMessage]() {
                        if (!WriteSSEEvent(InResponse, InMessage, InEventID)) {
                            CallOnError("Failed replay events");
                            InResponse->End(MSG_EMPTY);
                        }
                    });
                });

            auto StreamID = StreamIDFuture.get();
            m_StreamMapping[StreamID] = InResponse;
        } catch (const exception& error) { CallOnError(error); }
    });
}

bool StreamableHTTPServerTransport::WriteSSEEvent(shared_ptr<HTTP::Response> InResponse,
                                                  const MessageBase& InMessage,
                                                  const optional<string>& InEventID = nullopt) {
    string eventData = "event: message\n";
    // Include event ID if provided - this is important for resumability
    if (InEventID()) { eventData += "id: " + InEventID.value() + "\n"; }
    eventData += "data: " + InMessage.data.dump() + "\n\n";

    return InResponse->Write(eventData);
}

future<void>
StreamableHTTPServerTransport::HandleUnsupportedRequest(shared_ptr<HTTP::Response> InResponse) {
    return async(launch::async, [InResponse]() {
        HTTP::Headers Headers = {{"Allow", "GET, POST, DELETE"}};
        // TODO: Below -> Error should have {MSG_ID, nullptr}};
        ErrorBase Error(Errors::ConnectionClosed, "Method not allowed.", nullptr);
        InResponse->WriteHead(HTTP::Status::MethodNotAllowed, Headers);
        InResponse->End(Error.Serialize());
    });
}

future<void>
StreamableHTTPServerTransport::HandlePostRequest(const HTTP::Request& InRequest,
                                                 shared_ptr<HTTP::Response> InResponse,
                                                 const optional<JSON>& ParsedBody = nullopt) {
    return async(launch::async, [this, InRequest, InResponse, ParsedBody]() {
        try {
            // Validate the Accept header
            auto acceptIt = InRequest.Headers.find(TSPT_ACCEPT);
            // The client MUST include an Accept header, listing both application/json and
            // text/event-stream as supported content types.
            if (acceptIt == InRequest.Headers.end()
                || acceptIt->second.find(TSPT_APP_JSON) == string::npos
                || acceptIt->second.find(TSPT_TEXT_EVENT_STREAM) == string::npos) {
                // TODO: Below -> Error should have {MSG_ID, nullptr}};
                ErrorBase Error(Errors::ConnectionClosed,
                                "Not Acceptable: Client must accept both application/json and "
                                "text/event-stream",
                                nullptr);
                InResponse->WriteHead(HTTP::Status::NotAcceptable, {});
                InResponse->End(Error.Serialize());
                return;
            }

            auto ctIt = InRequest.Headers.find(TSPT_CONTENT_TYPE);
            if (ctIt == InRequest.Headers.end()
                || ctIt->second.find(TSPT_APP_JSON) == string::npos) {
                // TODO: Below -> Error should have {MSG_ID, nullptr}};
                ErrorBase Error(Errors::ConnectionClosed,
                                "Unsupported Media Type: Content-Type must be application/json",
                                nullptr);
                InResponse->WriteHead(HTTP::Status::UnsupportedMediaType, {});
                InResponse->End(Error.Serialize());
                return;
            }

            optional<AuthInfo> AuthInfo = InRequest.Auth;

            JSON rawMessage;
            if (ParsedBody()) {
                rawMessage = ParsedBody.value();
            } else {
                try {
                    rawMessage = JSON::parse(InRequest.Body);
                } catch (const JSON::parse_error& e) {
                    CallOnError("JSON parse error: " + string(e.what()));
                }
            }

            vector<MessageBase> Messages;

            // handle batch and single messages
            if (rawMessage.is_array()) {
                for (const auto& msg : rawMessage) {
                    // TODO: Fix External Ref: MessageBaseSchema validation
                    Messages.emplace_back(msg);
                }
            } else {
                // TODO: Fix External Ref: MessageBaseSchema validation
                Messages.emplace_back(rawMessage);
            }

            // Check if this is an initialization request
            // TODO: Fix External Ref: isInitializeRequest function
            bool isInitializationRequest = false;
            for (const auto& msg : Messages) {
                // Placeholder check - would need actual implementation
                if (msg.data.contains(MSG_METHOD) && msg.data[MSG_METHOD] == MTHD_INITIALIZE) {
                    isInitializationRequest = true;
                    break;
                }
            }

            if (isInitializationRequest) {
                // If it's a server with session management and the session ID is already set we
                // should reject the request to avoid re-initialization.
                if (m_Initialized && SessionID()) {
                    // TODO: Below -> Error should have {MSG_ID, nullptr}};
                    ErrorBase Error(Errors::InvalidRequest,
                                    "Invalid Request: Server already initialized", nullptr);
                    InResponse->WriteHead(HTTP::Status::BadRequest, {});
                    InResponse->End(Error.Serialize());
                    return;
                }
                if (messages.size() > 1) {
                    // TODO: Below -> Error should have {MSG_ID, nullptr}};
                    ErrorBase Error(Errors::InvalidRequest,
                                    "Invalid Request: Only one initialization request is allowed",
                                    nullptr);
                    InResponse->WriteHead(HTTP::Status::BadRequest, {});
                    InResponse->End(Error.Serialize());
                    return;
                }
                if (m_SessionIDGenerator()) { m_SessionID = m_SessionIDGenerator.value()(); }
                m_Initialized = true;

                // If we have a session ID and an onsessioninitialized handler, call it
                // immediately This is needed in cases where the server needs to keep track of
                // multiple sessions
                if (SessionID() && m_OnSessionInitialized()) {
                    m_OnSessionInitialized.value()(SessionID.value());
                }
            }

            // If an Mcp-Session-Id is returned by the server during initialization,
            // clients using the Streamable HTTP transport MUST include it
            // in the Mcp-Session-Id header on all of their subsequent HTTP requests.
            if (!isInitializationRequest && !ValidateSession(InRequest, InResponse)) { return; }

            // check if it contains requests
            bool hasRequests = false;
            for (const auto& msg : Messages) {
                if (IsRequestBase(msg)) {
                    hasRequests = true;
                    break;
                }
            }

            if (!hasRequests) {
                // if it only contains notifications or responses, return 202
                InResponse->WriteHead(HTTP::Status::Accepted, {});
                InResponse->End(MSG_EMPTY);

                // handle each message
                for (const auto& message : Messages) {
                    if (m_OnMessage) { m_OnMessage(message, AuthInfo); }
                }
            } else if (hasRequests) {
                // The default behavior is to use SSE streaming
                // but in some cases server will return JSON responses
                string StreamID = GenerateUUID();
                if (!m_EnableJSONResponse) {
                    HTTP::Headers Headers = {{TSPT_CONTENT_TYPE, TSPT_TEXT_EVENT_STREAM},
                                             {"Cache-Control", "no-cache"},
                                             {"Connection", "keep-alive"}};

                    // After initialization, always include the session ID if we have one
                    if (SessionID()) { Headers["mcp-session-id"] = SessionID.value(); }

                    InResponse->WriteHead(HTTP::Status::Ok, Headers);
                }
                // Store the response for this request to send messages back through this
                // connection We need to track by request ID to maintain the connection
                for (const auto& message : messages) {
                    if (IsRequestBase(message)) {
                        m_StreamMapping[StreamID] = InResponse;
                        m_RequestToStreamMapping[message.data[MSG_ID]] = StreamID;
                    }
                }
                // Set up close handler for client disconnects
                InResponse->On([this, StreamID]() { m_StreamMapping.erase(StreamID); });

                // handle each message
                for (const auto& message : Messages)
                    if (m_OnMessage) { m_OnMessage(message, AuthInfo); }
            }
            // The server SHOULD NOT close the SSE stream before sending all JSON-RPC
            // responses This will be handled by the send() method when responses are ready
        }
    } catch (const exception& error) {
        // return JSON-RPC formatted error
        // TODO: Below -> Error should have {MSG_ID, nullptr}};
        ErrorBase Error(Errors::ParseError, "Parse error", error.what());
        InResponse->WriteHead(HTTP::Status::BadRequest, {});
        InResponse->End(Error.Serialize());
        CallOnError(Error);
    }
}
});
}

future<void>
StreamableHTTPServerTransport::HandleDeleteRequest(const HTTP::Request& InRequest,
                                                   shared_ptr<HTTP::Response> InResponse) {
    return async(launch::async, [this, InRequest, InResponse]() {
        if (!ValidateSession(InRequest, InResponse)) { return; }
        Close().wait();
        InResponse->WriteHead(HTTP::Status::Ok), {});
        InResponse->End(MSG_EMPTY);
    });
}

bool StreamableHTTPServerTransport::ValidateSession(const HTTP::Request& InRequest,
                                                    shared_ptr<HTTP::Response> InResponse) {
    if (!m_SessionIDGenerator) {
        // If the SessionIDGenerator ID is not set, the session management is disabled
        // and we don't need to validate the session ID
        return true;
    }
    if (!m_Initialized) {
        // If the server has not been initialized yet, reject all requests
        // TODO: Below -> Error should have {MSG_ID, nullptr}};
        ErrorBase Error(Errors::ConnectionClosed, "Bad Request: Server not initialized", nullptr);
        InResponse->WriteHead(HTTP::Status::BadRequest, {});
        InResponse->End(Error.Serialize());
        return false;
    }

    auto SessionIDIt = InRequest.Headers.find("mcp-session-id");

    if (SessionIDIt == InRequest.Headers.end()) {
        // Non-initialization requests without a session ID should return HTTP::Status::BadRequest
        // Bad Request
        // TODO: Below -> Error should have {MSG_ID, nullptr}};
        ErrorBase Error(Errors::ConnectionClosed, "Bad Request: Mcp-Session-Id header is required",
                        nullptr);
        InResponse->WriteHead(HTTP::Status::BadRequest, {});
        InResponse->End(Error.Serialize());
        return false;
    } else if (SessionIDIt->second != SessionID.value_or(MSG_EMPTY)) {
        // Reject requests with invalid session ID with HTTP::Status::NotFound
        // TODO: Below -> Error should have {MSG_ID, nullptr}};
        ErrorBase Error(Errors::RequestTimeout, "Session not found", nullptr);
        InResponse->WriteHead(HTTP::Status::NotFound, {});
        InResponse->End(Error.Serialize());
        return false;
    }

    return true;
}

future<void> StreamableHTTPServerTransport::Close() override {
    return async(launch::async, [this]() {
        // Close all SSE connections
        for (auto& [id, response] : m_StreamMapping) { response->End(MSG_EMPTY); }
        m_StreamMapping.clear();

        // Clear any pending responses
        m_RequestResponseMap.clear();
        if (OnClose) { OnClose(); }
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
            auto standaloneSseIt = m_StreamMapping.find(m_StandaloneSSEStreamID);
            if (standaloneSseIt == m_StreamMapping.end()) {
                // The spec says the server MAY send messages on the stream, so it's ok to
                // discard if no stream
                return;
            }

            // Generate and store event ID if event store is provided
            optional<string> EventID;
            if (m_EventStore) {
                // Stores the event and gets the generated event ID
                auto EventIDFuture = m_EventStore->storeEvent(m_StandaloneSSEStreamID, InMessage);
                EventID = EventIDFuture.get();
            }

            // Send the message to the standalone SSE stream
            WriteSSEEvent(standaloneSseIt->second, InMessage, EventID);
            return;
        }

        // Get the response for this request
        auto StreamIDIt = m_RequestToStreamMapping.find(RequestID.value());
        if (StreamIDIt == m_RequestToStreamMapping.end()) {
            throw runtime_error("No connection established for request ID: " + RequestID.value());
        }

        auto responseIt = m_StreamMapping.find(StreamIDIt->second);
        if (responseIt == m_StreamMapping.end()) {
            throw runtime_error("No connection established for request ID: " + RequestID.value());
        }

        auto response = responseIt->second;

        if (!m_EnableJSONResponse) {
            // For SSE responses, generate event ID if event store is provided
            optional<string> EventID;

            if (m_EventStore) {
                auto EventIDFuture = m_EventStore->storeEvent(StreamIDIt->second, InMessage);
                EventID = EventIDFuture.get();
            }
            if (response) {
                // Write the event to the response stream
                WriteSSEEvent(response, InMessage, EventID);
            }
        }

        if (isResponse) {
            m_RequestResponseMap[RequestID.value()] = InMessage;

            vector<RequestID> relatedIds;
            for (const auto& [id, StreamID] : m_RequestToStreamMapping) {
                if (m_StreamMapping[StreamID] == response) { relatedIds.push_back(id); }
            }

            // Check if we have responses for all requests using this connection
            bool allResponsesReady = true;
            for (const auto& id : relatedIds) {
                if (m_RequestResponseMap.find(id) == m_RequestResponseMap.end()) {
                    allResponsesReady = false;
                    break;
                }
            }

            if (allResponsesReady) {
                if (!response) {
                    throw runtime_error("No connection established for request ID: "
                                        + RequestID.value());
                }
                if (m_EnableJSONResponse) {
                    // All responses ready, send as JSON
                    HTTP::Headers Headers = {{TSPT_CONTENT_TYPE, TSPT_APP_JSON}};
                    if (SessionID()) { Headers["mcp-session-id"] = SessionID.value(); }

                    vector<JSON> responses;
                    for (const auto& id : relatedIds) {
                        responses.push_back(m_RequestResponseMap[id].data);
                    }

                    response->WriteHead(HTTP::Status::Ok, Headers);
                    if (responses.size() == 1) {
                        response->End(responses[0].dump());
                    } else {
                        JSON responseArray = responses;
                        response->End(responseArray.dump());
                    }
                } else {
                    // End the SSE stream
                    response->End(MSG_EMPTY);
                }
                // Clean up
                for (const auto& id : relatedIds) {
                    m_RequestResponseMap.erase(id);
                    m_RequestToStreamMapping.erase(id);
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
        if (!m_AuthProvider) { throw UnauthorizedError("No auth provider"); }

        try {
            // TODO: Fix External Ref: auth function
            // AuthResult result = auth(auth_provider_, {url_, resource_metadata_url_});
            // if (result != "AUTHORIZED") {
            //     throw UnauthorizedError();
            // }
        } catch (const exception& error) {
            CallOnError(error);
            throw;
        }

        return startOrAuthSse({nullopt, nullptr, nullopt});
    });
}

future<HTTP::Headers> StreamableHTTPClientTransport::CommonHeaders() {
    return async(launch::async, [this]() {
        HTTP::Headers Headers;

        if (m_AuthProvider) {
            // TODO: Fix External Ref: tokens() method
            // auto tokens = auth_provider_->tokens();
            // if (tokens) {
            //     Headers["Authorization"] = "Bearer " + tokens->access_token;
            // }
        }

        if (SessionID()) { Headers["mcp-session-id"] = *SessionID; }

        // Merge with request Headers
        for (const auto& [key, value] : m_RequestHeaders) { Headers[key] = value; }

        return Headers;
    });
}

future<void> StreamableHTTPClientTransport::startOrAuthSse(const TransportSendOptions& options) {
    return async(launch::async, [this, options]() {
        try {
            auto Headers = commonHeaders().get();
            Headers[TSPT_ACCEPT] = TSPT_TEXT_EVENT_STREAM;

            if (options.resumptionToken) { Headers["last-event-id"] = *options.resumptionToken; }

            // TODO: Fix External Ref: HTTP GET implementation
            // auto response = httpGet(url_, Headers, abort_requested_);

            // if (!response.ok) {
            //     if (response.status == HTTP::Status::Unauthorized && auth_provider_) {
            //         return authThenStart().get();
            //     }
            //     if (response.status == HTTP::Status::MethodNotAllowed) {
            //         return; // Server doesn't support SSE
            //     }
            //     throw StreamableHTTPError(response.status,
            //         "Failed to open SSE stream: " + response.statusText);
            // }

            // handleSseStream(response.body, options);
        } catch (const exception& error) {
            CallOnError(error);
            throw;
        }
    });
}

int StreamableHTTPClientTransport::getNextReconnectionDelay(int attempt) {
    int initialDelay = m_ReconnectionOptions.initialReconnectionDelay;
    double growFactor = m_ReconnectionOptions.reconnectionDelayGrowFactor;
    int maxDelay = m_ReconnectionOptions.maxReconnectionDelay;

    return min(static_cast<int>(initialDelay * pow(growFactor, attempt)), maxDelay);
}

void StreamableHTTPClientTransport::scheduleReconnection(const TransportSendOptions& options,
                                                         int attemptCount = 0) {
    int maxRetries = m_ReconnectionOptions.maxRetries;

    if (maxRetries > 0 && attemptCount >= maxRetries) {
        string msg = "Maximum reconnection attempts (" + to_string(maxRetries) + ") exceeded.";
        CallOnError(msg);
        return;
    }

    int delay = getNextReconnectionDelay(attemptCount);

    thread([this, options, attemptCount, delay]() {
        this_thread::sleep_for(chrono::milliseconds(delay));

        try {
            startOrAuthSse(options).get();
        } catch (const exception& error) {
            string msg = "Failed to reconnect SSE stream: " + string(error.what());
            CallOnError(msg);
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
        string msg = "SSE stream disconnected: " + string(error.what());
        CallOnError(msg);

        if (!abort_requested_ && lastEventID) {
            try {
                TransportSendOptions reconnectOptions = {*lastEventID, options.onresumptiontoken,
                                                         options.replayRequestID};
                scheduleReconnection(reconnectOptions, 0);
            } catch (const exception& reconnectError) {
                string msg = "Failed to reconnect: " + string(reconnectError.what());
                CallOnError(msg);
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
        if (!m_AuthProvider) { throw UnauthorizedError("No auth provider"); }

        // TODO: Fix External Ref: auth function with authorizationCode
        // auto result = auth(auth_provider_, {url_, authorizationCode,
        // resource_metadata_url_}); if (result != "AUTHORIZED") {
        //     throw UnauthorizedError("Failed to authorize");
        // }
    });
}

future<void> StreamableHTTPClientTransport::Close() {
    return async(launch::async, [this]() {
        abort_requested_ = true;
        CallOnClose();
    });
}

future<void> StreamableHTTPClientTransport::Send(const MessageBase& InMessage,
                                                 const TransportSendOptions& InOptions = {}) {
    return async(launch::async, [this, InMessage, InOptions]() {
        try {
            if (InOptions.ResumptionToken) {
                TransportSendOptions SSEOptions = {
                    InOptions.ResumptionToken, InOptions.OnResumptionToken,
                    nullopt // TODO: Extract message ID if it's a request
                };

                async(launch::async, [this, &SSEOptions]() {
                    try {
                        startOrAuthSse(SSEOptions).get();
                    } catch (const exception& err) { CallOnError(err); }
                });
                return;
            }

            auto Headers = CommonHeaders().get();
            Headers[TSPT_CONTENT_TYPE] = TSPT_APP_JSON;
            Headers[TSPT_ACCEPT] = TSPT_APP_JSON_AND_EVENT_STREAM;

            // TODO: Fix External Ref: HTTP POST implementation
            // string body = JSON::stringify(message);
            // auto response = httpPost(url_, Headers, body, abort_requested_);

            // Handle session ID received during initialization
            // if (response.Headers.count("mcp-session-id")) {
            //     session_id_ = response.Headers["mcp-session-id"];
            // }

            // if (!response.ok) {
            //     if (response.status == HTTP::Status::Unauthorized && auth_provider_) {
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
            CallOnError(error);
            throw;
        }
    });
}

future<void> StreamableHTTPClientTransport::Send(const vector<MessageBase>& InMessages,
                                                 const TransportSendOptions& InOptions = {}) {
    return async(launch::async, [this, InMessages, InOptions]() {
        // TODO: Implement batch message sending
        // Similar to single message but handle array serialization
    });
}

future<void> StreamableHTTPClientTransport::terminateSession() {
    return async(launch::async, [this]() {
        if (!SessionID()) {
            return; // No session to terminate
        }

        try {
            auto Headers = CommonHeaders().get();

            // TODO: Fix External Ref: HTTP DELETE implementation
            // auto response = httpDelete(url_, headers, abort_requested_);

            // Handle HTTP::Status::MethodNotAllowed as valid response per spec
            // if (!response.ok && response.status != HTTP::Status::MethodNotAllowed) {
            //     throw StreamableHTTPError(response.status,
            //         "Failed to terminate session: " + response.statusText);
            // }

            SessionID = nullopt;
        } catch (const exception& error) {
            CallOnError(error);
            throw;
        }
    });
}

MCP_NAMESPACE_END