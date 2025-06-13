#pragma once

#include "Core.h"
#include "Core/Protocol__DT.hpp"
#include "SchemaAliases.h"

MCP_NAMESPACE_BEGIN

struct ServerOptions : public ProtocolOptions {
    optional<ServerCapabilities>
        Capabilities; // Capabilities to advertise as being supported by this server.
    optional<string>
        Instructions; // Optional instructions describing how to use the server and its features.
};

/**
 * An MCP server on top of a pluggable transport.
 *
 * This server will automatically respond to the initialization flow as initiated from the client.
 *
 * To use with custom types, extend the base Request/Notification/Result types and pass them as type
 * parameters.
 */
class Server : public Protocol<ServerRequest, ServerNotification, ServerResult> {
  private:
    optional<ClientCapabilities> m_ClientCapabilities;
    optional<Implementation> m_ClientVersion;
    ServerCapabilities m_Capabilities;
    optional<string> m_Instructions;
    Implementation m_ServerInfo;

  public:
    /**
     * Callback for when initialization has fully completed (i.e., the client has sent an
     * `initialized` notification).
     */
    optional<function<void()>> OnInitialized;

    // Initializes this server with the given name and version information.
    Server(const Implementation& InServerInfo, const optional<ServerOptions>& InOptions = nullopt)
        : Protocol<ServerRequest, ServerNotification, ServerResult>(), m_ServerInfo(InServerInfo) {
        if (InOptions) {
            m_Capabilities = InOptions->Capabilities.value_or(ServerCapabilities{});
            m_Instructions = InOptions->Instructions;
        }

        // Set request handler for InitializeRequestSchema
        this->SetRequestHandler(InitializeRequestSchema{}, [this](const JSON& InRequest) {
            // TODO: Parse request into InitializeRequest
            InitializeRequest InitializeRequest; // Would parse from JSON
            return this->OnInitialize(InitializeRequest);
        });

        // Set notification handler for InitializedNotificationSchema
        this->SetNotificationHandler(InitializedNotificationSchema{}, [this]() {
            if (this->OnInitialized) { (*this->OnInitialized)(); }
        });
    }

    // Registers new capabilities. This can only be called before connecting to a transport.
    //
    // The new capabilities will be merged with any existing capabilities previously given (e.g., at
    // initialization).
    void RegisterCapabilities(const ServerCapabilities& InCapabilities);

  protected:
    void AssertCapabilityForMethod(const string& InMethod);

    void AssertNotificationCapability(const string& InMethod);

    void AssertRequestHandlerCapability(const string& InMethod);

  private:
    future<JSON> OnInitialize(const InitializeRequest& InRequest);

  public:
    /**
     * After initialization has completed, this will be populated with the client's reported
     * capabilities.
     */
    optional<ClientCapabilities> GetClientCapabilities() const;

    /**
     * After initialization has completed, this will be populated with information about the
     * client's name and version.
     */
    optional<Implementation> GetClientVersion() const;

  private:
    ServerCapabilities GetCapabilities() const;

  public:
    future<JSON> Ping();

    future<JSON> CreateMessage(const JSON& InParams,
                               const optional<RequestOptions>& InOptions = nullopt);

    future<JSON> ListRoots(const optional<JSON>& InParams = nullopt,
                           const optional<RequestOptions>& InOptions = nullopt);

    future<void> SendLoggingMessage(const JSON& InParams);

    future<void> SendResourceUpdated(const JSON& InParams);

    future<void> SendResourceListChanged();

    future<void> SendToolListChanged();

    future<void> SendPromptListChanged();
};

MCP_NAMESPACE_END
