#pragma once

#include "Core.h"
#include "Transport.h"
#include "Utilities/HTTP/HTTPLayer.hpp"

MCP_NAMESPACE_BEGIN

// Default reconnection options for StreamableHTTP connections
struct StreamableHTTPReconnectionOptions {
    int MaxReconnectionDelay = 30000;    // 30 seconds
    int InitialReconnectionDelay = 1000; // 1 second
    double ReconnectionDelayGrowFactor = 1.5;
    int MaxRetries = 2;
};

const StreamableHTTPReconnectionOptions DEFAULT_STREAMABLE_HTTP_RECONNECTION_OPTIONS = {
    1000,  // initialReconnectionDelay
    30000, // maxReconnectionDelay
    1.5,   // reconnectionDelayGrowFactor
    2      // maxRetries
};

class StreamableHTTPTransportBase : public Transport {
  public:
    explicit StreamableHTTPTransportBase(const string& InURL);
    ~StreamableHTTPTransportBase() override;

    // Transport interface implementation
    future<void> Start() override;
    future<void> Close() override;
    future<void> Send(const MessageBase& InMessage,
                      const TransportSendOptions& InOptions = {}) override;
    void WriteSSEEvent(const string& InEvent, const string& InData) override;

    // New method for resumability support
    bool Resume(const string& InResumptionToken) override;

  private:
    void ReadLoop();
    void ParseSSEData(const string& InData);

    string m_URL;
    string m_Path;
    int m_Port;
    unique_ptr<HTTP_Client> m_Client;
    atomic<bool> m_IsRunning;
    thread m_ReadThread;
};

MCP_NAMESPACE_END