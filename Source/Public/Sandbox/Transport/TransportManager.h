
#pragma once

#include "Core.h"
#include "ITransport.h"

// Poco Net includes
#include <Poco/Event.h>
#include <Poco/Exception.h>
#include <Poco/Net/NetException.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Pipe.h>
#include <Poco/PipeStream.h>
#include <Poco/Process.h>
#include <Poco/Runnable.h>
#include <Poco/StreamCopier.h>
#include <Poco/Thread.h>
#include <Poco/URI.h>

MCP_NAMESPACE_BEGIN

// Transport Manager - Orchestrates transport lifecycle
class TransportManager {
  private:
    std::unique_ptr<ITransport> m_Transport;
    bool m_IsInitialized;

  public:
    TransportManager() : m_IsInitialized(false) {}

    void SetTransport(std::unique_ptr<ITransport> InTransport) {
        m_Transport = std::move(InTransport);
    }

    MCPTask_Void Initialize() {
        if (!m_Transport) { throw std::runtime_error("No transport configured"); }

        co_await m_Transport->Connect();
        m_IsInitialized = true;
        co_return;
    }

    MCPTask_Void Shutdown() {
        if (m_Transport && m_IsInitialized) {
            co_await m_Transport->Disconnect();
            m_IsInitialized = false;
        }
        co_return;
    }

    MCPTask_Void Send(const MessageBase& InMessage) {
        if (!m_IsInitialized) { throw std::runtime_error("Transport not initialized"); }

        co_await m_Transport->SendMessage(InMessage);
        co_return;
    }

    void SetMessageHandler(std::function<void(const MessageBase&)> InHandler) {
        if (m_Transport) { m_Transport->SetMessageHandler(InHandler); }
    }

    void SetErrorHandler(std::function<void(const std::string&)> InHandler) {
        if (m_Transport) { m_Transport->SetErrorHandler(InHandler); }
    }

    bool IsConnected() const {
        return m_Transport && m_Transport->IsConnected();
    }

    std::string GetTransportType() const {
        return m_Transport ? m_Transport->GetTransportType() : "none";
    }
};

MCP_NAMESPACE_END