#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// Forward declarations - these should exist in MCP namespace
struct ProtocolOptions;
struct RequestOptions;
struct ClientCapabilities;
struct ServerCapabilities;
struct Implementation;
struct Request;
struct Notification;
struct Result;
struct ServerRequest;
struct ServerNotification;
struct ServerResult;
struct InitializeRequest;
struct InitializeResult;
struct CreateMessageRequest;
struct ListRootsRequest;
struct LoggingMessageNotification;
struct ResourceUpdatedNotification;
struct CreateMessageResultSchema;
struct EmptyResultSchema;
struct InitializedNotificationSchema;
struct InitializeRequestSchema;
struct ListRootsResultSchema;
class Protocol;

struct ServerOptions : public ProtocolOptions {
    /**
     * Capabilities to advertise as being supported by this server.
     */
    optional<ServerCapabilities> Capabilities;

    /**
     * Optional instructions describing how to use the server and its features.
     */
    optional<string> Instructions;
};

/**
 * An MCP server on top of a pluggable transport.
 *
 * This server will automatically respond to the initialization flow as initiated from the client.
 *
 * To use with custom types, extend the base Request/Notification/Result types and pass them as type
 * parameters.
 */
template <typename RequestT = Request, typename NotificationT = Notification,
          typename ResultT = Result>
class Server
    : public Protocol<ServerRequest,      // TODO: Should be ServerRequest | RequestT in TypeScript
                      ServerNotification, // TODO: Should be ServerNotification | NotificationT in
                                          // TypeScript
                      ServerResult        // TODO: Should be ServerResult | ResultT in TypeScript
                      > {
  private:
    optional<ClientCapabilities> ClientCapabilities;
    optional<Implementation> ClientVersion;
    ServerCapabilities Capabilities;
    optional<string> Instructions;
    Implementation ServerInfo;

  public:
    /**
     * Callback for when initialization has fully completed (i.e., the client has sent an
     * `initialized` notification).
     */
    optional<function<void()>> OnInitialized;

    // Initializes this server with the given name and version information.
    Server(const Implementation& serverInfo, const optional<ServerOptions>& options = nullopt)
        : Protocol<ServerRequest, ServerNotification, ServerResult>(options),
          ServerInfo(serverInfo) {
        if (options) {
            Capabilities_ = options->capabilities.value_or(ServerCapabilities{});
            Instructions_ = options->instructions;
        }

        // Set request handler for InitializeRequestSchema
        this->SetRequestHandler(InitializeRequestSchema{}, [this](const JSON& request) {
            // TODO: Parse request into InitializeRequest
            InitializeRequest initRequest; // Would parse from JSON
            return this->OnInitialize(initRequest);
        });

        // Set notification handler for InitializedNotificationSchema
        this->SetNotificationHandler(InitializedNotificationSchema{}, [this]() {
            if (this->OnInitialized) {
                (*this->OnInitialized)();
            }
        });
    }

    // Registers new capabilities. This can only be called before connecting to a transport.
    //
    // The new capabilities will be merged with any existing capabilities previously given (e.g., at
    // initialization).
    void RegisterCapabilities(const ServerCapabilities& capabilities);

  protected:
    void AssertCapabilityForMethod(const string& method);

    void AssertNotificationCapability(const string& method);

    void AssertRequestHandlerCapability(const string& method);

  private:
    future<JSON> OnInitialize(const InitializeRequest& request);

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

    future<JSON> CreateMessage(const JSON& params,
                               const optional<RequestOptions>& options = nullopt);

    future<JSON> ListRoots(const optional<JSON>& params = nullopt,
                           const optional<RequestOptions>& options = nullopt);

    future<void> SendLoggingMessage(const JSON& params);

    future<void> SendResourceUpdated(const JSON& params);

    future<void> SendResourceListChanged();

    future<void> SendToolListChanged();

    future<void> SendPromptListChanged();
};

MCP_NAMESPACE_END
