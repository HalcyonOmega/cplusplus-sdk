#include "Communication/Transport/StdioTransport.h"

#include "Communication/Utilities/TransportUtilities.h"
#include "Core.h"
#include "Core/Constants/TransportConstants.h"

MCP_NAMESPACE_BEGIN

StdioTransport::StdioTransport() : _isRunning(false) {}

StdioTransport::~StdioTransport() {
    Stop();
}

void StdioTransport::Start() {
    if (_isRunning) { return; }

    _isRunning = true;
    if (_onStart) { _onStart(); }

    // Start reading thread
    _readThread = std::thread([this] { ReadLoop(); });
}

void StdioTransport::Stop() {
    if (!_isRunning) { return; }

    _isRunning = false;
    if (_readThread.joinable()) { _readThread.join(); }

    if (_onStop) { _onStop(); }
}

void StdioTransport::Send(const std::string& message, const TransportSendOptions& options) {
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

        std::cout << message << std::endl;
        if (std::cout.fail()) {
            if (_onError) { _onError("Failed to write to stdout"); }
        }
    } catch (const std::exception& e) {
        if (_onError) { _onError(std::string("Error sending message: ") + e.what()); }
    }
}

void StdioTransport::SetOnMessage(MessageCallback callback) {
    _onMessage = std::move(callback);
}

void StdioTransport::SetOnError(ErrorCallback callback) {
    _onError = std::move(callback);
}

void StdioTransport::SetOnClose(CloseCallback callback) {
    _onClose = std::move(callback);
}

void StdioTransport::SetOnStart(StartCallback callback) {
    _onStart = std::move(callback);
}

void StdioTransport::SetOnStop(StopCallback callback) {
    _onStop = std::move(callback);
}

void StdioTransport::WriteSSEEvent(const std::string& event, const std::string& data) {
    std::string sseMessage = "event: " + event + TSPT_EVENT_DELIMITER + TSPT_EVENT_DATA_PREFIX
                             + data + TSPT_EVENT_DELIMITER;
    Send(sseMessage);
}

// New method for resumability support
bool StdioTransport::Resume(const std::string& resumptionToken) {
    // Stdio transport does not support resumption
    if (_onError) { _onError("Resumption not supported by StdioTransport"); }
    return false;
}

std::optional<std::string> StdioTransport::GetSessionID() const {
    return _sessionId;
}

void StdioTransport::ReadLoop() {
    std::string line;
    while (_isRunning && std::getline(std::cin, line)) {
        try {
            // Validate UTF-8 encoding
            if (!TransportUtilities::IsValidUTF8(line)) {
                if (_onError) { _onError(TRANSPORT_ERR_INVALID_UTF8); }
                continue;
            }

            // Validate JSON-RPC message format
            if (!TransportUtilities::IsValidJSONRPC(line)) {
                if (_onError) { _onError(TRANSPORT_ERR_INVALID_JSON_RPC); }
                continue;
            }

            if (_onMessage) { _onMessage(line, nullptr); }
        } catch (const std::exception& e) {
            if (_onError) { _onError(std::string("Error processing message: ") + e.what()); }
        }
    }

    // Handle stdin close
    if (_onClose) { _onClose(); }
}

void StdioTransport::ParseSSEData(const std::string& data) {
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