#pragma once

#include <chrono>
#include <future>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Messages/MCPMessages.h"
#include "CoreSDK/Messages/MessageManager.h"
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
	virtual ~MCPProtocol() = default;

    // Lifecycle
    virtual MCPTask_Void Start() = 0;
    virtual MCPTask_Void Stop() = 0;
    bool IsInitialized() const;
    MCPProtocolState GetState() const;
    void SetState(MCPProtocolState InNewState);
    ITransport* GetTransport() const; // TODO: @HalcyonOmega Consider converting to reference
    bool IsConnected() const;

    // Core protocol operations
    MCPTask<PingResponse> Ping(const PingRequest& InRequest);

    // Protocol version validation
    static const std::vector<std::string> SUPPORTED_PROTOCOL_VERSIONS;
	static void ValidateProtocolVersion(const std::string& InVersion);

    // Message sending utilities
    MCPTask_Void SendRequest(const RequestBase& InRequest);
    MCPTask_Void SendResponse(const ResponseBase& InResponse) const;
    MCPTask_Void SendNotification(const NotificationBase& InNotification) const;
    MCPTask_Void SendErrorResponse(const ErrorResponseBase& InError) const;

  private:
    void SetupTransportRouter() const;

  protected:
    MCPProtocolState m_State;
    std::unique_ptr<ITransport> m_Transport;
    std::unique_ptr<MessageManager> m_MessageManager;

  private:
    // Request tracking
    struct PendingResponse {
        RequestID RequestID;
        std::promise<JSONData> Promise;
        std::chrono::steady_clock::time_point StartTime;
    };

    std::unordered_map<std::string, std::unique_ptr<PendingResponse>> m_PendingResponses;
    mutable std::mutex m_ResponsesMutex;

  protected:
    Implementation m_ServerInfo;
    Implementation m_ClientInfo;
    ServerCapabilities m_ServerCapabilities;
    ClientCapabilities m_ClientCapabilities;

  public:
    void SetServerInfo(const Implementation& InServerInfo) {
        m_ServerInfo = InServerInfo;
    }
    void SetClientInfo(const Implementation& InClientInfo) {
        m_ClientInfo = InClientInfo;
    }
    void SetServerCapabilities(const ServerCapabilities& InServerCapabilities) {
        m_ServerCapabilities = InServerCapabilities;
    }
    void SetClientCapabilities(const ClientCapabilities& InClientCapabilities) {
        m_ClientCapabilities = InClientCapabilities;
    }

    Implementation GetServerInfo() const {
        return m_ServerInfo;
    }
    Implementation GetClientInfo() const {
        return m_ClientInfo;
    }
    ServerCapabilities GetServerCapabilities() const {
        return m_ServerCapabilities;
    }
    ClientCapabilities GetClientCapabilities() const {
        return m_ClientCapabilities;
    }
};

MCP_NAMESPACE_END
