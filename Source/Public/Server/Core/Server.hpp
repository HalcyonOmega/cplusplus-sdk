#pragma once

#include "../Core/Common.hpp"

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

struct ServerOptions : public ProtocolOptions {
    /**
     * Capabilities to advertise as being supported by this server.
     */
    optional<ServerCapabilities> capabilities;

    /**
     * Optional instructions describing how to use the server and its features.
     */
    optional<string> instructions;
};

/**
 * An MCP server on top of a pluggable transport.
 *
 * This server will automatically respond to the initialization flow as initiated from the client.
 *
 * To use with custom types, extend the base Request/Notification/Result types and pass them as type parameters.
 */
template<
    typename RequestT = Request,
    typename NotificationT = Notification,
    typename ResultT = Result
>
class Server : public Protocol<
    ServerRequest, // TODO: Should be ServerRequest | RequestT in TypeScript
    ServerNotification, // TODO: Should be ServerNotification | NotificationT in TypeScript
    ServerResult // TODO: Should be ServerResult | ResultT in TypeScript
> {
private:
    optional<ClientCapabilities> ClientCapabilities_;
    optional<Implementation> ClientVersion_;
    ServerCapabilities Capabilities_;
    optional<string> Instructions_;
    Implementation ServerInfo_;

public:
    /**
     * Callback for when initialization has fully completed (i.e., the client has sent an `initialized` notification).
     */
    optional<function<void()>> OnInitialized;

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
            if (this->OnInitialized) {
                (*this->OnInitialized)();
            }
        });
    }

    /**
     * Registers new capabilities. This can only be called before connecting to a transport.
     *
     * The new capabilities will be merged with any existing capabilities previously given (e.g., at initialization).
     */
    void RegisterCapabilities(const ServerCapabilities& capabilities) {
        if (this->transport) {
            throw runtime_error(
                "Cannot register capabilities after connecting to transport"
            );
        }

        Capabilities_ = MergeCapabilities(Capabilities_, capabilities);
    }

protected:
    void AssertCapabilityForMethod(const string& method) {
        if (method == "sampling/createMessage") {
            if (!ClientCapabilities_ || !ClientCapabilities_->sampling) { // TODO: Access sampling field
                throw runtime_error(
                    "Client does not support sampling (required for " + method + ")"
                );
            }
        }
        else if (method == "roots/list") {
            if (!ClientCapabilities_ || !ClientCapabilities_->roots) { // TODO: Access roots field
                throw runtime_error(
                    "Client does not support listing roots (required for " + method + ")"
                );
            }
        }
        else if (method == "ping") {
            // No specific capability required for ping
        }
    }

    void AssertNotificationCapability(const string& method) {
        if (method == "notifications/message") {
            if (!Capabilities_.logging) { // TODO: Access logging field
                throw runtime_error(
                    "Server does not support logging (required for " + method + ")"
                );
            }
        }
        else if (method == "notifications/resources/updated" ||
                 method == "notifications/resources/list_changed") {
            if (!Capabilities_.resources) { // TODO: Access resources field
                throw runtime_error(
                    "Server does not support notifying about resources (required for " + method + ")"
                );
            }
        }
        else if (method == "notifications/tools/list_changed") {
            if (!Capabilities_.tools) { // TODO: Access tools field
                throw runtime_error(
                    "Server does not support notifying of tool list changes (required for " + method + ")"
                );
            }
        }
        else if (method == "notifications/prompts/list_changed") {
            if (!Capabilities_.prompts) { // TODO: Access prompts field
                throw runtime_error(
                    "Server does not support notifying of prompt list changes (required for " + method + ")"
                );
            }
        }
        else if (method == "notifications/cancelled" ||
                 method == "notifications/progress") {
            // Cancellation and progress notifications are always allowed
        }
    }

    void AssertRequestHandlerCapability(const string& method) {
        if (method == "sampling/createMessage") {
            if (!Capabilities_.sampling) { // TODO: Access sampling field
                throw runtime_error(
                    "Server does not support sampling (required for " + method + ")"
                );
            }
        }
        else if (method == "logging/setLevel") {
            if (!Capabilities_.logging) { // TODO: Access logging field
                throw runtime_error(
                    "Server does not support logging (required for " + method + ")"
                );
            }
        }
        else if (method == "prompts/get" || method == "prompts/list") {
            if (!Capabilities_.prompts) { // TODO: Access prompts field
                throw runtime_error(
                    "Server does not support prompts (required for " + method + ")"
                );
            }
        }
        else if (method == "resources/list" ||
                 method == "resources/templates/list" ||
                 method == "resources/read") {
            if (!Capabilities_.resources) { // TODO: Access resources field
                throw runtime_error(
                    "Server does not support resources (required for " + method + ")"
                );
            }
        }
        else if (method == "tools/call" || method == "tools/list") {
            if (!Capabilities_.tools) { // TODO: Access tools field
                throw runtime_error(
                    "Server does not support tools (required for " + method + ")"
                );
            }
        }
        else if (method == "ping" || method == "initialize") {
            // No specific capability required for these methods
        }
    }

