#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Messages/ErrorResponseBase.h"
#include "CoreSDK/Messages/NotificationBase.h"
#include "CoreSDK/Messages/RequestBase.h"
#include "CoreSDK/Messages/ResponseBase.h"
#include "JSONProxy.h"
#include "Utilities/Async/MCPTask.h"

MCP_NAMESPACE_BEGIN

// Transport events
enum class TransportState { Disconnected, Connecting, Connected, Error };

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

    void SetRequestRouter(std::function<void(const RequestBase&)> InRouter);
    void SetResponseRouter(std::function<void(const ResponseBase&)> InRouter);
    void SetNotificationRouter(std::function<void(const NotificationBase&)> InRouter);
    void SetErrorResponseRouter(std::function<void(const ErrorResponseBase&)> InRouter);

    void CallRequestRouter(const RequestBase& InRequest);
    void CallResponseRouter(const ResponseBase& InResponse);
    void CallNotificationRouter(const NotificationBase& InNotification);
    void CallErrorResponseRouter(const ErrorResponseBase& InError);

  protected:
    TransportState m_CurrentState{TransportState::Disconnected};

    // Event handlers
    RequestHandler m_RequestRouter;
    ResponseHandler m_ResponseRouter;
    NotificationHandler m_NotificationRouter;
    ErrorResponseHandler m_ErrorResponseRouter;
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