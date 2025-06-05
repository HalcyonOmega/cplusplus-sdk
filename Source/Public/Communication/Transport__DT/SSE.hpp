#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// TODO: Fix External Ref: HTTP Server Response functionality
// TODO: Fix External Ref: HTTP Request/Response handling
// TODO: Fix External Ref: Server-Sent Events implementation
// TODO: Fix External Ref: Raw body parsing
// TODO: Fix External Ref: Content-Type parsing

// Forward declarations for external dependencies
struct IncomingMessage {
    // TODO: Implement HTTP request structure
    map<string, string> headers;
    optional<AuthInfo> auth; // Optional auth info
};

struct ServerResponse {
    // TODO: Implement HTTP response structure
    bool is_ended = false;

    void writeHead(int status_code, const optional<map<string, string>>& headers = nullopt) {
        // TODO: Implement HTTP response header writing
    }

    void write(const string& data) {
        // TODO: Implement HTTP response writing
    }

    void end(const optional<string>& data = nullopt) {
        // TODO: Implement HTTP response ending
        is_ended = true;
    }

    void on(const string& event, optional<function<void()>> callback) {
        // TODO: Implement event handling
    }
};

// TODO: Fix External Ref: UUID generation
string generateRandomUUID() {
    // TODO: Implement proper UUID generation
    // For now, return a placeholder
    return "placeholder-uuid-" + to_string(rand());
}

// TODO: Fix External Ref: URL parsing and manipulation
struct URLHelper {
    static string addSessionParam(const string& endpoint, const string& session_id) {
        // TODO: Implement proper URL parameter handling like TypeScript URL class
        string separator = (endpoint.find('?') != string::npos) ? "&" : "?";
        return endpoint + separator + "sessionId=" + session_id;
    }
};

// TODO: Fix External Ref: Content-Type parsing
struct ContentTypeResult {
    string type;
    map<string, string> parameters;
};

ContentTypeResult parseContentType(const string& content_type_header) {
    // TODO: Implement content-type parsing equivalent to contentType.parse()
    ContentTypeResult result;
    result.type = "application/json";       // Placeholder
    result.parameters["charset"] = "utf-8"; // Placeholder
    return result;
}

// TODO: Fix External Ref: Raw body parsing
string getRawBodyEquivalent(IncomingMessage* req, const string& limit, const string& encoding) {
    // TODO: Implement raw body parsing with size limit
    throw runtime_error("Raw body parsing not implemented");
}

const string MAXIMUM_MESSAGE_SIZE = "4mb";

/**
 * Server transport for SSE: this will send messages over an SSE connection and receive messages
 * from HTTP POST requests.
 *
 * This transport is only available in Node.js environments.
 */
class SSEServerTransport {
  private:
    optional<ServerResponse*> _sseResponse;
    string _sessionId;
    string _endpoint;
    ServerResponse* res;

  public:
    optional<function<void()>> onclose;
    optional<function<void(const Error&)>> onerror;
    optional<function<void(const JSONRPCMessage&, const optional<map<string, AuthInfo>>& extra)>>
        onmessage;

    /**
     * Creates a new SSE server transport, which will direct the client to POST messages to the
     * relative or absolute URL identified by _endpoint.
     */
    SSEServerTransport(const string& endpoint, ServerResponse* res_param)
        : _endpoint(endpoint), res(res_param), _sseResponse(nullopt) {
        _sessionId = generateRandomUUID();
    }

