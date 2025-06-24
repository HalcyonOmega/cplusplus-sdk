#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "CoreSDK/Common/EventSignatures.h"
#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"
#include "Utilities/Async/MCPTask.h"

MCP_NAMESPACE_BEGIN

// Transport events
enum class TransportState { Disconnected, Connecting, Connected, Error };

using StateChangeHandler =
    std::function<void(TransportState InOldState, TransportState InNewState)>;

// Transport options for different transport types
struct TransportOptions {
    virtual ~TransportOptions() noexcept = default;
};

struct StdioClientTransportOptions : TransportOptions {
    bool UseStderr{false};
    std::string Command;
    std::vector<std::string> Arguments;
};

struct HTTPTransportOptions : TransportOptions {
    static constexpr std::chrono::milliseconds DEFAULT_CONNECT_TIMEOUT{5000};
    static constexpr std::chrono::milliseconds DEFAULT_REQUEST_TIMEOUT{30000};
    static constexpr std::string_view DEFAULT_HOST{"localhost"};
    static constexpr uint16_t DEFAULT_PORT{8080};
    static constexpr std::string_view DEFAULT_PATH{"/mcp"};

    bool UseHTTPS = false;
    uint16_t Port = DEFAULT_PORT;
    std::string Host = std::string{DEFAULT_HOST};
    std::string Path = std::string{DEFAULT_PATH};
    std::chrono::milliseconds ConnectTimeout{DEFAULT_CONNECT_TIMEOUT};
    std::chrono::milliseconds RequestTimeout{DEFAULT_REQUEST_TIMEOUT};
};

// Transport interface
class ITransport {
  public:
    virtual ~ITransport() = default;

    // Connection management
    virtual MCPTask_Void Start() = 0;
    virtual MCPTask_Void Stop() = 0;
    [[nodiscard]] virtual bool IsConnected() const {
        return m_CurrentState == TransportState::Connected;
    }
    [[nodiscard]] virtual TransportState GetState() const {
        return m_CurrentState;
    }

    // Message sending
    virtual MCPTask_Void TransmitMessage(const JSONValue& InMessage) = 0;

    // Utility
    [[nodiscard]] virtual std::string GetConnectionInfo() const = 0;

  private:
    mutable std::atomic<uint64_t> m_RequestCounter{0};

  protected:
    // Helper methods for derived classes
    void TriggerStateChange(TransportState InNewState);

    TransportState m_CurrentState{TransportState::Disconnected};

    // Event handlers
    MessageHandler m_MessageHandler;
    RequestHandler m_RequestHandler;
    ResponseHandler m_ResponseHandler;
    NotificationHandler m_NotificationHandler;
    ErrorResponseHandler m_ErrorResponseHandler;
    StateChangeHandler m_StateChangeHandler;

  public:
    void SetMessageHandler(MessageHandler InHandler) {
        m_MessageHandler = InHandler;
    }
    void SetRequestHandler(RequestHandler InHandler) {
        m_RequestHandler = InHandler;
    }
    void SetResponseHandler(ResponseHandler InHandler) {
        m_ResponseHandler = InHandler;
    }
    void SetNotificationHandler(NotificationHandler InHandler) {
        m_NotificationHandler = InHandler;
    }
    void SetErrorResponseHandler(ErrorResponseHandler InHandler) {
        m_ErrorResponseHandler = InHandler;
    }
    void SetStateChangeHandler(StateChangeHandler InHandler) {
        m_StateChangeHandler = InHandler;
    }

    void CallMessageHandler(const MessageBase& InMessage) {
        if (m_MessageHandler) { m_MessageHandler(InMessage); }
    }
    void CallRequestHandler(const RequestBase& InRequest) {
        if (m_RequestHandler) { m_RequestHandler(InRequest); }
    }
    void CallResponseHandler(const ResponseBase& InResponse) {
        if (m_ResponseHandler) { m_ResponseHandler(InResponse); }
    }
    void CallNotificationHandler(const NotificationBase& InNotification) {
        if (m_NotificationHandler) { m_NotificationHandler(InNotification); }
    }
    void CallErrorResponseHandler(const ErrorResponseBase& InError) {
        if (m_ErrorResponseHandler) { m_ErrorResponseHandler(InError); }
    }
    void CallStateChangeHandler(TransportState InOldState, TransportState InNewState) {
        if (m_StateChangeHandler) { m_StateChangeHandler(InOldState, InNewState); }
    }
};

// Transport factory
enum class TransportType { Stdio, StreamableHTTP };

class TransportFactory {
  public:
    [[nodiscard]] static std::unique_ptr<ITransport>
    CreateTransport(TransportType InType, std::unique_ptr<TransportOptions> InOptions);

    // Convenience factory methods
    [[nodiscard]] static std::unique_ptr<ITransport>
    CreateStdioClientTransport(const StdioClientTransportOptions& InOptions);
    [[nodiscard]] static std::unique_ptr<ITransport>
    CreateHTTPTransport(const HTTPTransportOptions& InOptions);
};

MCP_NAMESPACE_END