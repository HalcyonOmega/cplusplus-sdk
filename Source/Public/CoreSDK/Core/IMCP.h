#pragma once

#include <chrono>
#include <future>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "CoreSDK/Common/Capabilities.h"
#include "CoreSDK/Common/Implementation.h"
#include "CoreSDK/Common/Macros.h"
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
    virtual ~MCPProtocol();

    // Lifecycle
    MCPTask_Void Start();
    MCPTask_Void Shutdown();
    bool IsInitialized() const {
        return m_State == MCPProtocolState::Initialized;
    }
    MCPProtocolState GetState() const {
        return m_State;
    }

    // Core protocol operations
    MCPTask<InitializeResponse::InitializeResult>
    Initialize(const std::string& InProtocolVersion, const ClientCapabilities& InCapabilities,
               const Implementation& InClientInfo);
    MCPTask_Void SendInitialized();
    MCPTask<PingResponse> Ping();

    // Protocol version validation
    static const std::vector<std::string> SUPPORTED_PROTOCOL_VERSIONS;
    void ValidateProtocolVersion(const std::string& InVersion) const;

    // Message sending utilities
    MCPTask<const ResponseBase&> SendRequest(const RequestBase& InRequest);
    MCPTask_Void SendResponse(const ResponseBase& InResponse);
    MCPTask_Void SendNotification(const NotificationBase& InNotification);
    MCPTask_Void SendError(const ErrorResponseBase& InError);

    // Transport access
    // TODO: @HalcyonOmega Consider converting to reference
    ITransport* GetTransport() const {
        return m_Transport.get();
    }

  protected:
    virtual void OnInitializeRequest(const InitializeRequest& InRequest,
                                     const std::string& InRequestID) = 0;
    virtual void OnInitializedNotification() = 0;
    virtual MCPTask_Void HandleRequest(const RequestBase& InRequest);
    virtual void HandleResponse(const ResponseBase& InResponse);
    virtual void HandleNotification(const NotificationBase& InNotification);
    virtual void HandleError(const ErrorResponseBase& InError);

    void SetState(MCPProtocolState InNewState);

    MCPProtocolState m_State;
    std::unique_ptr<ITransport> m_Transport;

  private:
    void SetupTransportHandlers();
    void OnTransportMessage(const MessageBase& InMessage);
    void OnTransportRequest(const RequestBase& InRequest);
    void OnTransportResponse(const ResponseBase& InResponse);
    void OnTransportNotification(const NotificationBase& InNotification);
    void OnTransportErrorResponse(const ErrorResponseBase& InError);
    void OnTransportStateChange(TransportState InOldState, TransportState InNewState);

    // Request tracking
    struct PendingResponse {
        std::string RequestID;
        std::promise<JSONValue> Promise;
        std::chrono::steady_clock::time_point StartTime;
    };

    std::unordered_map<std::string, std::unique_ptr<PendingResponse>> m_PendingResponses;
    mutable std::mutex m_ResponsesMutex;
};

MCP_NAMESPACE_END