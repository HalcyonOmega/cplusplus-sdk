#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "MCPTask.h"
#include "json.hpp"

// Forward declarations
struct RequestBase;
struct ResponseBase;
struct NotificationBase;

// Transport events
enum class TransportState { Disconnected, Connecting, Connected, Error };

// Message handlers
using MessageHandler = std::function<void(const std::string& InRawMessage)>;
using RequestHandler = std::function<void(
    const std::string& InMethod, const nlohmann::json& InParams, const std::string& InRequestID)>;
using ResponseHandler =
    std::function<void(const nlohmann::json& InResult, const std::string& InRequestID)>;
using NotificationHandler =
    std::function<void(const std::string& InMethod, const nlohmann::json& InParams)>;
using ErrorHandler = std::function<void(const std::string& InErrorMessage)>;
using StateChangeHandler =
    std::function<void(TransportState InOldState, TransportState InNewState)>;

// Transport options for different transport types
struct TransportOptions {
    virtual ~TransportOptions() = default;
};

struct StdioTransportOptions : TransportOptions {
    bool UseStderr = false;
    std::string Command;
    std::vector<std::string> Arguments;
};

struct HTTPTransportOptions : TransportOptions {
    std::string Host = "localhost";
    uint16_t Port = 8080;
    std::string Path = "/mcp";
    bool UseHTTPS = false;
    std::chrono::milliseconds ConnectTimeout{5000};
    std::chrono::milliseconds RequestTimeout{30000};
};

// Transport interface
class ITransport {
  public:
    virtual ~ITransport() = default;

    // Connection management
    virtual MCPTaskVoid Start() = 0;
    virtual MCPTaskVoid Stop() = 0;
    virtual bool IsConnected() const = 0;
    virtual TransportState GetState() const = 0;

    // Message sending
    virtual MCPTask<std::string> SendRequest(const std::string& InMethod,
                                             const nlohmann::json& InParams) = 0;
    virtual MCPTaskVoid SendResponse(const std::string& InRequestID,
                                     const nlohmann::json& InResult) = 0;
    virtual MCPTaskVoid SendErrorResponse(const std::string& InRequestID, int64_t InErrorCode,
                                          const std::string& InErrorMessage,
                                          const nlohmann::json& InErrorData = {}) = 0;
    virtual MCPTaskVoid SendNotification(const std::string& InMethod,
                                         const nlohmann::json& InParams = {}) = 0;

    // Event handlers
    virtual void SetMessageHandler(MessageHandler InHandler) = 0;
    virtual void SetRequestHandler(RequestHandler InHandler) = 0;
    virtual void SetResponseHandler(ResponseHandler InHandler) = 0;
    virtual void SetNotificationHandler(NotificationHandler InHandler) = 0;
    virtual void SetErrorHandler(ErrorHandler InHandler) = 0;
    virtual void SetStateChangeHandler(StateChangeHandler InHandler) = 0;

    // Utility
    virtual std::string GetConnectionInfo() const = 0;

  protected:
    // Helper methods for derived classes
    std::string GenerateRequestID() const;
    bool IsValidJSONRPC(const nlohmann::json& InMessage) const;
    void TriggerStateChange(TransportState InNewState);

    TransportState m_CurrentState = TransportState::Disconnected;

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
    static std::unique_ptr<ITransport> CreateTransport(TransportType InType,
                                                       std::unique_ptr<TransportOptions> InOptions);

    // Convenience factory methods
    static std::unique_ptr<ITransport> CreateStdioTransport(const StdioTransportOptions& InOptions);
    static std::unique_ptr<ITransport> CreateHTTPTransport(const HTTPTransportOptions& InOptions);
};

// Helper functions for message parsing
namespace MessageUtils {
std::optional<nlohmann::json> ParseJSONMessage(const std::string& InRawMessage);
bool IsRequest(const nlohmann::json& InMessage);
bool IsResponse(const nlohmann::json& InMessage);
bool IsNotification(const nlohmann::json& InMessage);
std::string ExtractMethod(const nlohmann::json& InMessage);
std::string ExtractRequestID(const nlohmann::json& InMessage);
nlohmann::json ExtractParams(const nlohmann::json& InMessage);
nlohmann::json ExtractResult(const nlohmann::json& InMessage);
nlohmann::json ExtractError(const nlohmann::json& InMessage);
} // namespace MessageUtils