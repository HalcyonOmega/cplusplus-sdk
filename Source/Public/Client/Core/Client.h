#pragma once

#include <future>

#include "Communication/Transport/Transport.h"
#include "Core.h"
#include "Core/Features/Resource/Resources.h"
#include "Core/Features/Tool/Tools.h"
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
    // Initializes this client with the given name and version information.
    Client(const Implementation& ClientInfo, const optional<ClientOptions>& Options = nullopt);

    // Registers new capabilities. This can only be called before connecting to a transport.
    //
    // The new capabilities will be merged with any existing capabilities previously given (e.g., at
    // initialization).
    void RegisterCapabilities(const ClientCapabilities& Capabilities);

  protected:
    void AssertCapability(const string& Capability, const string& Method);
    void AssertCapabilityForMethod(const string& Method);
    void AssertNotificationCapability(const string& Method);
    void AssertRequestHandlerCapability(const string& Method);

  public:
    future<void> Connect(shared_ptr<Transport> TransportPtr,
                         const optional<RequestOptions>& Options = nullopt) override;

    // After initialization has completed, this will be populated with the server's reported
    // capabilities.
    [[nodiscard]] optional<ServerCapabilities> GetServerCapabilities() const;

    // After initialization has completed, this will be populated with information about the
    // server's name and version.
    [[nodiscard]] optional<Implementation> GetServerVersion() const;

    // After initialization has completed, this may be populated with information about the server's
    // instructions.
    [[nodiscard]] optional<string> GetInstructions() const;

    // Client Methods
    future<void> Ping(const optional<RequestOptions>& Options = nullopt);
    future<CompleteResult> Complete(const CompleteRequest::CompleteRequestParams& Params,
                                    const optional<RequestOptions>& Options = nullopt);
    void SetLoggingLevel(LoggingLevel Level, const optional<RequestOptions>& Options = nullopt);
    future<GetPromptResult> GetPrompt(const GetPromptRequest::GetPromptRequestParams& Params,
                                      const optional<RequestOptions>& Options = nullopt);
    future<ListPromptsResult> ListPrompts(const optional<ListPromptsRequest>& Params = nullopt,
                                          const optional<RequestOptions>& Options = nullopt);
    future<ListResourcesResult>
    ListResources(const optional<ListResourcesRequest>& Params = nullopt,
                  const optional<RequestOptions>& Options = nullopt);
    future<ListResourceTemplatesResult>
    ListResourceTemplates(const optional<ListResourceTemplatesRequest>& Params = nullopt,
                          const optional<RequestOptions>& Options = nullopt);
    future<ReadResourceResult>
    ReadResource(const ReadResourceRequest::ReadResourceRequestParams& Params,
                 const optional<RequestOptions>& Options = nullopt);
    future<void> SubscribeResource(const SubscribeRequest::SubscribeRequestParams& Params,
                                   const optional<RequestOptions>& Options = nullopt);
    future<void> UnsubscribeResource(const UnsubscribeRequest::UnsubscribeRequestParams& Params,
                                     const optional<RequestOptions>& Options = nullopt);
    future<CallToolResult> CallTool(const CallToolRequest::CallToolRequestParams& Params,
                                    const string& ResultSchema = "CallToolResultSchema",
                                    const optional<RequestOptions>& Options = nullopt);
    future<ListToolsResult> ListTools(const optional<ListToolsRequest>& Params = nullopt,
                                      const optional<RequestOptions>& Options = nullopt);
    void SendRootsListChanged();

  private:
    void CacheToolOutputSchemas(const vector<Tool>& Tools);
    optional<ValidateFunction> GetToolOutputValidator(const string& ToolName);
};

MCP_NAMESPACE_END
