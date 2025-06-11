#include "Communication/Transport__DT/SSE.h"

#include "Core/Constants/TransportConstants.h"

MCP_NAMESPACE_BEGIN

// TODO: Fix External Ref: HTTP Server Response functionality
// TODO: Fix External Ref: HTTP Request/Response handling
// TODO: Fix External Ref: Server-Sent Events implementation
// TODO: Fix External Ref: Raw body parsing
// TODO: Fix External Ref: Content-Type parsing

void ServerResponse::writeHead(int status_code,
                               const optional<map<string, string>>& headers = nullopt) {
    // TODO: Implement HTTP response header writing
}

void ServerResponse::write(const string& data) {
    // TODO: Implement HTTP response writing
}

void ServerResponse::end(const optional<string>& data = nullopt) {
    // TODO: Implement HTTP response ending
    is_ended = true;
}

void ServerResponse::on(const string& event, optional<function<void()>> callback) {
    // TODO: Implement event handling
}

// TODO: Fix External Ref: UUID generation
string generateRandomUUID() {
    // TODO: Implement proper UUID generation
    // For now, return a placeholder
    return "placeholder-uuid-" + to_string(rand());
}

string URLHelper::addSessionParam(const string& endpoint, const string& session_id) {
    // TODO: Implement proper URL parameter handling like TypeScript URL class
    string separator = (endpoint.find('?') != string::npos) ? "&" : "?";
    return endpoint + separator + "sessionId=" + session_id;
}

ContentTypeResult parseContentType(const string& content_type_header) {
    // TODO: Implement content-type parsing equivalent to contentType.parse()
    ContentTypeResult result;
    result.type = TSPT_APP_JSON;            // Placeholder
    result.parameters["charset"] = "utf-8"; // Placeholder
    return result;
}

// TODO: Fix External Ref: Raw body parsing
string getRawBodyEquivalent(IncomingMessage* req, const string& limit, const string& encoding) {
    // TODO: Implement raw body parsing with size limit
    throw runtime_error("Raw body parsing not implemented");
}

void start() {
    if (_sseResponse.has_value()) {
        throw runtime_error("SSEServerTransport already started! If using Server class, note "
                            "that connect() calls start() automatically.");
    }

    map<string, string> headers = {{TSPT_CONTENT_TYPE, TSPT_TEXT_EVENT_STREAM},
                                   {"Cache-Control", "no-cache, no-transform"},
                                   {"Connection", "keep-alive"}};
    res->writeHead(HTTPStatus::Ok, headers);

    // Send the endpoint event
    // Use a dummy base URL because this._endpoint is relative.
    // This allows using URL/URLSearchParams for robust parameter handling.
    string dummyBase = "http://localhost"; // Any valid base works
    string relativeUrlWithSession = URLHelper::addSessionParam(_endpoint, _sessionId);

    string endpointEvent = "event: endpoint\ndata: " + relativeUrlWithSession + "\n\n";
    res->write(endpointEvent);

    _sseResponse = res;
    res->on("close", [this]() {
        _sseResponse = nullopt;
        if (onclose.has_value()) { onclose.value()(); }
    });
}

void handlePostMessage(IncomingMessage* req, ServerResponse* res_param,
                       const optional<JSON>& parsedBody = nullopt) {
    if (!_sseResponse.has_value()) {
        string message = "SSE connection not established";
        res_param->writeHead(500);
        res_param->end(message);
        throw runtime_error(message);
    }

    optional<AuthInfo> authInfo = req->auth;

    JSON body;
    try {
        string contentTypeHeader = MSG_NULL;
        auto ctIt = req->headers.find(TSPT_CONTENT_TYPE);
        if (ctIt != req->headers.end()) { contentTypeHeader = ctIt->second; }

        ContentTypeResult ct = parseContentType(contentTypeHeader);
        if (ct.type != TSPT_APP_JSON) {
            throw runtime_error("Unsupported content-type: " + ct.type);
        }

        if (parsedBody.has_value()) {
            body = parsedBody.value();
        } else {
            string encoding = "utf-8";
            auto charsetIt = ct.parameters.find("charset");
            if (charsetIt != ct.parameters.end()) { encoding = charsetIt->second; }

            string rawBody = getRawBodyEquivalent(req, MAXIMUM_MESSAGE_SIZE, encoding);
            body = JSON::parse(rawBody);
        }
    } catch (const exception& error) {
        res_param->writeHead(HTTPStatus::BadRequest);
        res_param->end(error.what());
        if (onerror.has_value()) { onerror.value()(Error(error.what())); }
        return;
    }

    try {
        optional<map<string, AuthInfo>> extra = nullopt;
        if (authInfo.has_value()) { extra = map<string, AuthInfo>{{"authInfo", authInfo.value()}}; }
        handleMessage(body, extra);
    } catch (const exception&) {
        res_param->writeHead(HTTPStatus::BadRequest);
        res_param->end("Invalid message: " + body.dump());
        return;
    }

    res_param->writeHead(202);
    res_param->end("Accepted");
}

