#pragma once

#include <functional>
#include <future>
#include <iostream>
#include <mutex>

#include "MCP_ReadBuffer.h"
#include "MCP_Transport.h"

MCP_NAMESPACE_BEGIN

/**
 * Server transport for stdio: this communicates with a MCP client by reading from stdin
 * and writing to stdout.
 */
class MCP_StdioServerTransport : public MCP_Transport {
  public:
    MCP_StdioServerTransport(std::istream& stdin = std::cin, std::ostream& stdout = std::cout)
        : _stdin(stdin), _stdout(stdout) {}

    ~MCP_StdioServerTransport() override {
        if (_started) { Close(); }
    }

    // Transport interface implementation
    std::future<void> Start() override {
        std::promise<void> promise;
        auto future = promise.get_future();

        if (_started) {
            promise.set_exception(std::make_exception_ptr(
                std::runtime_error("StdioServerTransport already started! If using Server class, "
                                   "note that connect() calls start() automatically.")));
            return future;
        }

        _started = true;
        promise.set_value();
        return future;
    }

    std::future<void> Send(const MCP_MessageBase& message,
                           const MCP_TransportSendOptions& options = {}) override {
        std::promise<void> promise;
        auto future = promise.get_future();

        try {
            // Handle resumption token if provided
            if (options.resumptionToken && options.onResumptionToken) {
                options.onResumptionToken(*options.resumptionToken);
            }

            std::string json = SerializeMessage(message);
            _stdout << json << std::flush;
            promise.set_value();
        } catch (const std::exception& e) { promise.set_exception(std::current_exception()); }

        return future;
    }

    std::future<void> Close() override {
        std::promise<void> promise;
        auto future = promise.get_future();

        if (_started) {
            _started = false;
            _readBuffer.Clear();
            if (_closeCallback) { _closeCallback(); }
        }

        promise.set_value();
        return future;
    }

    // Callback setters
    void SetCloseCallback(CloseCallback callback) override {
        _closeCallback = std::move(callback);
    }

    void SetErrorCallback(ErrorCallback callback) override {
        _errorCallback = std::move(callback);
    }

    void SetMessageCallback(MessageCallback callback) override {
        _messageCallback = std::move(callback);
    }

    // Process any pending input
    void ProcessInput() {
        if (!_started || !_messageCallback) return;

        std::vector<uint8_t> buffer(1024);
        while (_stdin.good()) {
            _stdin.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
            std::streamsize count = _stdin.gcount();
            if (count > 0) {
                buffer.resize(count);
                _readBuffer.Append(buffer);
                buffer.resize(1024);

                while (auto message = _readBuffer.ReadMessage()) {
                    try {
                        _messageCallback(*message, nullptr);
                    } catch (const std::exception& e) {
                        if (_errorCallback) { _errorCallback(e.what()); }
                    }
                }
            }
        }
    }

  private:
    std::istream& _stdin;
    std::ostream& _stdout;
    MCP_ReadBuffer _readBuffer;
    bool _started = false;

    // Callbacks
    CloseCallback _closeCallback;
    ErrorCallback _errorCallback;
    MessageCallback _messageCallback;
};

MCP_NAMESPACE_END