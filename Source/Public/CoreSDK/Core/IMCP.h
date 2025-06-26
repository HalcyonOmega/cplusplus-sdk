#pragma once

#include <chrono>
#include <future>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Common/RuntimeError.h"
#include "CoreSDK/Messages/MCPMessages.h"
#include "CoreSDK/Transport/ITransport.h"
#include "Utilities/Async/MCPTask.h"

MCP_NAMESPACE_BEGIN

static constexpr double DEFAULT_TEMPERATURE{0.7};

// Protocol state
enum class MCPProtocolState { Uninitialized, Initializing, Initialized, Error, Shutdown };

// Base protocol handler
class MCPProtocol {
  public:
    explicit MCPProtocol(std::unique_ptr<ITransport> InTransport);
    virtual ~MCPProtocol() noexcept;

    // Lifecycle
    virtual MCPTask_Void Start() = 0;
    virtual MCPTask_Void Stop() = 0;
    bool IsInitialized() const;
    MCPProtocolState GetState() const;
    void SetState(MCPProtocolState InNewState);
    ITransport* GetTransport() const; // TODO: @HalcyonOmega Consider converting to reference
    bool IsConnected() const;

    // Core protocol operations
    MCPTask<PingResponse> Ping();

    // Protocol version validation
    static const std::vector<std::string> SUPPORTED_PROTOCOL_VERSIONS;
    void ValidateProtocolVersion(const std::string& InVersion) const;

    // Message sending utilities
    MCPTask_Void SendRequest(const RequestBase& InRequest);
    MCPTask_Void SendResponse(const ResponseBase& InResponse);
    MCPTask_Void SendNotification(const NotificationBase& InResponse);
    MCPTask_Void SendErrorResponse(const ErrorResponseBase& InError);

    void RegisterRequestHandler(const RequestBase& InRequest);
    void RegisterResponseHandler(const ResponseBase& InResponse);
    void RegisterNotificationHandler(const NotificationBase& InNotification);
    void RegisterErrorResponseHandler(const ErrorResponseBase& InErrorResponse);

  private:
    void SetupTransportHandlers();
    virtual void HandleRequest(const RequestBase& InRequest);
    virtual void HandleResponse(const ResponseBase& InResponse);
    virtual void HandleNotification(const NotificationBase& InNotification);
    virtual void HandleErrorResponse(const ErrorResponseBase& InError);

  protected:
    MCPProtocolState m_State;
    std::unique_ptr<ITransport> m_Transport;

  private:
    // Request tracking
    struct PendingResponse {
        RequestID RequestID;
        std::promise<JSONValue> Promise;
        std::chrono::steady_clock::time_point StartTime;
    };

    std::unordered_map<std::string, std::unique_ptr<PendingResponse>> m_PendingResponses;
    mutable std::mutex m_ResponsesMutex;

    mutable std::mutex m_HandlersMutex;

    // Vectorized handler registries - store full message objects
    std::vector<RequestBase> m_RegisteredRequests;
    std::vector<ResponseBase> m_RegisteredResponses;
    std::vector<NotificationBase> m_RegisteredNotifications;
    std::vector<ErrorResponseBase> m_RegisteredErrorResponses;

  protected:
    Implementation m_ServerInfo;
    Implementation m_ClientInfo;
    ServerCapabilities m_ServerCapabilities;
    ClientCapabilities m_ClientCapabilities;

  public:
    template <typename T> MCPTask<T> SendRequest(const RequestBase& InRequest) {
        (void)InRequest;
        co_return {};
    }

    template <typename T> MCPTask<T> SendResponse(const ResponseBase& InResponse) {
        (void)InResponse;
        co_return {};
    }
};

MCP_NAMESPACE_END
