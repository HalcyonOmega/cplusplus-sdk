#pragma once

#include "../ITransport.h"
#include "Communication/Utilities/ReadBuffer.h"
#include "Core.h"
#include "Poco/PipeStream.h"

MCP_NAMESPACE_BEGIN

// TODO: @HalcyonOmega Fix conversion of typescript to c++
using Buffer = Poco::Buffer<char>;

class StdioServerTransport : public ITransport {
  public:
    // === ITransport Implementation ===
    MCPTask_Void Connect() override;
    MCPTask_Void Disconnect() override;
    MCPTask_Void SendMessage(const MessageBase& InMessage) override;
    // === End ITransport Implementation ===

  private:
    void OnData(const Buffer& InChunk);
    void ProcessReadBuffer();

    ReadBuffer m_ReadBuffer;
    bool m_Started = false;

    Poco::PipeInputStream m_Stdin;
    Poco::PipeOutputStream m_Stdout;

    // === ITransport Members ===
    optional<string> m_SessionID;

    // Callbacks
    optional<ConnectCallback> m_ConnectCallback;
    optional<DisconnectCallback> m_DisconnectCallback;
    optional<ErrorCallback> m_ErrorCallback;
    optional<MessageCallback> m_MessageCallback;
};

MCP_NAMESPACE_END