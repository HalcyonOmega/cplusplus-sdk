#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN
/**
 * @brief Abstract base class for MCP Transport.
 *
 * This class defines the interface for sending and receiving JSON-RPC messages
 * as per the Model Context Protocol (MCP) specification.
 */
class TransportUtils {
  public:
    virtual ~TransportUtils() = default;

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

    // TODO: @HalcyonOmega Begin Direct Translated Code
    // Getters
    optional<string> GetSessionID() const {
        return m_SessionID;
    }

    // Note: Resumability is not yet supported by any transport implementation.
    [[deprecated("Not yet implemented - will be supported in a future version")]]
    virtual bool Resume(const string& InResumptionToken) = 0;

    optional<StartCallback> OnStart;

    // Callback for when the connection is closed for any reason. This should be invoked when
    // Close() is called as well.
    optional<CloseCallback> OnClose;

    // Callback for when an error occurs. Note that errors are not necessarily fatal; they are used
    // for reporting any kind of exceptional condition out of band.
    optional<ErrorCallback> OnError;

    // Callback for when a message (request or response) is received over the connection. Includes
    // the AuthInfo if the transport is authenticated.
    optional<MessageCallback> OnMessage;

  protected:
    // Helper methods to safely invoke callbacks with proper locking and null checks
    void CallOnStart() const {
        lock_guard<mutex> lock(m_CallbackMutex);
        if (OnStart) { (*OnStart)(); }
    }

    void CallOnClose() const {
        lock_guard<mutex> lock(m_CallbackMutex);
        if (OnClose) { (*OnClose)(); }
    }

    void CallOnError(const ErrorBase& InError) const {
        lock_guard<mutex> lock(m_CallbackMutex);
        if (OnError) { (*OnError)(InError); }
    }

    void CallOnError(const string& InMessage) const {
        ErrorBase Error(Errors::InternalError, InMessage);
        CallOnError(Error);
    }

    void CallOnMessage(const MessageBase& InMessage,
                       const optional<AuthInfo>& InAuthInfo = nullopt) const {
        lock_guard<mutex> lock(m_CallbackMutex);
        if (OnMessage) { (*OnMessage)(InMessage, InAuthInfo); }
    }

  private:
    // The session ID generated for this connection.
    optional<string> m_SessionID;

    mutable mutex m_CallbackMutex; // Protects callback invocation to avoid concurrent access

  public:
    // Starts processing messages on the transport, including any connection steps that might need
    // to be taken. This method should only be called after callbacks are installed, or else
    // messages may be lost. NOTE: This method should not be called explicitly when using Client,
    // Server, or Protocol classes, as they will implicitly call Start().
    virtual future<void> Start() = 0;

    // Closes the connection.
    virtual future<void> Close() = 0;

    // Sends a JSON-RPC message (request or response). If present, `relatedRequestID` is used to
    // indicate to the transport which incoming request to associate this outgoing message with.
    // TODO: @HalcyonOmega Should the TransportSendOptions be optional?
    virtual future<void> Send(const MessageBase& InMessage,
                              const TransportSendOptions& InOptions = {}) = 0;
    // TODO: @HalcyonOmega End Direct Translated Code

  protected:
    // --- Callback Types (following naming conventions) ---

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