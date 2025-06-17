#pragma once

#include "Core.h"
#include "ErrorBase.h"
#include "IProtocol.h"
#include "MessageBase.h"

MCP_NAMESPACE_BEGIN

// Transport types for easy selection
enum class TransportType {
    Stdio,     // Standard input/output
    HTTP,      // HTTP with Server-Sent Events
    WebSocket, // WebSocket transport
    InMemory   // In-memory transport (for testing)
};

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
    virtual ~ITransport() = default;

    /**
     * @brief Starts the transport, establishing connections and beginning message processing.
     * @return Coroutine task that completes when transport is started
     */
    virtual MCPTask_Void Start() = 0;

    /**
     * @brief Sends a JSON_RPC message over the transport.
     * @param InMessage Reference to the JSON_RPC message to send
     * @return Coroutine task that completes when message is sent
     */
    virtual MCPTask_Void Send(const MessageBase& InMessage) = 0;

    /**
     * @brief Closes the transport connection and cleans up resources.
     * @return Coroutine task that completes when transport is closed
     */
    virtual MCPTask_Void Close() = 0;

    /**
     * @brief Sends raw JSON string for protocol-level operations.
     * @param InJSON_Message Raw JSON string to send
     * @return Coroutine task that completes when message is sent
     */
    virtual MCPTask_Void SendRaw(const std::string& InJSON_Message) = 0;

    // TODO: @HalcyonOmega: Add transport type enum and update output
    // --- Connection State Management ---

    /**
     * @brief Check if transport is currently connected and ready
     */
    virtual bool IsConnected() const = 0;

    /**
     * @brief Check if transport is currently starting up
     */
    virtual bool IsStarting() const = 0;

    /**
     * @brief Get current connection state as string for debugging
     */
    virtual std::string GetConnectionState() const = 0;

    // --- Transport Metadata ---

    /**
     * @brief Get transport type (e.g., "stdio", "HTTP", "websocket")
     */
    virtual TransportType GetTransportType() const = 0;

    /**
     * @brief Get optional connection information for debugging/logging
     */
    virtual std::optional<std::string> GetConnectionInfo() const = 0;

  protected:
    // --- Callback Types (following naming conventions) ---
    using OnCloseDelegate = std::function<void()>;
    using OnErrorDelegate = std::function<void(const ErrorBase& InError)>;
    using OnMessageDelegate = std::function<void(const MessageBase& InMessage)>;
    using OnRawMessageDelegate = std::function<void(const std::string& InRawMessage)>;
    using OnStateChangeDelegate =
        std::function<void(const std::string& InOldState, const std::string& InNewState)>;

    /**
     * @brief Callback invoked when transport connection is closed
     */
    OnCloseDelegate m_OnClose;

    /**
     * @brief Callback invoked when transport error occurs
     */
    OnErrorDelegate m_OnError;

    /**
     * @brief Callback invoked when JSON_RPC message is received
     */
    OnMessageDelegate m_OnMessage;

    /**
     * @brief Callback invoked when raw JSON message is received (before parsing)
     */
    OnRawMessageDelegate m_OnRawMessage;

    /**
     * @brief Callback invoked when transport state changes
     */
    OnStateChangeDelegate m_OnStateChange;

    /**
     * @brief Default constructor for derived classes
     */
    ITransport() = default;

    /**
     * @brief Basic constructor for derived classes to initialize core callbacks
     */
    ITransport(OnCloseDelegate InOnClose, OnErrorDelegate InOnError, OnMessageDelegate InOnMessage)
        : m_OnClose(std::move(InOnClose)), m_OnError(std::move(InOnError)),
          m_OnMessage(std::move(InOnMessage)) {}

    /**
     * @brief Enhanced constructor with all callback support
     */
    ITransport(OnCloseDelegate InOnClose, OnErrorDelegate InOnError, OnMessageDelegate InOnMessage,
               OnRawMessageDelegate InOnRawMessage, OnStateChangeDelegate InOnStateChange)
        : m_OnClose(std::move(InOnClose)), m_OnError(std::move(InOnError)),
          m_OnMessage(std::move(InOnMessage)), m_OnRawMessage(std::move(InOnRawMessage)),
          m_OnStateChange(std::move(InOnStateChange)) {}

  public:
    // Disallow copy and move operations to prevent slicing
    ITransport(const ITransport&) = delete;
    ITransport& operator=(const ITransport&) = delete;
    ITransport(ITransport&&) = delete;
    ITransport& operator=(ITransport&&) = delete;
};

MCP_NAMESPACE_END