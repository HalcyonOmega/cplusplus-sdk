#include "StdioServerTransport.h"

#include <iostream>
#include <sstream>

#include "Constants.h"
#include "Core.h"
#include "Utilities/TransportUtilities.h"

MCP_NAMESPACE_BEGIN

StdioServerTransport::StdioServerTransport() : _isRunning(false) {}

StdioServerTransport::~StdioServerTransport() {
    Stop();
}

void StdioServerTransport::Start() {
    if (_isRunning) { return; }

    _isRunning = true;
    if (_onStart) { _onStart(); }

    // Start reading thread
    _readThread = std::thread([this] { ReadLoop(); });
}

void StdioServerTransport::Stop() {
    if (!_isRunning) { return; }

    _isRunning = false;
    if (_readThread.joinable()) { _readThread.join(); }

    if (_onStop) { _onStop(); }
}

void StdioServerTransport::Send(const std::string& message,
                                [[maybe_unused]] const TransportSendOptions& options) {
    if (!_isRunning) { throw std::runtime_error("Transport not started"); }

    if (!TransportUtilities::IsValidJSONRPC(message)) {
        throw std::runtime_error("Invalid JSON-RPC message");
    }

    std::cout << message << std::endl;
}

void StdioServerTransport::SetOnMessage(MessageCallback callback) {
    _onMessage = std::move(callback);
}

void StdioServerTransport::SetOnError(ErrorCallback callback) {
    _onError = std::move(callback);
}

void StdioServerTransport::SetOnClose(CloseCallback callback) {
    _onClose = std::move(callback);
}

void StdioServerTransport::SetOnStart(StartCallback callback) {
    _onStart = std::move(callback);
}

void StdioServerTransport::SetOnStop(StopCallback callback) {
    _onStop = std::move(callback);
}

void StdioServerTransport::WriteSSEEvent(const std::string& event, const std::string& data) {
    if (!_isRunning) { throw std::runtime_error("Transport not started"); }

    std::stringstream ss;
    ss << "event: " << event << "\n";
    ss << "data: " << data << "\n\n";
    std::cout << ss.str() << std::flush;
}

std::optional<std::string> StdioServerTransport::GetSessionId() const {
    return _sessionId;
}

void StdioServerTransport::ReadLoop() {
    std::string line;
    while (_isRunning && std::getline(std::cin, line)) {
        try {
            if (TransportUtilities::IsValidJSONRPC(line)) {
                if (_onMessage) { _onMessage(line, nullptr); }
            } else {
                ParseSSEData(line);
            }
        } catch (const std::exception& e) {
            if (_onError) { _onError(e.what()); }
        }
    }

    if (_onClose) { _onClose(); }
}

void StdioServerTransport::ParseSSEData(const std::string& data) {
    // TODO: Implement SSE parsing
    // For now, just log it
    TransportUtilities::Log("Received SSE data: " + data);
}

MCP_NAMESPACE_END