private:
    future<JSON> OnInitialize(const InitializeRequest& request) {
        const string& requestedVersion = request.params.protocolVersion; // TODO: Access protocolVersion field

        ClientCapabilities_ = request.params.capabilities; // TODO: Access capabilities field
        ClientVersion_ = request.params.clientInfo; // TODO: Access clientInfo field

        InitializeResult result; // TODO: Create InitializeResult

        // Check if requested version is supported
        bool versionSupported = false;
        for (const auto& version : SUPPORTED_PROTOCOL_VERSIONS) {
            if (version == requestedVersion) {
                versionSupported = true;
                break;
            }
        }

        result.protocolVersion = versionSupported ? requestedVersion : LATEST_PROTOCOL_VERSION; // TODO: Set protocolVersion field
        result.capabilities = GetCapabilities(); // TODO: Set capabilities field
        result.serverInfo = ServerInfo_; // TODO: Set serverInfo field

        if (Instructions_) {
            result.instructions = Instructions_; // TODO: Set instructions field
        }

        // TODO: Convert InitializeResult to JSON
        return async(launch::deferred, []() { return JSON{}; });
    }

public:
    /**
     * After initialization has completed, this will be populated with the client's reported capabilities.
     */
    optional<ClientCapabilities> GetClientCapabilities() const {
        return ClientCapabilities_;
    }

    /**
     * After initialization has completed, this will be populated with information about the client's name and version.
     */
    optional<Implementation> GetClientVersion() const {
        return ClientVersion_;
    }

private:
    ServerCapabilities GetCapabilities() const {
        return Capabilities_;
    }

public:
    future<JSON> Ping() {
        JSON request;
        request["method"] = "ping";
        return this->Request(request, EmptyResultSchema{});
    }

    future<JSON> CreateMessage(const JSON& params, const optional<RequestOptions>& options = nullopt) {
        JSON request;
        request["method"] = "sampling/createMessage";
        request["params"] = params;
        return this->Request(request, CreateMessageResultSchema{}, options);
    }

    future<JSON> ListRoots(const optional<JSON>& params = nullopt, const optional<RequestOptions>& options = nullopt) {
        JSON request;
        request["method"] = "roots/list";
        if (params) {
            request["params"] = *params;
        }
        return this->Request(request, ListRootsResultSchema{}, options);
    }

    future<void> SendLoggingMessage(const JSON& params) {
        JSON notification;
        notification["method"] = "notifications/message";
        notification["params"] = params;
        return this->Notification(notification);
    }

    future<void> SendResourceUpdated(const JSON& params) {
        JSON notification;
        notification["method"] = "notifications/resources/updated";
        notification["params"] = params;
        return this->Notification(notification);
    }

    future<void> SendResourceListChanged() {
        JSON notification;
        notification["method"] = "notifications/resources/list_changed";
        return this->Notification(notification);
    }

    future<void> SendToolListChanged() {
        JSON notification;
        notification["method"] = "notifications/tools/list_changed";
        return this->Notification(notification);
    }

    future<void> SendPromptListChanged() {
        JSON notification;
        notification["method"] = "notifications/prompts/list_changed";
        return this->Notification(notification);
    }
};

} // namespace MCP
