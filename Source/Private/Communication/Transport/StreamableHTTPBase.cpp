#include "Communication/Transport/StreamableHTTPTransport.h"
#include "Communication/Utilities/TransportUtilities.h"
#include "Core.h"
#include "Core/Constants/TransportConstants.h"

MCP_NAMESPACE_BEGIN

StreamableHTTPTransportBase::StreamableHTTPTransportBase(const string& InURL)
    : m_URL(InURL), m_IsRunning(false) {
    // Parse URL to get host and port
    size_t protocolEnd = m_URL.find("://");
    if (protocolEnd == string::npos) { throw runtime_error("Invalid URL format"); }

    string host = url.substr(protocolEnd + 3);
    size_t pathStart = host.find('/');
    if (pathStart != string::npos) {
        _path = host.substr(pathStart);
        host = host.substr(0, pathStart);
    }

    size_t portStart = host.find(':');
    if (portStart != string::npos) {
        _port = stoi(host.substr(portStart + 1));
        host = host.substr(0, portStart);
    } else {
        _port = 80; // Default HTTP port
    }

    _client = make_unique<httplib::Client>(host, _port);
    _client->set_keep_alive(true);
}

StreamableHTTPTransportBase::~StreamableHTTPTransportBase() {
    Close();
}

future<void> StreamableHTTPTransportBase::Start() {
    if (_isRunning) { return; }

    _isRunning = true;
    if (_onStart) { _onStart(); }

    // Start reading thread
    _readThread = thread([this] { ReadLoop(); });
}

future<void> StreamableHTTPTransportBase::Close() {
    if (!_isRunning) { return; }

    _isRunning = false;
    if (_readThread.joinable()) { _readThread.join(); }

    if (_onStop) { _onStop(); }
}

future<void> StreamableHTTPTransportBase::Send(const MessageBase& InMessage,
                                               const TransportSendOptions& InOptions) {
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
                         + (res ? to_string(res->status) : "Unknown error"));
            }
            return;
        }

        // Check for session ID in response headers
        auto sessionHeader = res->get_header_value(TSPT_SESSION_ID);
        if (!sessionHeader.empty() && !_sessionId) { _sessionId = sessionHeader; }
    } catch (const exception& e) {
        if (_onError) { _onError(string("Error sending message: ") + e.what()); }
    }
}

void StreamableHTTPTransportBase::WriteSSEEvent(const string& InEvent, const string& InData) {
    string sseMessage = "event: " + InEvent + TSPT_EVENT_DELIMITER + TSPT_EVENT_DATA_PREFIX + InData
                        + TSPT_EVENT_DELIMITER;
    Send(sseMessage);
}

bool StreamableHTTPTransportBase::Resume(const string& InResumptionToken) {
    // HTTP transport does not support resumption
    if (_onError) { _onError("Resumption not supported by StreamableHTTPTransport"); }
    return false;
}

void StreamableHTTPTransportBase::ReadLoop() {
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
                string chunk(data, len);
                ParseSSEData(chunk);
                return true;
            });

            if (!res || res->status != HTTPStatus::Ok) {
                if (_onError) {
                    _onError(TRANSPORT_ERR_HTTP_REQUEST_FAILED
                             + (res ? to_string(res->status) : "Unknown error"));
                }
                break;
            }

            // Check for session ID in response headers
            auto sessionHeader = res->get_header_value(TSPT_SESSION_ID);
            if (!sessionHeader.empty() && !_sessionId) { _sessionId = sessionHeader; }
        } catch (const exception& e) {
            if (_onError) { _onError(string("Error in read loop: ") + e.what()); }
            break;
        }
    }

    if (_onClose) { _onClose(); }
}

void StreamableHTTPTransportBase::ParseSSEData(const string& data) {
    size_t pos = 0;
    string currentEvent;
    string currentData;

    while (pos < data.length()) {
        size_t lineEnd = data.find('\n', pos);
        if (lineEnd == string::npos) { break; }

        string line = data.substr(pos, lineEnd - pos);
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