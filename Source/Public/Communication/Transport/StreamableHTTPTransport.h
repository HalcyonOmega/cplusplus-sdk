#pragma once

#include "Core.h"
#include "Transport.h"
#include "httplib.h"

MCP_NAMESPACE_BEGIN

class StreamableHTTPTransport : public Transport {
  public:
    explicit StreamableHTTPTransport(const std::string& url);
    ~StreamableHTTPTransport() override;

    // Transport interface implementation
    void Start() override;
    void Stop() override;
    void Send(const std::string& message, const TransportSendOptions& options = {}) override;
    void SetOnMessage(MessageCallback callback) override;
    void SetOnError(ErrorCallback callback) override;
    void SetOnClose(CloseCallback callback) override;
    void SetOnStart(StartCallback callback) override;
    void SetOnStop(StopCallback callback) override;
    void WriteSSEEvent(const std::string& event, const std::string& data) override;

    // New method for resumability support
    bool Resume(const std::string& resumptionToken) override;

    std::optional<std::string> GetSessionId() const;

  private:
    void ReadLoop();
    void ParseSSEData(const std::string& data);

    std::string _url;
    std::string _path;
    int _port;
    std::unique_ptr<httplib::Client> _client;
    std::atomic<bool> _isRunning;
    std::thread _readThread;
    std::optional<std::string> _sessionId;

    MessageCallback _onMessage;
    ErrorCallback _onError;
    CloseCallback _onClose;
    StartCallback _onStart;
    StopCallback _onStop;
};

MCP_NAMESPACE_END