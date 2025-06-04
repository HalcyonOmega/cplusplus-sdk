#include "StreamableHTTPTransport.h"

#include "Communication/Utilities/TransportUtilities.h"
#include "Constants.h"
#include "Core.h"

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
    _readThread = std::thread(&StreamableHTTPTransport::ReadLoop, this);
}

void StreamableHTTPTransport::Stop() {
    if (!_isRunning) { return; }

    _isRunning = false;
    if (_readThread.joinable()) { _readThread.join(); }

    if (_onStop) { _onStop(); }
}

void StreamableHTTPTransport::Send(const std::string& message,
                                   [[maybe_unused]] const TransportSendOptions& options) {
    if (!_isRunning) { throw std::runtime_error("Transport not started"); }

    if (!TransportUtilities::IsValidJSONRPC(message)) {
        throw std::runtime_error("Invalid JSON-RPC message");
    }

    auto res = _client->Post(_path, message, "application/json");
    if (!res) { throw std::runtime_error("Failed to send message"); }

    if (res->status != 200) {
        throw std::runtime_error("Server returned error: " + std::to_string(res->status));
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
    if (!_isRunning) { throw std::runtime_error("Transport not started"); }

    std::stringstream ss;
    ss << "event: " << event << "\n";
    ss << "data: " << data << "\n\n";

    auto res = _client->Post(_path, ss.str(), "text/event-stream");
    if (!res) { throw std::runtime_error("Failed to send SSE event"); }

    if (res->status != 200) {
        throw std::runtime_error("Server returned error: " + std::to_string(res->status));
    }
}

void StreamableHTTPTransport::ReadLoop() {
    while (_isRunning) {
        try {
            auto res = _client->Get(_path);
            if (!res) {
                if (_onError) { _onError("Failed to receive message"); }
                continue;
            }

            if (res->status != 200) {
                if (_onError) { _onError("Server returned error: " + std::to_string(res->status)); }
                continue;
            }

            if (TransportUtilities::IsValidJSONRPC(res->body)) {
                if (_onMessage) { _onMessage(res->body, nullptr); }
            } else {
                ParseSSEData(res->body);
            }
        } catch (const std::exception& e) {
            if (_onError) { _onError(e.what()); }
        }
    }

    if (_onClose) { _onClose(); }
}

void StreamableHTTPTransport::ParseSSEData(const std::string& data) {
    // TODO: Implement SSE parsing
    // For now, just log it
    TransportUtilities::Log("Received SSE data: " + data);
}

MCP_NAMESPACE_END