    /**
     * Handles the initial SSE connection request.
     *
     * This should be called when a GET request is made to establish the SSE stream.
     */
    void start() {
        if (_sseResponse.has_value()) {
            throw runtime_error("SSEServerTransport already started! If using Server class, note "
                                "that connect() calls start() automatically.");
        }

        map<string, string> headers = {{"Content-Type", "text/event-stream"},
                                       {"Cache-Control", "no-cache, no-transform"},
                                       {"Connection", "keep-alive"}};
        res->writeHead(200, headers);

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
            if (onclose.has_value()) {
                onclose.value()();
            }
        });
    }

    /**
     * Handles incoming POST messages.
     *
     * This should be called when a POST request is made to send a message to the server.
     */
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
            string contentTypeHeader = "";
            auto ctIt = req->headers.find("content-type");
            if (ctIt != req->headers.end()) {
                contentTypeHeader = ctIt->second;
            }

            ContentTypeResult ct = parseContentType(contentTypeHeader);
            if (ct.type != "application/json") {
                throw runtime_error("Unsupported content-type: " + ct.type);
            }

            if (parsedBody.has_value()) {
                body = parsedBody.value();
            } else {
                string encoding = "utf-8";
                auto charsetIt = ct.parameters.find("charset");
                if (charsetIt != ct.parameters.end()) {
                    encoding = charsetIt->second;
                }

                string rawBody = getRawBodyEquivalent(req, MAXIMUM_MESSAGE_SIZE, encoding);
                body = JSON::parse(rawBody);
            }
        } catch (const exception& error) {
            res_param->writeHead(400);
            res_param->end(error.what());
            if (onerror.has_value()) {
                onerror.value()(Error(error.what()));
            }
            return;
        }

        try {
            optional<map<string, AuthInfo>> extra = nullopt;
            if (authInfo.has_value()) {
                extra = map<string, AuthInfo>{{"authInfo", authInfo.value()}};
            }
            handleMessage(body, extra);
        } catch (const exception&) {
            res_param->writeHead(400);
            res_param->end("Invalid message: " + body.dump());
            return;
        }

        res_param->writeHead(202);
        res_param->end("Accepted");
    }

    /**
     * Handle a client message, regardless of how it arrived. This can be used to inform the server
     * of messages that arrive via a means different than HTTP POST.
     */
    void handleMessage(const JSON& message,
                       const optional<map<string, AuthInfo>>& extra = nullopt) {
        JSONRPCMessage parsedMessage;
        try {
            parsedMessage = JSONRPCMessageSchema::parse(message);
        } catch (const exception& error) {
            if (onerror.has_value()) {
                onerror.value()(Error(error.what()));
            }
            throw;
        }

        if (onmessage.has_value()) {
            onmessage.value()(parsedMessage, extra);
        }
    }

    void close() {
        if (_sseResponse.has_value()) {
            _sseResponse.value()->end();
        }
        _sseResponse = nullopt;
        if (onclose.has_value()) {
            onclose.value()();
        }
    }

    void send(const JSONRPCMessage& message) {
        if (!_sseResponse.has_value()) {
            throw runtime_error("Not connected");
        }

        // Convert JSONRPCMessage to JSON
        JSON jsonMessage;
        jsonMessage[MSG_KEY_JSON_RPC] = message.jsonrpc;
        if (message.id.has_value()) {
            jsonMessage[MSG_KEY_ID] = message.id.value();
        }
        if (message.method.has_value()) {
            jsonMessage[MSG_KEY_METHOD] = message.method.value();
        }
        if (message.params.has_value()) {
            jsonMessage[MSG_KEY_PARAMS] = message.params.value();
        }
        if (message.result.has_value()) {
            jsonMessage[MSG_KEY_RESULT] = message.result.value();
        }
        if (message.error.has_value()) {
            jsonMessage[MSG_KEY_ERROR] = message.error.value();
        }

        string eventData = "event: message\ndata: " + jsonMessage.dump() + "\n\n";
        _sseResponse.value()->write(eventData);
    }

    /**
     * Returns the session ID for this transport.
     *
     * This can be used to route incoming POST requests.
     */
    string sessionId() const {
        return _sessionId;
    }
};

// ##########################################################
// ##########################################################
// ===================== CLIENT SECTION =====================
// ##########################################################
// ##########################################################

// HTTP Response struct (moved up for forward declaration)
struct HttpResponse {
    int status;
    string body;
    map<string, string> headers;
    bool ok;

    future<string> text() const {
        return async(launch::async, [this]() { return body; });
    }
};

// URL class placeholder
// TODO: Fix External Ref: Implement proper URL class
class URL {
  public:
    string href;
    string origin;

    URL(const string& url_string) : href(url_string) {
        // TODO: Proper URL parsing
        origin = url_string; // Simplified
    }

    URL(const string& relative, const URL& base) {
        // TODO: Proper relative URL resolution
        href = base.href + "/" + relative;
        origin = base.origin;
    }
};

// HeadersInit type equivalent
using HeadersInit = map<string, string>;

// EventSourceInit equivalent
using EventSourceInit = map<string, string>;

// RequestInit equivalent - using variant to handle different value types
using RequestInit = map<string, variant<string, HeadersInit, bool>>;

// ErrorEvent equivalent
// TODO: Fix External Ref: Implement proper ErrorEvent
struct ErrorEvent {
    optional<int> code;
    string message;
};

