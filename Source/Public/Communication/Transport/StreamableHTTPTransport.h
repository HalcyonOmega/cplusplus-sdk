#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "../../../ThirdParty/httplib.h"
#include "Transport.h"

MCP_NAMESPACE_BEGIN

class StreamableHTTPTransport : public Transport {
  public:
    StreamableHTTPTransport(const std::string& url) : _url(url), _isRunning(false) {
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

    ~StreamableHTTPTransport() override {
        Stop();
    }

    // Transport interface implementation
    void Start() override {
        if (_isRunning) { return; }

        _isRunning = true;
        if (_onStart) { _onStart(); }

        // Start reading thread
        _readThread = std::thread([this] { ReadLoop(); });
    }

    void Stop() override {
        if (!_isRunning) { return; }

        _isRunning = false;
        if (_readThread.joinable()) { _readThread.join(); }

        if (_onStop) { _onStop(); }
    }

    void Send(const std::string& message, const TransportSendOptions& options = {}) override {
        if (!_isRunning) {
            if (_onError) { _onError("Transport is not running"); }
            return;
        }

        std::lock_guard<std::mutex> lock(_mutex);
        _messageQueue.push(message);
        _condition.notify_one();
    }

    void SetOnMessage(MessageCallback callback) override {
        _onMessage = std::move(callback);
    }

    void SetOnError(ErrorCallback callback) override {
        _onError = std::move(callback);
    }

    void SetOnClose(CloseCallback callback) override {
        _onClose = std::move(callback);
    }

    void SetOnStart(StartCallback callback) override {
        _onStart = std::move(callback);
    }

    void SetOnStop(StopCallback callback) override {
        _onStop = std::move(callback);
    }

    void WriteSSEEvent(const std::string& event, const std::string& data) override {
        std::string sseMessage = "event: " + event + "\ndata: " + data + "\n\n";
        Send(sseMessage);
    }

  private:
    void ReadLoop() {
        if (!_client) {
            if (_onError) { _onError("Failed to initialize HTTP client"); }
            return;
        }

        while (_isRunning) {
            auto res = _client->Get(_path, [this](const char* data, size_t len) {
                std::string chunk(data, len);
                ParseSSEData(chunk);
                return true;
            });

            if (!res || res->status != 200) {
                if (_onError) {
                    _onError("HTTP request failed: "
                             + (res ? std::to_string(res->status) : "Unknown error"));
                }
                break;
            }
        }
    }

    void ParseSSEData(const std::string& data) {
        size_t pos = 0;
        while (pos < data.length()) {
            size_t lineEnd = data.find('\n', pos);
            if (lineEnd == std::string::npos) { break; }

            std::string line = data.substr(pos, lineEnd - pos);
            pos = lineEnd + 1;

            if (line.empty() || line[0] == ':') { continue; }

            if (line.substr(0, 6) == "data: ") {
                std::string eventData = line.substr(6);
                if (_onMessage) { _onMessage(eventData, nullptr); }
            }
        }
    }

    std::string _url;
    std::string _path;
    int _port;
    std::unique_ptr<httplib::Client> _client;
    std::atomic<bool> _isRunning;
    std::thread _readThread;
    std::queue<std::string> _messageQueue;
    std::mutex _mutex;
    std::condition_variable _condition;

    MessageCallback _onMessage;
    ErrorCallback _onError;
    CloseCallback _onClose;
    StartCallback _onStart;
    StopCallback _onStop;
};

MCP_NAMESPACE_END