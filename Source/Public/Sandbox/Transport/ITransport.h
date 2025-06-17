#pragma once

#include "Core.h"
#include "ErrorBase.h"
#include "MessageBase.h"

// Poco Net includes
#include <Poco/Event.h>
#include <Poco/Exception.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPServer.h>
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

// Transport types for easy selection
enum class TransportType {
    Stdio,   // Standard input/output
    HTTP,    // HTTP with Server-Sent Events
    InMemory // In-memory transport (for testing)
};

/**
 * @brief Abstract base class for MCP Transport.
 *
 * This class defines the interface for sending and receiving JSON-RPC messages
 * as per the Model Context Protocol (MCP) specification.
 */
class ITransport {
  public:
    virtual ~ITransport() = default;

    // Core transport operations
    virtual MCPTask_Void Connect() = 0;
    virtual MCPTask_Void Disconnect() = 0;
    virtual MCPTask_Void SendMessage(const MessageBase& InMessage) = 0;
    virtual MCPTask_Void SendBatch(const JSONRPCBatch& InBatch) = 0;

    // Message reception callbacks
    virtual void SetMessageHandler(std::function<void(const MessageBase&)> InHandler) = 0;
    virtual void SetErrorHandler(std::function<void(const std::string&)> InHandler) = 0;

    // State management
    virtual bool IsConnected() const = 0;
    virtual TransportType GetTransportType() const = 0;

  protected:
    // --- Callback Types (following naming conventions) ---
    using OnCloseDelegate = std::function<void()>;
    using OnErrorDelegate = std::function<void(const ErrorBase& InError)>;
    using OnMessageDelegate = std::function<void(const MessageBase& InMessage)>;
    using OnRawMessageDelegate = std::function<void(const std::string& InRawMessage)>;
    using OnStateChangeDelegate =
        std::function<void(const std::string& InOldState, const std::string& InNewState)>;

    OnCloseDelegate m_OnClose;
    OnErrorDelegate m_OnError;
    OnMessageDelegate m_OnMessage;
    OnStateChangeDelegate m_OnStateChange;

    ITransport() = default;

    ITransport(OnCloseDelegate InOnClose, OnErrorDelegate InOnError, OnMessageDelegate InOnMessage)
        : m_OnClose(std::move(InOnClose)), m_OnError(std::move(InOnError)),
          m_OnMessage(std::move(InOnMessage)) {}

    ITransport(OnCloseDelegate InOnClose, OnErrorDelegate InOnError, OnMessageDelegate InOnMessage,
               OnStateChangeDelegate InOnStateChange)
        : m_OnClose(std::move(InOnClose)), m_OnError(std::move(InOnError)),
          m_OnMessage(std::move(InOnMessage)), m_OnStateChange(std::move(InOnStateChange)) {}

  public:
    // Disallow copy and move operations to prevent slicing
    ITransport(const ITransport&) = delete;
    ITransport& operator=(const ITransport&) = delete;
    ITransport(ITransport&&) = delete;
    ITransport& operator=(ITransport&&) = delete;
};

MCP_NAMESPACE_END