class SseError : public exception {
  public:
    SseError(optional<int> code, const string& message, const ErrorEvent& event)
        : code_(code), event_(event) {
        message_ = "SSE error: " + message;
    }

    const char* what() const noexcept override {
        return message_.c_str();
    }

    optional<int> GetCode() const {
        return code_;
    }
    const ErrorEvent& GetEvent() const {
        return event_;
    }

  private:
    optional<int> code_;
    string message_;
    ErrorEvent event_;
};

/**
 * Configuration options for the SSEClientTransport.
 */
struct SSEClientTransportOptions {
    /**
     * An OAuth client provider to use for authentication.
     *
     * When an authProvider is specified and the SSE connection is started:
     * 1. The connection is attempted with any existing access token from the authProvider.
     * 2. If the access token has expired, the authProvider is used to refresh the token.
     * 3. If token refresh fails or no access token exists, and auth is required,
     * OAuthClientProvider.redirectToAuthorization is called, and an UnauthorizedError will be
     * thrown from connect/start.
     *
     * After the user has finished authorizing via their user agent, and is redirected back to the
     * MCP client application, call SSEClientTransport.finishAuth with the authorization code before
     * retrying the connection.
     *
     * If an authProvider is not provided, and auth is required, an UnauthorizedError will be
     * thrown.
     *
     * UnauthorizedError might also be thrown when sending any message over the SSE transport,
     * indicating that the session has expired, and needs to be re-authed and reconnected.
     */
    shared_ptr<OAuthClientProvider> authProvider = nullptr;

    /**
     * Customizes the initial SSE request to the server (the request that begins the stream).
     *
     * NOTE: Setting this property will prevent an Authorization header from
     * being automatically attached to the SSE request, if an authProvider is
     * also given. This can be worked around by setting the Authorization header
     * manually.
     */
    optional<EventSourceInit> eventSourceInit = nullopt;

    /**
     * Customizes recurring POST requests to the server.
     */
    optional<RequestInit> requestInit = nullopt;
};

/**
 * Client transport for SSE: this will connect to a server using Server-Sent Events for receiving
 * messages and make separate POST requests for sending messages.
 */
class SSEClientTransport : public Transport {
  private:
    // TODO: Fix External Ref: EventSource equivalent
    void* _eventSource; // Placeholder for EventSource
    optional<URL> _endpoint;
    // TODO: Fix External Ref: AbortController equivalent
    void* _abortController; // Placeholder for AbortController
    URL _url;
    optional<URL> _resourceMetadataUrl;
    optional<EventSourceInit> _eventSourceInit;
    optional<RequestInit> _requestInit;
    shared_ptr<OAuthClientProvider> _authProvider;

  public:
    // Callback function types (exactly matching TypeScript)
    function<void()> onclose;
    function<void(const exception& error)> onerror;
    function<void(const JSONRPCMessage& message)> onmessage;

    SSEClientTransport(const URL& url, const optional<SSEClientTransportOptions>& opts = nullopt);

    // Public interface methods (async equivalent using futures for Promise pattern)
    future<void> start();
    future<void> finishAuth(const string& authorizationCode);
    future<void> close();
    future<void> send(const JSONRPCMessage& message);

  private:
    // Private methods (exactly matching TypeScript structure)
    future<void> _authThenStart();
    future<HeadersInit> _commonHeaders();
    future<void> _startOrAuth();

    // TODO: Fix External Ref: Auth function
    future<AuthResult> auth(shared_ptr<OAuthClientProvider> authProvider,
                            const map<string, variant<URL, string>>& params);

    // TODO: Fix External Ref: extractResourceMetadataUrl function
    optional<URL> extractResourceMetadataUrl(const HttpResponse& response);

    // TODO: Fix External Ref: HTTP functionality
    future<HttpResponse> fetch(const URL& url, const RequestInit& init);
};

// Implementation

SSEClientTransport::SSEClientTransport(const URL& url,
                                       const optional<SSEClientTransportOptions>& opts)
    : _eventSource(nullptr), _endpoint(nullopt), _abortController(nullptr), _url(url),
      _resourceMetadataUrl(nullopt),
      _eventSourceInit(opts.has_value() ? opts.value().eventSourceInit : nullopt),
      _requestInit(opts.has_value() ? opts.value().requestInit : nullopt),
      _authProvider(opts.has_value() ? opts.value().authProvider : nullptr) {}