void handleMessage(const JSON& message, const optional<map<string, AuthInfo>>& extra = nullopt) {
    MessageBase parsedMessage;
    try {
        parsedMessage = MessageBaseSchema::parse(message);
    } catch (const exception& error) {
        if (onerror.has_value()) { onerror.value()(Error(error.what())); }
        throw;
    }

    if (onmessage.has_value()) { onmessage.value()(parsedMessage, extra); }
}

void close() {
    if (_sseResponse.has_value()) { _sseResponse.value()->end(); }
    _sseResponse = nullopt;
    if (onclose.has_value()) { onclose.value()(); }
}

void send(const MessageBase& message) {
    if (!_sseResponse.has_value()) { throw runtime_error("Not connected"); }

    // Convert MessageBase to JSON

    JSON jsonMessage;
    jsonMessage[MSG_JSON_RPC] = message.jsonrpc;
    if (message.id.has_value()) { jsonMessage[MSG_ID] = message.id.value(); }
    if (message.method.has_value()) { jsonMessage[MSG_METHOD] = message.method.value(); }
    if (message.params.has_value()) { jsonMessage[MSG_PARAMS] = message.params.value(); }
    if (message.result.has_value()) { jsonMessage[MSG_RESULT] = message.result.value(); }
    if (message.error.has_value()) { jsonMessage[MSG_ERROR] = message.error.value(); }

    string eventData = "event: message\ndata: " + jsonMessage.dump() + "\n\n";
    _sseResponse.value()->write(eventData);
}

string sessionId() const {
    return _sessionId;
}

// ##########################################################
// ##########################################################
// ===================== CLIENT SECTION =====================
// ##########################################################
// ##########################################################

future<string> HTTP_Response::text() const {
    return async(launch::async, [this]() { return body; });
}

const char* SseError::what() const noexcept {
    return message_.c_str();
}

optional<int> SseError::GetCode() const {
    return code_;
}
const ErrorEvent& SseError::GetEvent() const {
    return event_;
}

future<void> SSEClientTransport::_authThenStart() {
    return async(launch::async, [this]() -> void {
        if (!_authProvider) { throw UnauthorizedError("No auth provider"); }

        AuthResult result;
        try {
            map<string, variant<URL, string>> authParams;
            authParams["serverUrl"] = _url;
            if (_resourceMetadataUrl.has_value()) {
                authParams["resourceMetadataUrl"] = _resourceMetadataUrl.value();
            }

            result = auth(_authProvider, authParams).get();
        } catch (const exception& error) {
            if (onerror) { onerror(error); }
            throw;
        }

        if (result != AuthResult::AUTHORIZED) { throw UnauthorizedError(); }

        return _startOrAuth().get();
    });
}

future<HeadersInit> SSEClientTransport::_commonHeaders() {
    return async(launch::async, [this]() -> HeadersInit {
        HeadersInit headers;
        if (_authProvider) {
            // TODO: Fix External Ref: Get tokens from auth provider
            // auto tokens = _authProvider->tokens().get();
            // if (tokens.has_value()) {
            //     headers["Authorization"] = "Bearer " + tokens.value().access_token;
            // }
        }
        return headers;
    });
}

future<void> SSEClientTransport::_startOrAuth() {
    return async(launch::async, [this]() -> void {
        // TODO: Fix External Ref: EventSource implementation
        // This needs to implement the exact Promise pattern from TypeScript:

        // _eventSource = new EventSource(
        //     _url.href,
        //     _eventSourceInit.value_or({
        //         fetch: [this](const URL& url, const RequestInit& init) {
        //             return _commonHeaders().then([=](const HeadersInit& headers) {
        //                 RequestInit newInit = init;
        //                 HeadersInit mergedHeaders = headers;
        //                 mergedHeaders[TSPT_ACCEPT] = TSPT_TEXT_EVENT_STREAM;
        //                 newInit[MSG_HEADERS] = mergedHeaders;
        //                 return fetch(url, newInit);
        //             });
        //         }
        //     })
        // );
        // _abortController = new AbortController();

        // _eventSource->onerror = [this](const ErrorEvent& event) {
        //     if (event.code == HTTPStatus::Unauthorized && _authProvider) {
        //         return _authThenStart();
        //     }
        //
        //     SseError error(event.code, event.message, event);
        //     if (onerror) {
        //         onerror(error);
        //     }
        //     throw error;
        // };

        // _eventSource->onopen = []() {
        //     // The connection is open, but we need to wait for the endpoint to be received.
        // };

        // _eventSource->addEventListener("endpoint", [this](const MessageEvent& event) {
        //     try {
        //         _endpoint = URL(event.data, _url);
        //         if (_endpoint.value().origin != _url.origin) {
        //             throw runtime_error(
        //                 "Endpoint origin does not match connection origin: " +
        //                 _endpoint.value().origin
        //             );
        //         }
        //     } catch (const exception& error) {
        //         if (onerror) {
        //             onerror(error);
        //         }
        //         close().get();
        //         throw;
        //     }
        // });

        // _eventSource->onmessage = [this](const MessageEvent& event) {
        //     MessageBase message;
        //     try {
        //         // TODO: Fix External Ref: MessageBaseSchema.parse(JSON.parse(event.data))
        //         // message = MessageBaseSchema::parse(JSON::parse(event.data));
        //     } catch (const exception& error) {
        //         if (onerror) {
        //             onerror(error);
        //         }
        //         return;
        //     }
        //
        //     if (onmessage) {
        //         onmessage(message);
        //     }
        // };

        // For now, simulate successful connection
        _endpoint = URL("/messages", _url);
    });
}

