#include "Communication/Transport/InMemoryTransport.h"

#include <random>
#include <sstream>

#include "Constants.h"
#include "Core.h"

namespace MCP {

InMemoryTransport::InMemoryTransport() {
    // Generate a unique session ID
    std::random_device rd;
    std::stringstream ss;
    ss << std::hex << rd();
    _sessionId = ss.str();
}

InMemoryTransport::~InMemoryTransport() {
    Stop();
}

void InMemoryTransport::Start() {
    // Process any messages that were queued before start was called
    std::lock_guard<std::mutex> lock(_queueMutex);
    while (!_messageQueue.empty()) {
        const auto& queuedMessage = _messageQueue.front();
        if (_onMessage) {
            _onMessage(queuedMessage.message,
                       queuedMessage.authInfo ? &*queuedMessage.authInfo : nullptr);
        }
        _messageQueue.pop();
    }
    if (_onStart) { _onStart(); }
}

void InMemoryTransport::Stop() {
    if (auto other = _otherTransport.lock()) { other->_otherTransport.reset(); }
    _otherTransport.reset();
    if (_onStop) { _onStop(); }
    if (_onClose) { _onClose(); }
}

void InMemoryTransport::Send(const std::string& message, const TransportSendOptions& options) {
    if (auto other = _otherTransport.lock()) {
        if (other->_onMessage) {
            other->_onMessage(message, nullptr);
        } else {
            std::lock_guard<std::mutex> lock(other->_queueMutex);
            other->_messageQueue.push({message, std::nullopt});
        }

        // Handle resumption token if provided
        if (options.onResumptionToken && options.resumptionToken) {
            options.onResumptionToken(*options.resumptionToken);
        }
    } else {
        if (_onError) { _onError("Not connected"); }
    }
}

void InMemoryTransport::SetOnMessage(MessageCallback callback) {
    _onMessage = std::move(callback);
}

void InMemoryTransport::SetOnError(ErrorCallback callback) {
    _onError = std::move(callback);
}

void InMemoryTransport::SetOnClose(CloseCallback callback) {
    _onClose = std::move(callback);
}

void InMemoryTransport::SetOnStart(StartCallback callback) {
    _onStart = std::move(callback);
}

void InMemoryTransport::SetOnStop(StopCallback callback) {
    _onStop = std::move(callback);
}

void InMemoryTransport::WriteSSEEvent(const std::string& event, const std::string& data) {
    std::string sseMessage = "event: " + event + "\ndata: " + data + "\n\n";
    Send(sseMessage);
}

std::pair<std::shared_ptr<InMemoryTransport>, std::shared_ptr<InMemoryTransport>>
InMemoryTransport::CreateLinkedPair() {
    auto clientTransport = std::make_shared<InMemoryTransport>();
    auto serverTransport = std::make_shared<InMemoryTransport>();
    clientTransport->_otherTransport = serverTransport;
    serverTransport->_otherTransport = clientTransport;
    return {clientTransport, serverTransport};
}

} // namespace MCP