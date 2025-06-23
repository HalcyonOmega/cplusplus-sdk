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
    MCPTask<JSONValue> Ping();

    // Protocol version validation
    static const std::vector<std::string> SUPPORTED_PROTOCOL_VERSIONS;
    void ValidateProtocolVersion(const std::string& InVersion) const;

    // Message sending utilities
    template <typename TRequest>
    MCPTask<typename TRequest::ResponseType::ResultType> SendRequest(const TRequest& InRequest);

    template <typename TResponse>
    MCPTask_Void SendResponse(const std::string& InRequestID, const TResponse& InResponse);

    template <typename TNotification>
    MCPTask_Void SendNotification(const TNotification& InNotification);

    // Error handling
    MCPTask_Void SendError(const std::string& InRequestID, int64_t InCode,
                           const std::string& InMessage, const JSONValue& InData = {});

    // Event handlers
    using RequestHandler = std::function<MCPTask_Void(
        const std::string& InMethod, const JSONValue& InParams, const std::string& InRequestID)>;
    using ResponseHandler =
        std::function<void(const JSONValue& InResult, const std::string& InRequestID)>;
    using NotificationHandler =
        std::function<void(const std::string& InMethod, const JSONValue& InParams)>;
    using ErrorHandler = std::function<void(const std::string& InError)>;

    void SetRequestHandler(RequestHandler InHandler) {
        m_RequestHandler = InHandler;
    }
    void SetResponseHandler(ResponseHandler InHandler) {
        m_ResponseHandler = InHandler;
    }
    void SetNotificationHandler(NotificationHandler InHandler) {
        m_NotificationHandler = InHandler;
    }
    void SetErrorHandler(ErrorHandler InHandler) {
        m_ErrorHandler = InHandler;
    }

    // Transport access
    ITransport* GetTransport() const {
        return m_Transport.get();
    }

  protected:
    virtual void OnInitializeRequest(const InitializeRequest& InRequest,
                                     const std::string& InRequestID) = 0;
    virtual void OnInitializedNotification() = 0;
    virtual MCPTask_Void HandleRequest(const std::string& InMethod, const JSONValue& InParams,
                                       const std::string& InRequestID);
    virtual void HandleResponse(const JSONValue& InResult, const std::string& InRequestID);
    virtual void HandleNotification(const std::string& InMethod, const JSONValue& InParams);
    virtual void HandleError(const std::string& InError);

    void SetState(MCPProtocolState InNewState);

    MCPProtocolState m_State;
    std::unique_ptr<ITransport> m_Transport;

  private:
    void SetupTransportHandlers();
    void OnTransportMessage(const std::string& InRawMessage);
    void OnTransportRequest(const std::string& InMethod, const JSONValue& InParams,
                            const std::string& InRequestID);
    void OnTransportResponse(const JSONValue& InResult, const std::string& InRequestID);
    void OnTransportNotification(const std::string& InMethod, const JSONValue& InParams);
    void OnTransportError(const std::string& InError);
    void OnTransportStateChange(TransportState InOldState, TransportState InNewState);

    // Event handlers
    RequestHandler m_RequestHandler;
    ResponseHandler m_ResponseHandler;
    NotificationHandler m_NotificationHandler;
    ErrorHandler m_ErrorHandler;

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