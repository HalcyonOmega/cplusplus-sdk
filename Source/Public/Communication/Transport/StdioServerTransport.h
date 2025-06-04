#pragma once

#include <atomic>
#include <iostream>
#include <thread>

#include "Transport.h"

MCP_NAMESPACE_BEGIN

/**
 * Server transport for stdio: this communicates with a MCP client by reading from stdin
 * and writing to stdout.
 */
class StdioServerTransport : public Transport {
  public:
    StdioServerTransport() : _isRunning(false) {}
    ~StdioServerTransport() override {
        Stop();
    }

    // Transport interface implementation
    void Start() override {
        if (_isRunning) { return; }

        _isRunning = true;
        if (_onStart) { _onStart(); }

        // Start reading thread
        _readThread = std::thread([this] {
            std::string line;
            while (_isRunning && std::getline(std::cin, line)) {
                if (_onMessage) { _onMessage(line, nullptr); }
            }
        });
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

        std::cout << message << std::endl;
        if (std::cout.fail()) {
            if (_onError) { _onError("Failed to write to stdout"); }
        }
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
    std::atomic<bool> _isRunning;
    std::thread _readThread;

    MessageCallback _onMessage;
    ErrorCallback _onError;
    CloseCallback _onClose;
    StartCallback _onStart;
    StopCallback _onStop;
};

MCP_NAMESPACE_END