future<void> SSEClientTransport::start() {
    return async(launch::async, [this]() -> void {
        if (_eventSource != nullptr) {
            throw runtime_error("SSEClientTransport already started! If using Client class, note "
                                "that connect() calls start() automatically.");
        }

        return _startOrAuth().get();
    });
}

future<void> SSEClientTransport::finishAuth(const string& authorizationCode) {
    return async(launch::async, [this, authorizationCode]() -> void {
        if (!_authProvider) { throw UnauthorizedError("No auth provider"); }

        map<string, variant<URL, string>> authParams;
        authParams["serverUrl"] = _url;
        authParams["authorizationCode"] = authorizationCode;
        if (_resourceMetadataUrl.has_value()) {
            authParams["resourceMetadataUrl"] = _resourceMetadataUrl.value();
        }

        AuthResult result = auth(_authProvider, authParams).get();
        if (result != AuthResult::AUTHORIZED) { throw UnauthorizedError("Failed to authorize"); }
    });
}

future<void> SSEClientTransport::close() {
    return async(launch::async, [this]() -> void {
        // TODO: Fix External Ref: _abortController->abort();
        // TODO: Fix External Ref: _eventSource->close();
        _abortController = nullptr;
        _eventSource = nullptr;

        if (onclose) { onclose(); }
    });
}

future<void> SSEClientTransport::send(const MessageBase& message) {
    // Fix: Copy message to avoid reference issues
    return async(launch::async, [this, message]() -> void {
        if (!_endpoint.has_value()) { throw runtime_error("Not connected"); }

        try {
            HeadersInit commonHeaders = _commonHeaders().get();
            HeadersInit headers = commonHeaders;

            // Merge _requestInit headers if they exist
            if (_requestInit.has_value()) {
                for (const auto& [key, value] : _requestInit.value()) {
                    if (holds_alternative<string>(value)) {
                        headers[key] = get<string>(value);
                    }
                    // Handle HeadersInit case - merge maps
                    else if (holds_alternative<HeadersInit>(value)) {
                        HeadersInit headerMap = get<HeadersInit>(value);
                        for (const auto& [hKey, hValue] : headerMap) { headers[hKey] = hValue; }
                    }
                }
            }

            headers[TSPT_CONTENT_TYPE] = TSPT_APP_JSON;

            RequestInit init;
            if (_requestInit.has_value()) { init = _requestInit.value(); }
            init[MSG_METHOD] = MTHD_POST;
            init[MSG_HEADERS] = headers;
            // TODO: Fix External Ref: JSON.stringify(message)
            init[MSG_BODY] = string("{}"); // JSON::stringify(message);
            // TODO: Fix External Ref: signal: _abortController?.signal

            HTTP_Response response = fetch(_endpoint.value(), init).get();

            if (!response.ok) {
                if (response.status == HTTPStatus::Unauthorized && _authProvider) {
                    _resourceMetadataUrl = extractResourceMetadataUrl(response);

                    map<string, variant<URL, string>> authParams;
                    authParams["serverUrl"] = _url;
                    if (_resourceMetadataUrl.has_value()) {
                        authParams["resourceMetadataUrl"] = _resourceMetadataUrl.value();
                    }

                    AuthResult result = auth(_authProvider, authParams).get();
                    if (result != AuthResult::AUTHORIZED) { throw UnauthorizedError(); }

                    // Fix: Properly await recursive call
                    send(message).get();
                    return;
                }

                string text;
                try {
                    text = response.text().get();
                } catch (...) { text = MSG_NULL; }

                throw runtime_error("Error POSTing to endpoint (HTTP " + to_string(response.status)
                                    + "): " + text);
            }
        } catch (const exception& error) {
            if (onerror) { onerror(error); }
            throw;
        }
    });
}

// TODO: Implement these placeholder methods with actual functionality
future<AuthResult> SSEClientTransport::auth(shared_ptr<OAuthClientProvider> authProvider,
                                            const map<string, variant<URL, string>>& params) {
    return async(launch::async, []() -> AuthResult {
        // TODO: Fix External Ref: Actual auth implementation
        return AuthResult::AUTHORIZED;
    });
}

future<HTTP_Response> SSEClientTransport::fetch(const URL& url, const RequestInit& init) {
    return async(launch::async, [=]() -> HTTP_Response {
        // TODO: Fix External Ref: Actual HTTP implementation
        HTTP_Response response;
        response.status = HTTPStatus::Ok;
        response.ok = true;
        return response;
    });
}

optional<URL> SSEClientTransport::extractResourceMetadataUrl(const HTTP_Response& response) {
    // TODO: Fix External Ref: Extract from response headers
    return nullopt;
}

MCP_NAMESPACE_END
