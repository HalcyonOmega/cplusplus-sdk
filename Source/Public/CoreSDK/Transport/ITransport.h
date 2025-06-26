#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
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

    virtual MCPTask_Void Connect() = 0;
    virtual MCPTask_Void Disconnect() = 0;
    virtual MCPTask_Void TransmitMessage(const JSONValue& InMessage) = 0;
    virtual MCPTask<JSONValue> TransmitRequest(const JSONValue& InRequest) = 0;
    [[nodiscard]] virtual std::string GetConnectionInfo() const = 0;

    // Default Implementations
    [[nodiscard]] bool IsConnected() const;
    [[nodiscard]] TransportState GetState() const;
    void SetState(TransportState InNewState);
    void SetRequestHandler(RequestHandler InHandler);
    void SetResponseHandler(ResponseHandler InHandler);
    void SetNotificationHandler(NotificationHandler InHandler);
    void SetErrorResponseHandler(ErrorResponseHandler InHandler);
    void SetStateChangeHandler(StateChangeHandler InHandler);
    void CallRequestHandler(const RequestBase& InRequest);
    void CallResponseHandler(const ResponseBase& InResponse);
    void CallNotificationHandler(const NotificationBase& InNotification);
    void CallErrorResponseHandler(const ErrorResponseBase& InError);

  protected:
    TransportState m_CurrentState{TransportState::Disconnected};

    // Event handlers
    RequestHandler m_RequestHandler;
    ResponseHandler m_ResponseHandler;
    NotificationHandler m_NotificationHandler;
    ErrorResponseHandler m_ErrorResponseHandler;
};

// Transport factory
enum class TransportType { Stdio, StreamableHTTP };

enum class TransportSide { Client, Server };

class TransportFactory {
  public:
    [[nodiscard]] static std::unique_ptr<ITransport>
    CreateTransport(TransportType InType, TransportSide InSide,
                    std::optional<std::unique_ptr<TransportOptions>> InOptions);

    // Convenience factory methods
    [[nodiscard]] static std::unique_ptr<ITransport>
    CreateStdioClientTransport(const StdioClientTransportOptions& InOptions);
    [[nodiscard]] static std::unique_ptr<ITransport>
    CreateHTTPTransport(const HTTPTransportOptions& InOptions);
};

MCP_NAMESPACE_END