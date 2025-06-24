#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Messages/ErrorBase.h"
#include "CoreSDK/Messages/MessageBase.h"
#include "CoreSDK/Messages/NotificationBase.h"
#include "CoreSDK/Messages/RequestBase.h"
#include "CoreSDK/Messages/ResponseBase.h"
#include "JSONProxy.h"
#include "Utilities/Async/MCPTask.h"

MCP_NAMESPACE_BEGIN

// Transport events
enum class TransportState { Disconnected, Connecting, Connected, Error };

// Message handlers
using MessageHandler = std::function<void(const MessageBase& InMessage)>;
using RequestHandler = std::function<void(const RequestBase& InRequest)>;
using ResponseHandler = std::function<void(const ResponseBase& InResponse)>;
using NotificationHandler = std::function<void(const NotificationBase& InNotification)>;
using ErrorHandler = std::function<void(const ErrorBase& InError)>;
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
    [[nodiscard]] virtual bool IsConnected() const = 0;
    [[nodiscard]] virtual TransportState GetState() const = 0;

    // Message sending
    virtual MCPTask_Void TransmitMessage(const JSONValue& InMessage) = 0;

    // Event handlers
    virtual void SetMessageHandler(MessageHandler InHandler) {
        m_MessageHandler = InHandler;
    };
    virtual void SetRequestHandler(RequestHandler InHandler) {
        m_RequestHandler = InHandler;
    };
    virtual void SetResponseHandler(ResponseHandler InHandler) {
        m_ResponseHandler = InHandler;
    };
    virtual void SetNotificationHandler(NotificationHandler InHandler) {
        m_NotificationHandler = InHandler;
    };
    virtual void SetErrorHandler(ErrorHandler InHandler) {
        m_ErrorHandler = InHandler;
    };
    virtual void SetStateChangeHandler(StateChangeHandler InHandler) {
        m_StateChangeHandler = InHandler;
    };

    // Utility
    [[nodiscard]] virtual std::string GetConnectionInfo() const = 0;

  protected:
    // Helper methods for derived classes
    [[nodiscard]] bool IsValidJSONRPC(const JSONValue& InMessage) const;
    void TriggerStateChange(TransportState InNewState);

    TransportState m_CurrentState{TransportState::Disconnected};

    // Event handlers
    MessageHandler m_MessageHandler;
    RequestHandler m_RequestHandler;
    ResponseHandler m_ResponseHandler;
    NotificationHandler m_NotificationHandler;
    ErrorHandler m_ErrorHandler;
    StateChangeHandler m_StateChangeHandler;

  private:
    mutable std::atomic<uint64_t> m_RequestCounter{0};
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

// Helper functions for message parsing
namespace MessageUtils {
[[nodiscard]] std::optional<JSONValue> ParseJSONMessage(const std::string& InRawMessage);
[[nodiscard]] bool IsRequest(const JSONValue& InMessage);
[[nodiscard]] bool IsResponse(const JSONValue& InMessage);
[[nodiscard]] bool IsNotification(const JSONValue& InMessage);
[[nodiscard]] std::string ExtractMethod(const JSONValue& InMessage);
[[nodiscard]] std::string ExtractRequestID(const JSONValue& InMessage);
[[nodiscard]] JSONValue ExtractParams(const JSONValue& InMessage);
[[nodiscard]] JSONValue ExtractResult(const JSONValue& InMessage);
[[nodiscard]] JSONValue ExtractError(const JSONValue& InMessage);
} // namespace MessageUtils

MCP_NAMESPACE_END