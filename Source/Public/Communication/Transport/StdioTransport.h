#pragma once

#include "Core.h"
#include "Transport.h"

MCP_NAMESPACE_BEGIN

/**
 * Transport for stdio: this communicates by reading from stdin and writing to stdout.
 */
class StdioTransport : public Transport {
  public:
    StdioTransport();
    ~StdioTransport() override;

    // Transport interface implementation
    void Start() override;
    void Stop() override;
    void Send(const std::string& InMessage, const TransportSendOptions& InOptions = {}) override;
    void SetOnMessage(MessageCallback InCallback) override;
    void SetOnError(ErrorCallback InCallback) override;
    void SetOnClose(CloseCallback InCallback) override;
    void SetOnStart(StartCallback InCallback) override;
    void SetOnStop(StopCallback InCallback) override;
    void WriteSSEEvent(const std::string& InEvent, const std::string& InData) override;

    // New method for resumability support
    bool Resume(const std::string& InResumptionToken) override;

    std::optional<std::string> GetSessionID() const;

  private:
    void ReadLoop();
    void ParseSSEData(const std::string& InData);

    std::atomic<bool> m_IsRunning;
    std::thread m_ReadThread;
    std::optional<std::string> m_SessionID;

    MessageCallback m_OnMessage;
    ErrorCallback m_OnError;
    CloseCallback m_OnClose;
    StartCallback m_OnStart;
    StopCallback m_OnStop;
};

MCP_NAMESPACE_END