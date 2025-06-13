#pragma once

#include "Communication/Transport/Transport.h"
#include "Core.h"
#include "Core/Messages/MessagesIncludes.h"
#include "Schemas/Client/ClientSchemas.h"
#include "Schemas/Common/CommonSchemas.h"
#include "Schemas/Common/InitializeSchemas.h"
#include "Schemas/Server/ServerSchemas.h"

MCP_NAMESPACE_BEGIN

// Enum to represent the session state
enum class SessionState { Uninitialized, Initializing, Initialized, ShuttingDown, Shutdown, Error };

class Session {
  public:
    Session(std::shared_ptr<Transport> Transport, const ClientCapabilities& ClientCaps,
            const Implementation& ClientInfo);
    ~Session();

    void Initialize(std::function<void(const std::optional<ErrorBase>&)> Callback);
    void Shutdown();

    SessionState GetState() const;
    const ClientCapabilities& GetClientCapabilities() const;
    const std::optional<ServerCapabilities>& GetServerCapabilities() const;
    const Implementation& GetClientInfo() const;
    const std::optional<Implementation>& GetServerInfo() const;
    const std::optional<std::string>& GetNegotiatedProtocolVersion() const;

  private:
    void HandleTransportMessage(const std::string& InMessage);
    void ProcessInitializeResult(const InitializeResult& InResult);
    void SendInitializedNotification();

    std::shared_ptr<Transport> m_Transport;
    ClientCapabilities m_ClientCapabilities;
    Implementation m_ClientInfo;

    std::optional<ServerCapabilities> m_ServerCapabilities;
    std::optional<Implementation> m_ServerInfo;
    std::optional<std::string> m_NegotiatedProtocolVersion;

    SessionState m_State;
    std::function<void(const std::optional<ErrorBase>&)> m_InitializeCallback;
    // TODO: Add request ID management
    // TODO: Add JSON serialization/deserialization (using JSON alias from Core.h)
    // TODO: Add methods for sending requests and handling responses/notifications
};

MCP_NAMESPACE_END
