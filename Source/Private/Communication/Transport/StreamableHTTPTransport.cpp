#include "Communication/Transport/StreamableHTTPTransport.h"

#include "Communication/Utilities/TransportUtilities.h"
#include "Core.h"
#include "Core/Constants/TransportConstants.h"

MCP_NAMESPACE_BEGIN

StreamableHTTPTransport::StreamableHTTPTransport(const std::string& url)
    : _url(url), _isRunning(false) {
    // Parse URL to get host and port
    size_t protocolEnd = url.find("://");
    if (protocolEnd == std::string::npos) { throw std::runtime_error("Invalid URL format"); }

    std::string host = url.substr(protocolEnd + 3);
    size_t pathStart = host.find('/');
    if (pathStart != std::string::npos) {
        _path = host.substr(pathStart);
        host = host.substr(0, pathStart);
    }

    size_t portStart = host.find(':');
    if (portStart != std::string::npos) {
        _port = std::stoi(host.substr(portStart + 1));
        host = host.substr(0, portStart);
    } else {
        _port = 80; // Default HTTP port
    }

    _client = std::make_unique<httplib::Client>(host, _port);
    _client->set_keep_alive(true);
}

StreamableHTTPTransport::~StreamableHTTPTransport() {
    Stop();
}

void StreamableHTTPTransport::Start() {
    if (_isRunning) { return; }

    _isRunning = true;
    if (_onStart) { _onStart(); }

    // Start reading thread
    _readThread = std::thread([this] { ReadLoop(); });
}

void StreamableHTTPTransport::Stop() {
    if (!_isRunning) { return; }

    _isRunning = false;
    if (_readThread.joinable()) { _readThread.join(); }

    if (_onStop) { _onStop(); }
}

void StreamableHTTPTransport::Send(const std::string& message,
                                   const TransportSendOptions& options) {
    if (!_isRunning) {
        if (_onError) { _onError(TRANSPORT_ERR_NOT_RUNNING); }
        return;
    }

    try {
        // Validate UTF-8 encoding
        if (!TransportUtilities::IsValidUTF8(message)) {
            if (_onError) { _onError(TRANSPORT_ERR_INVALID_UTF8); }
            return;
        }

        // Validate JSON-RPC message format
        if (!TransportUtilities::IsValidJSONRPC(message)) {
            if (_onError) { _onError(TRANSPORT_ERR_INVALID_JSON_RPC); }
            return;
        }

        // Handle resumption token if provided
        if (options.resumptionToken && options.onResumptionToken) {
            options.onResumptionToken(*options.resumptionToken);
        }

        // Add session ID to headers if available
        httplib::Headers headers;
        if (_sessionId) { headers.emplace(TSPT_SESSION_ID, *_sessionId); }

        // Send message via POST request
        auto res = _client->Post(_path, headers, message, TSPT_APP_JSON);
        if (!res || res->status != HTTPStatus::Ok) {
            if (_onError) {
                _onError(TRANSPORT_ERR_HTTP_REQUEST_FAILED
                         + (res ? std::to_string(res->status) : "Unknown error"));
            }
            return;
        }

        // Check for session ID in response headers
        auto sessionHeader = res->get_header_value(TSPT_SESSION_ID);
        if (!sessionHeader.empty() && !_sessionId) { _sessionId = sessionHeader; }
    } catch (const std::exception& e) {
        if (_onError) { _onError(std::string("Error sending message: ") + e.what()); }
    }
}

void StreamableHTTPTransport::SetOnMessage(MessageCallback callback) {
    _onMessage = std::move(callback);
}

void StreamableHTTPTransport::SetOnError(ErrorCallback callback) {
    _onError = std::move(callback);
}

void StreamableHTTPTransport::SetOnClose(CloseCallback callback) {
    _onClose = std::move(callback);
}

void StreamableHTTPTransport::SetOnStart(StartCallback callback) {
    _onStart = std::move(callback);
}

void StreamableHTTPTransport::SetOnStop(StopCallback callback) {
    _onStop = std::move(callback);
}

void StreamableHTTPTransport::WriteSSEEvent(const std::string& event, const std::string& data) {
    std::string sseMessage = "event: " + event + TSPT_EVENT_DELIMITER + TSPT_EVENT_DATA_PREFIX
                             + data + TSPT_EVENT_DELIMITER;
    Send(sseMessage);
}

// New method for resumability support
bool StreamableHTTPTransport::Resume(const std::string& resumptionToken) {
    // HTTP transport does not support resumption
    if (_onError) { _onError("Resumption not supported by StreamableHTTPTransport"); }
    return false;
}

std::optional<std::string> StreamableHTTPTransport::GetSessionId() const {
    return _sessionId;
}

void StreamableHTTPTransport::ReadLoop() {
    if (!_client) {
        if (_onError) { _onError("Failed to initialize HTTP client"); }
        return;
    }

    while (_isRunning) {
        try {
            // Add session ID to headers if available
            httplib::Headers headers;
            if (_sessionId) { headers.emplace(TSPT_SESSION_ID, *_sessionId); }

            auto res = _client->Get(_path, headers, [this](const char* data, size_t len) {
                std::string chunk(data, len);
                ParseSSEData(chunk);
                return true;
            });

            if (!res || res->status != HTTPStatus::Ok) {
                if (_onError) {
                    _onError(TRANSPORT_ERR_HTTP_REQUEST_FAILED
                             + (res ? std::to_string(res->status) : "Unknown error"));
                }
                break;
            }

            // Check for session ID in response headers
            auto sessionHeader = res->get_header_value(TSPT_SESSION_ID);
            if (!sessionHeader.empty() && !_sessionId) { _sessionId = sessionHeader; }
        } catch (const std::exception& e) {
            if (_onError) { _onError(std::string("Error in read loop: ") + e.what()); }
            break;
        }
    }

    if (_onClose) { _onClose(); }
}

void StreamableHTTPTransport::ParseSSEData(const std::string& data) {
    size_t pos = 0;
    std::string currentEvent;
    std::string currentData;

    while (pos < data.length()) {
        size_t lineEnd = data.find('\n', pos);
        if (lineEnd == std::string::npos) { break; }

        std::string line = data.substr(pos, lineEnd - pos);
        pos = lineEnd + 1;

        if (line.empty() || line[0] == ':') { continue; }

        if (line.substr(0, 6) == "event: ") {
            currentEvent = line.substr(6);
        } else if (line.substr(0, TSPT_EVENT_DATA_PREFIX_LEN) == TSPT_EVENT_DATA_PREFIX) {
            currentData = line.substr(TSPT_EVENT_DATA_PREFIX_LEN);
        } else if (line.empty() && !currentData.empty()) {
            // End of event
            if (_onMessage) { _onMessage(currentData, nullptr); }
            currentEvent.clear();
            currentData.clear();
        }
    }
}

MCP_NAMESPACE_END