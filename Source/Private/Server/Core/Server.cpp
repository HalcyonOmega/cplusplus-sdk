#include "Server/Core/Server.h"

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

/**
 * Initializes this server with the given name and version information.
 */
Server(const Implementation& serverInfo, const optional<ServerOptions>& options = nullopt)
    : Protocol<ServerRequest, ServerNotification, ServerResult>(options), ServerInfo_(serverInfo) {
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
        if (this->OnInitialized) { (*this->OnInitialized)(); }
    });

    /**
     * Registers new capabilities. This can only be called before connecting to a transport.
     *
     * The new capabilities will be merged with any existing capabilities previously given (e.g., at
     * initialization).
     */
    void Server::RegisterCapabilities(const ServerCapabilities& capabilities) {
        if (this->transport) {
            throw runtime_error("Cannot register capabilities after connecting to transport");
        }

        Capabilities_ = MergeCapabilities(Capabilities_, capabilities);
    }

    void Server::AssertCapabilityForMethod(const string& method) {
        if (method == MTHD_SAMPLING_CREATE_MESSAGE) {
            if (!ClientCapabilities_
                || !ClientCapabilities_->sampling) { // TODO: Access sampling field
                throw runtime_error("Client does not support sampling (required for " + method
                                    + ")");
            }
        } else if (method == MTHD_ROOTS_LIST) {
            if (!ClientCapabilities_ || !ClientCapabilities_->roots) { // TODO: Access roots field
                throw runtime_error("Client does not support listing roots (required for " + method
                                    + ")");
            }
        } else if (method == MTHD_PING) {
            // No specific capability required for ping
        }
    }

    void Server::AssertNotificationCapability(const string& method) {
        if (method == MTHD_NOTIFICATIONS_MESSAGE) {
            if (!Capabilities_.logging) { // TODO: Access logging field
                throw runtime_error("Server does not support logging (required for " + method
                                    + ")");
            }
        } else if (method == MTHD_NOTIFICATIONS_RESOURCES_UPDATED
                   || method == MTHD_NOTIFICATIONS_RESOURCES_LIST_CHANGED) {
            if (!Capabilities_.resources) { // TODO: Access resources field
                throw runtime_error(
                    "Server does not support notifying about resources (required for " + method
                    + ")");
            }
        } else if (method == MTHD_NOTIFICATIONS_TOOLS_LIST_CHANGED) {
            if (!Capabilities_.tools) { // TODO: Access tools field
                throw runtime_error(
                    "Server does not support notifying of tool list changes (required for " + method
                    + ")");
            }
        } else if (method == MTHD_NOTIFICATIONS_PROMPTS_LIST_CHANGED) {
            if (!Capabilities_.prompts) { // TODO: Access prompts field
                throw runtime_error(
                    "Server does not support notifying of prompt list changes (required for "
                    + method + ")");
            }
        } else if (method == MTHD_NOTIFICATION_CANCELLED || method == MTHD_NOTIFICATION_PROGRESS) {
            // Cancellation and progress notifications are always allowed
        }
    }

    void Server::AssertRequestHandlerCapability(const string& method) {
        if (method == MTHD_SAMPLING_CREATE_MESSAGE) {
            if (!Capabilities_.sampling) { // TODO: Access sampling field
                throw runtime_error("Server does not support sampling (required for " + method
                                    + ")");
            }
        } else if (method == MTHD_LOGGING_SET_LEVEL) {
            if (!Capabilities_.logging) { // TODO: Access logging field
                throw runtime_error("Server does not support logging (required for " + method
                                    + ")");
            }
        } else if (method == MTHD_PROMPTS_GET || method == MTHD_PROMPTS_LIST) {
            if (!Capabilities_.prompts) { // TODO: Access prompts field
                throw runtime_error("Server does not support prompts (required for " + method
                                    + ")");
            }
        } else if (method == MTHD_RESOURCES_LIST || method == MTHD_RESOURCES_TEMPLATES_LIST
                   || method == MTHD_RESOURCES_READ) {
            if (!Capabilities_.resources) { // TODO: Access resources field
                throw runtime_error("Server does not support resources (required for " + method
                                    + ")");
            }
        } else if (method == MTHD_TOOLS_CALL || method == MTHD_TOOLS_LIST) {
            if (!Capabilities_.tools) { // TODO: Access tools field
                throw runtime_error("Server does not support tools (required for " + method + ")");
            }
        } else if (method == MTHD_PING || method == MTHD_INITIALIZE) {
            // No specific capability required for these methods
        }
    }

    future<JSON> Server::OnInitialize(const InitializeRequest& request) {
        const string& requestedVersion =
            request.params.protocolVersion; // TODO: Access protocolVersion field

        ClientCapabilities_ = request.params.capabilities; // TODO: Access capabilities field
        ClientVersion_ = request.params.clientInfo;        // TODO: Access clientInfo field

        InitializeResult result; // TODO: Create InitializeResult

        // Check if requested version is supported
        bool versionSupported = false;
        for (const auto& version : SUPPORTED_PROTOCOL_VERSIONS) {
            if (version == requestedVersion) {
                versionSupported = true;
                break;
            }
        }

        result.protocolVersion =
            versionSupported ? requestedVersion
                             : MCP_LATEST_PROTOCOL_VERSION; // TODO: Set protocolVersion field
        result.capabilities = GetCapabilities();            // TODO: Set capabilities field
        result.serverInfo = ServerInfo_;                    // TODO: Set serverInfo field

        if (Instructions_) {
            result.instructions = Instructions_; // TODO: Set instructions field
        }

        // TODO: Convert InitializeResult to JSON
        return async(launch::deferred, []() { return JSON{}; });
    }

    /**
     * After initialization has completed, this will be populated with the client's reported
     * capabilities.
     */
    optional<ClientCapabilities> Server::GetClientCapabilities() const {
        return ClientCapabilities_;
    }

    /**
     * After initialization has completed, this will be populated with information about the
     * client's name and version.
     */
    optional<Implementation> Server::GetClientVersion() const {
        return ClientVersion_;
    }

    ServerCapabilities Server::GetCapabilities() const {
        return Capabilities_;
    }

    future<JSON> Server::Ping() {
        JSON request;
        request[MSG_METHOD] = MTHD_PING;
        return this->Request(request, EmptyResultSchema{});
    }

    future<JSON> Server::CreateMessage(const JSON& params,
                                       const optional<RequestOptions>& options = nullopt) {
        JSON request;
        request[MSG_METHOD] = MTHD_SAMPLING_CREATE_MESSAGE;
        request[MSG_PARAMS] = params;
        return this->Request(request, CreateMessageResultSchema{}, options);
    }

    future<JSON> Server::ListRoots(const optional<JSON>& params = nullopt,
                                   const optional<RequestOptions>& options = nullopt) {
        JSON request;
        request[MSG_METHOD] = MTHD_ROOTS_LIST;
        if (params) { request[MSG_PARAMS] = *params; }
        return this->Request(request, ListRootsResultSchema{}, options);
    }

    future<void> Server::SendLoggingMessage(const JSON& params) {
        JSON notification;
        notification[MSG_METHOD] = MTHD_NOTIFICATIONS_MESSAGE;
        notification[MSG_PARAMS] = params;
        return this->Notification(notification);
    }

    future<void> Server::SendResourceUpdated(const JSON& params) {
        JSON notification;
        notification[MSG_METHOD] = MTHD_NOTIFICATIONS_RESOURCES_UPDATED;
        notification[MSG_PARAMS] = params;
        return this->Notification(notification);
    }

    future<void> Server::SendResourceListChanged() {
        JSON notification;
        notification[MSG_METHOD] = MTHD_NOTIFICATIONS_RESOURCES_LIST_CHANGED;
        return this->Notification(notification);
    }

    future<void> Server::SendToolListChanged() {
        JSON notification;
        notification[MSG_METHOD] = MTHD_NOTIFICATIONS_TOOLS_LIST_CHANGED;
        return this->Notification(notification);
    }

    future<void> Server::SendPromptListChanged() {
        JSON notification;
        notification[MSG_METHOD] = MTHD_NOTIFICATIONS_PROMPTS_LIST_CHANGED;
        return this->Notification(notification);
    }
};

MCP_NAMESPACE_END
