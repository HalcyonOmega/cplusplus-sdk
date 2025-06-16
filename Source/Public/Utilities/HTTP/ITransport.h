#pragma once

// Standard Library
#include <functional>
#include <string>
#include <utility>

#include "Core.h"
#include "ErrorBase.h"
#include "MessageBase.h"

MCP_NAMESPACE_BEGIN

/**
 * @brief Abstract base class for MCP Transport.
 *
 * This class defines the interface for sending and receiving JSON-RPC messages
 * as per the Model Context Protocol (MCP) specification. Implementations will
 * leverage Poco::Net for the underlying communication (e.g., Stdio, HTTP/SSE).
 *
 * Derived classes are responsible for:
 * - Establishing and managing the connection (e.g., via Poco::Net::StreamSocket,
 * Poco::Net::HTTPClientSession).
 * - Serializing MCP messages to JSON-RPC strings and parsing incoming JSON-RPC strings.
 * - Invoking the OnMessage, OnError, and OnClose callbacks appropriately.
 * - Handling threading and asynchronous operations as required by the specific transport.
 *   For example, a Start() implementation might spawn a Poco::Thread to listen for incoming
 * messages. A Send() implementation might use a Poco::Net::HTTPSession to send data.
 */
class ITransport {
  public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~ITransport() = default;

    // --- Core Transport Methods ---

    /**
     * @brief Starts the transport, establishing any necessary connections
     *        and beginning message processing.
     *
     * This method may be blocking or asynchronous depending on the implementation.
     * If asynchronous, it should initiate the connection process.
     */
    virtual void Start() = 0;

    /**
     * @brief Sends a JSON-RPC message over the transport.
     * @param InMessage A reference to the JSON-RPC message to send.
     *
     * This method may be blocking or asynchronous.
     */
    virtual void Send(const MessageBase& InMessage) = 0;

    /**
     * @brief Closes the transport connection and cleans up resources.
     *
     * This method should close the transport connection and clean up resources.
     */
    virtual void Close() = 0;

  protected:
    // --- Callbacks ---
    // These should be set by the user of the transport (e.g., Client or Server class)
    // via constructor.

    // Type aliases for callbacks for clarity
    using OnCloseDelegate = function<void()>;
    using OnErrorDelegate = function<void(const ErrorBase& InError)>;
    using OnMessageDelegate = function<void(const MessageBase& InMessage)>;

    /**
     * @brief Callback invoked when the transport connection is closed.
     */
    OnCloseDelegate m_OnClose;

    /**
     * @brief Callback invoked when a transport error occurs.
     * @param InError The error that occurred.
     */
    OnErrorDelegate m_OnError;

    /**
     * @brief Callback invoked when a JSON-RPC message is received.
     * @param InMessage A reference to the parsed JSON-RPC message.
     */
    OnMessageDelegate m_OnMessage;

    /**
     * @brief Default constructor for derived classes.
     */
    ITransport() = default;

    /**
     * @brief Constructor for derived classes to initialize callbacks.
     * @param InOnClose Delegate for the OnClose event.
     * @param InOnError Delegate for the OnError event.
     * @param InOnMessage Delegate for the OnMessage event.
     */
    ITransport(OnCloseDelegate InOnClose, OnErrorDelegate InOnError, OnMessageDelegate InOnMessage)
        : m_OnClose(move(InOnClose)), m_OnError(move(InOnError)), m_OnMessage(move(InOnMessage)) {}

  public:
    // Disallow copy and move operations to prevent slicing and ensure
    // derived classes manage their resources correctly.
    ITransport(const ITransport&) = delete;
    ITransport& operator=(const ITransport&) = delete;
    ITransport(ITransport&&) = delete;
    ITransport& operator=(ITransport&&) = delete;
};

MCP_NAMESPACE_END