future<void> SSEClientTransport::_authThenStart() {
    return async(launch::async, [this]() -> void {
        if (!_authProvider) {
            throw UnauthorizedError("No auth provider");
        }

        AuthResult result;
        try {
            map<string, variant<URL, string>> authParams;
            authParams["serverUrl"] = _url;
            if (_resourceMetadataUrl.has_value()) {
                authParams["resourceMetadataUrl"] = _resourceMetadataUrl.value();
            }

            result = auth(_authProvider, authParams).get();
        } catch (const exception& error) {
            if (onerror) {
                onerror(error);
            }
            throw;
        }

        if (result != AuthResult::AUTHORIZED) {
            throw UnauthorizedError();
        }

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
        //                 mergedHeaders["Accept"] = "text/event-stream";
        //                 newInit["headers"] = mergedHeaders;
        //                 return fetch(url, newInit);
        //             });
        //         }
        //     })
        // );
        // _abortController = new AbortController();

        // _eventSource->onerror = [this](const ErrorEvent& event) {
        //     if (event.code == 401 && _authProvider) {
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
        //     JSONRPCMessage message;
        //     try {
        //         // TODO: Fix External Ref: JSONRPCMessageSchema.parse(JSON.parse(event.data))
        //         // message = JSONRPCMessageSchema::parse(JSON::parse(event.data));
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
        if (!_authProvider) {
            throw UnauthorizedError("No auth provider");
        }

        map<string, variant<URL, string>> authParams;
        authParams["serverUrl"] = _url;
        authParams["authorizationCode"] = authorizationCode;
        if (_resourceMetadataUrl.has_value()) {
            authParams["resourceMetadataUrl"] = _resourceMetadataUrl.value();
        }

        AuthResult result = auth(_authProvider, authParams).get();
        if (result != AuthResult::AUTHORIZED) {
            throw UnauthorizedError("Failed to authorize");
        }
    });
}

future<void> SSEClientTransport::close() {
    return async(launch::async, [this]() -> void {
        // TODO: Fix External Ref: _abortController->abort();
        // TODO: Fix External Ref: _eventSource->close();
        _abortController = nullptr;
        _eventSource = nullptr;

        if (onclose) {
            onclose();
        }
    });
}

future<void> SSEClientTransport::send(const JSONRPCMessage& message) {
    // Fix: Copy message to avoid reference issues
    return async(launch::async, [this, message]() -> void {
        if (!_endpoint.has_value()) {
            throw runtime_error("Not connected");
        }

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
                        for (const auto& [hKey, hValue] : headerMap) {
                            headers[hKey] = hValue;
                        }
                    }
                }
            }

            headers["content-type"] = "application/json";

            RequestInit init;
            if (_requestInit.has_value()) {
                init = _requestInit.value();
            }
            init[MSG_KEY_METHOD] = string("POST");
            init["headers"] = headers;
            // TODO: Fix External Ref: JSON.stringify(message)
            init["body"] = string("{}"); // JSON::stringify(message);
            // TODO: Fix External Ref: signal: _abortController?.signal

            HttpResponse response = fetch(_endpoint.value(), init).get();

            if (!response.ok) {
                if (response.status == 401 && _authProvider) {
                    _resourceMetadataUrl = extractResourceMetadataUrl(response);

                    map<string, variant<URL, string>> authParams;
                    authParams["serverUrl"] = _url;
                    if (_resourceMetadataUrl.has_value()) {
                        authParams["resourceMetadataUrl"] = _resourceMetadataUrl.value();
                    }

                    AuthResult result = auth(_authProvider, authParams).get();
                    if (result != AuthResult::AUTHORIZED) {
                        throw UnauthorizedError();
                    }

                    // Fix: Properly await recursive call
                    send(message).get();
                    return;
                }

                string text;
                try {
                    text = response.text().get();
                } catch (...) {
                    text = "";
                }

                throw runtime_error("Error POSTing to endpoint (HTTP " + to_string(response.status)
                                    + "): " + text);
            }
        } catch (const exception& error) {
            if (onerror) {
                onerror(error);
            }
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

future<HttpResponse> SSEClientTransport::fetch(const URL& url, const RequestInit& init) {
    return async(launch::async, [=]() -> HttpResponse {
        // TODO: Fix External Ref: Actual HTTP implementation
        HttpResponse response;
        response.status = 200;
        response.ok = true;
        return response;
    });
}

optional<URL> SSEClientTransport::extractResourceMetadataUrl(const HttpResponse& response) {
    // TODO: Fix External Ref: Extract from response headers
    return nullopt;
}

MCP_NAMESPACE_END
