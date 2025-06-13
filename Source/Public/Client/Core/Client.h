#pragma once

#include <future>

#include "Communication/Transport/Transport.h"
#include "Core.h"
#include "Core/Protocol__DT.hpp"
#include "SchemaAliases.h"

MCP_NAMESPACE_BEGIN

struct ClientOptions {
    optional<ClientCapabilities> Capabilities;
    // TODO: Add other ProtocolOptions fields
};

/**
 * An MCP client on top of a pluggable transport.
 *
 * The client will automatically begin the initialization flow with the server when Connect() is
 * called.
 *
 * To use with custom types, extend the base Request/Notification/Result types and pass them as type
 * parameters.
 */
class Client : public Protocol<ClientRequest, ClientNotification, ClientResult> {
  private:
    optional<ServerCapabilities> m_ServerCapabilities;
    optional<Implementation> m_ServerVersion;
    ClientCapabilities m_Capabilities;
    optional<string> m_Instructions;
    unordered_map<string, ValidateFunction> m_CachedToolOutputValidators;
    Implementation m_ClientInfo;

  public:
    /**
     * Initializes this client with the given name and version information.
     */
    Client(const Implementation& ClientInfo, const optional<ClientOptions>& Options = nullopt);

    /**
     * Registers new capabilities. This can only be called before connecting to a transport.
     *
     * The new capabilities will be merged with any existing capabilities previously given (e.g., at
     * initialization).
     */
    void RegisterCapabilities(const ClientCapabilities& Capabilities);

  protected:
    void AssertCapability(const string& Capability, const string& Method);
    void AssertCapabilityForMethod(const string& Method);
    void AssertNotificationCapability(const string& Method);
    void AssertRequestHandlerCapability(const string& Method);

  public:
    future<void> Connect(shared_ptr<Transport> TransportPtr,
                         const optional<RequestOptions>& Options = nullopt) override;

    /**
     * After initialization has completed, this will be populated with the server's reported
     * capabilities.
     */
    [[nodiscard]] optional<ServerCapabilities> GetServerCapabilities() const;

    /**
     * After initialization has completed, this will be populated with information about the
     * server's name and version.
     */
    [[nodiscard]] optional<Implementation> GetServerVersion() const;

    /**
     * After initialization has completed, this may be populated with information about the server's
     * instructions.
     */
    [[nodiscard]] optional<string> GetInstructions() const;

    // Client Methods
    future<JSON> Ping(const optional<RequestOptions>& Options = nullopt);
    future<JSON> Complete(const JSON& Params, const optional<RequestOptions>& Options = nullopt);
    future<JSON> SetLoggingLevel(LoggingLevel Level,
                                 const optional<RequestOptions>& Options = nullopt);
    future<JSON> GetPrompt(const JSON& Params, const optional<RequestOptions>& Options = nullopt);
    future<JSON> ListPrompts(const optional<JSON>& Params = nullopt,
                             const optional<RequestOptions>& Options = nullopt);
    future<JSON> ListResources(const optional<JSON>& Params = nullopt,
                               const optional<RequestOptions>& Options = nullopt);
    future<JSON> ListResourceTemplates(const optional<JSON>& Params = nullopt,
                                       const optional<RequestOptions>& Options = nullopt);
    future<JSON> ReadResource(const JSON& Params,
                              const optional<RequestOptions>& Options = nullopt);
    future<JSON> SubscribeResource(const JSON& Params,
                                   const optional<RequestOptions>& Options = nullopt);
    future<JSON> UnsubscribeResource(const JSON& Params,
                                     const optional<RequestOptions>& Options = nullopt);
    future<JSON> CallTool(const JSON& Params, const string& ResultSchema = "CallToolResultSchema",
                          const optional<RequestOptions>& Options = nullopt);
    future<JSON> ListTools(const optional<JSON>& Params = nullopt,
                           const optional<RequestOptions>& Options = nullopt);
    void SendRootsListChanged();

  private:
    void CacheToolOutputSchemas(const vector<Tool>& Tools);
    optional<ValidateFunction> GetToolOutputValidator(const string& ToolName);
};

MCP_NAMESPACE_END
