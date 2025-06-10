#pragma once

#include <future>

#include "Core.h"

MCP_NAMESPACE_BEGIN

// Basic validation function type (TODO: Implement proper Ajv equivalent)
using ValidateFunction = function<bool(const JSON&)>;

// Basic Ajv-like validator class (TODO: Implement proper validation)
class AjvValidator {
  public:
    ValidateFunction Compile(const JSON& Schema);
    string ErrorsText(const vector<string>& Errors);
    vector<string> Errors;
};

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
template <typename RequestT = Request, typename NotificationT = Notification,
          typename ResultT = Result>
class Client : public Protocol<ClientRequest, ClientNotification, ClientResult> {
  private:
    optional<ServerCapabilities> ServerCapabilities_;
    optional<Implementation> ServerVersion_;
    ClientCapabilities Capabilities_;
    optional<string> Instructions_;
    map<string, ValidateFunction> CachedToolOutputValidators_;
    AjvValidator Ajv_;
    Implementation ClientInfo_;

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
    async<void> Connect(shared_ptr<Transport> TransportPtr,
                        const optional<RequestOptions>& Options = nullopt) override;

    /**
     * After initialization has completed, this will be populated with the server's reported
     * capabilities.
     */
    optional<ServerCapabilities> GetServerCapabilities() const;

    /**
     * After initialization has completed, this will be populated with information about the
     * server's name and version.
     */
    optional<Implementation> GetServerVersion() const;

    /**
     * After initialization has completed, this may be populated with information about the server's
     * instructions.
     */
    optional<string> GetInstructions() const;

    // Client method implementations
    async<JSON> Ping(const optional<RequestOptions>& Options = nullopt);

    async<JSON> Complete(const JSON& Params, const optional<RequestOptions>& Options = nullopt);

    async<JSON> SetLoggingLevel(LoggingLevel Level,
                                const optional<RequestOptions>& Options = nullopt);

    async<JSON> GetPrompt(const JSON& Params, const optional<RequestOptions>& Options = nullopt);

    async<JSON> ListPrompts(const optional<JSON>& Params = nullopt,
                            const optional<RequestOptions>& Options = nullopt);

    async<JSON> ListResources(const optional<JSON>& Params = nullopt,
                              const optional<RequestOptions>& Options = nullopt);

    async<JSON> ListResourceTemplates(const optional<JSON>& Params = nullopt,
                                      const optional<RequestOptions>& Options = nullopt);

    async<JSON> ReadResource(const JSON& Params, const optional<RequestOptions>& Options = nullopt);

    async<JSON> SubscribeResource(const JSON& Params,
                                  const optional<RequestOptions>& Options = nullopt);

    async<JSON> UnsubscribeResource(const JSON& Params,
                                    const optional<RequestOptions>& Options = nullopt);

    async<JSON> CallTool(const JSON& Params, const string& ResultSchema = "CallToolResultSchema",
                         const optional<RequestOptions>& Options = nullopt);

    async<JSON> ListTools(const optional<JSON>& Params = nullopt,
                          const optional<RequestOptions>& Options = nullopt);

    async<void> SendRootsListChanged();

  private:
    void CacheToolOutputSchemas(const vector<Tool>& Tools);
    optional<ValidateFunction> GetToolOutputValidator(const string& ToolName);
};

// Template implementation
template <typename RequestT, typename NotificationT, typename ResultT>
Client<RequestT, NotificationT, ResultT>::Client(const Implementation& ClientInfo,
                                                 const optional<ClientOptions>& Options)
    : Protocol<ClientRequest, ClientNotification, ClientResult>(Options), ClientInfo_(ClientInfo),
      Capabilities_(Options ? Options->Capabilities.value_or(ClientCapabilities{})
                            : ClientCapabilities{}),
      Ajv_() {}

template <typename RequestT, typename NotificationT, typename ResultT>
void Client<RequestT, NotificationT, ResultT>::RegisterCapabilities(
    const ClientCapabilities& Capabilities) {
    if (GetTransport()) {
        throw runtime_error("Cannot register capabilities after connecting to transport");
    }

    // Exactly matching TypeScript: this._capabilities = mergeCapabilities(this._capabilities,
    // capabilities);
    // TODO: Implement MergeCapabilities function
    // Capabilities_ = MergeCapabilities(Capabilities_, Capabilities);
}

template <typename RequestT, typename NotificationT, typename ResultT>
void Client<RequestT, NotificationT, ResultT>::AssertCapability(const string& Capability,
                                                                const string& Method) {
    // TODO: Implement capability checking logic based on ServerCapabilities_
    if (!ServerCapabilities_) {
        throw runtime_error("Server does not support " + Capability + " (required for " + Method
                            + ")");
    }
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<void>
Client<RequestT, NotificationT, ResultT>::Connect(shared_ptr<Transport> TransportPtr,
                                                  const optional<RequestOptions>& Options) {
    co_await Protocol<ClientRequest, ClientNotification, ClientResult>::Connect(TransportPtr);

    // When transport sessionId is already set this means we are trying to reconnect.
    // In this case we don't need to initialize again.
    if (TransportPtr->GetSessionId()) { co_return; }

    try {
        JSON InitializeRequest = JSON{
            {MSG_KEY_METHOD, MTHD_INITIALIZE},
            {MSG_KEY_PARAMS,
             JSON{
                 {MSG_KEY_PROTOCOL_VERSION, LATEST_PROTOCOL_VERSION},
                 {MSG_KEY_CAPABILITIES, JSON::object()}, // TODO: Serialize Capabilities_ properly
                 {MSG_KEY_CLIENT_INFO, JSON::object()}   // TODO: Serialize ClientInfo_ properly
             }}};

        JSON Result = co_await Request(InitializeRequest, "InitializeResultSchema", Options);

        if (Result.is_null()) {
            throw runtime_error("Server sent invalid initialize result: " + Result.dump());
        }

        // TODO: Implement SUPPORTED_PROTOCOL_VERSIONS validation
        // if (!SUPPORTED_PROTOCOL_VERSIONS.contains(Result[MSG_KEY_PROTOCOL_VERSION])) {
        //     throw runtime_error("Server's protocol version is not supported: " +
        //     Result[MSG_KEY_PROTOCOL_VERSION].get<string>());
        // }

        // TODO: Parse and set ServerCapabilities_, ServerVersion_, Instructions_ from Result
        // ServerCapabilities_ = ParseServerCapabilities(Result[MSG_KEY_CAPABILITIES]);
        // ServerVersion_ = ParseImplementation(Result[MSG_KEY_SERVER_INFO]);
        // if (Result.contains("instructions")) {
        //     Instructions_ = Result["instructions"].get<string>();
        // }

        JSON InitializedNotification = JSON{{MSG_KEY_METHOD, MTHD_NOTIFICATION_INITIALIZED}};
        co_await Notification(InitializedNotification);

    } catch (const exception& Error) {
        // Disconnect if initialization fails.
        Close();
        throw;
    }
}

template <typename RequestT, typename NotificationT, typename ResultT>
optional<ServerCapabilities>
Client<RequestT, NotificationT, ResultT>::GetServerCapabilities() const {
    return ServerCapabilities_;
}

template <typename RequestT, typename NotificationT, typename ResultT>
optional<Implementation> Client<RequestT, NotificationT, ResultT>::GetServerVersion() const {
    return ServerVersion_;
}

template <typename RequestT, typename NotificationT, typename ResultT>
optional<string> Client<RequestT, NotificationT, ResultT>::GetInstructions() const {
    return Instructions_;
}

template <typename RequestT, typename NotificationT, typename ResultT>
void Client<RequestT, NotificationT, ResultT>::AssertCapabilityForMethod(const string& Method) {
    if (Method == "logging/setLevel") {
        if (!ServerCapabilities_
            || !ServerCapabilities_->Logging) { // TODO: Access logging capability properly
            throw runtime_error("Server does not support logging (required for " + Method + ")");
        }
    } else if (Method == "prompts/get" || Method == "prompts/list") {
        if (!ServerCapabilities_
            || !ServerCapabilities_->Prompts) { // TODO: Access prompts capability properly
            throw runtime_error("Server does not support prompts (required for " + Method + ")");
        }
    } else if (Method == "resources/list" || Method == "resources/templates/list"
               || Method == "resources/read" || Method == "resources/subscribe"
               || Method == "resources/unsubscribe") {
        if (!ServerCapabilities_
            || !ServerCapabilities_->Resources) { // TODO: Access resources capability properly
            throw runtime_error("Server does not support resources (required for " + Method + ")");
        }

        if (Method == "resources/subscribe") {
            // TODO: Check ServerCapabilities_->Resources->Subscribe
            // if (!ServerCapabilities_->Resources->Subscribe) {
            //     throw runtime_error("Server does not support resource subscriptions (required for
            //     " + Method + ")");
            // }
        }
    } else if (Method == MTHD_TOOLS_CALL || Method == MTHD_TOOLS_LIST) {
        if (!ServerCapabilities_
            || !ServerCapabilities_->Tools) { // TODO: Access tools capability properly
            throw runtime_error("Server does not support tools (required for " + Method + ")");
        }
    } else if (Method == "completion/complete") {
        if (!ServerCapabilities_
            || !ServerCapabilities_->Completions) { // TODO: Access completions capability properly
            throw runtime_error("Server does not support completions (required for " + Method
                                + ")");
        }
    }
    // MTHD_INITIALIZE and MTHD_PING require no specific capability
}

template <typename RequestT, typename NotificationT, typename ResultT>
void Client<RequestT, NotificationT, ResultT>::AssertNotificationCapability(const string& Method) {
    if (Method == "notifications/roots/list_changed") {
        // TODO: Check Capabilities_.Roots.ListChanged properly
        // if (!Capabilities_.Roots || !Capabilities_.Roots->ListChanged) {
        //     throw runtime_error("Client does not support roots list changed notifications
        //     (required for " + Method + ")");
        // }
    }
    // MTHD_NOTIFICATION_INITIALIZED, MTHD_NOTIFICATION_CANCELLED, MTHD_NOTIFICATION_PROGRESS
    // require no specific capability
}

template <typename RequestT, typename NotificationT, typename ResultT>
void Client<RequestT, NotificationT, ResultT>::AssertRequestHandlerCapability(
    const string& Method) {
    if (Method == "sampling/createMessage") {
        // TODO: Check Capabilities_.Sampling properly
        // if (!Capabilities_.Sampling) {
        //     throw runtime_error("Client does not support sampling capability (required for " +
        //     Method + ")");
        // }
    } else if (Method == "roots/list") {
        // TODO: Check Capabilities_.Roots properly
        // if (!Capabilities_.Roots) {
        //     throw runtime_error("Client does not support roots capability (required for " +
        //     Method + ")");
        // }
    }
    // MTHD_PING requires no specific capability
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<JSON>
Client<RequestT, NotificationT, ResultT>::Ping(const optional<RequestOptions>& Options) {
    return co_await Request(JSON{{MSG_KEY_METHOD, MTHD_PING}}, "EmptyResultSchema", Options);
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<JSON>
Client<RequestT, NotificationT, ResultT>::Complete(const JSON& Params,
                                                   const optional<RequestOptions>& Options) {
    JSON CompleteRequest = JSON{{MSG_KEY_METHOD, "completion/complete"}, {MSG_KEY_PARAMS, Params}};
    return co_await Request(CompleteRequest, "CompleteResultSchema", Options);
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<JSON>
Client<RequestT, NotificationT, ResultT>::SetLoggingLevel(LoggingLevel Level,
                                                          const optional<RequestOptions>& Options) {
    JSON SetLevelRequest = JSON{{MSG_KEY_METHOD, "logging/setLevel"},
                                {MSG_KEY_PARAMS, JSON{{"level", static_cast<int>(Level)}}}};
    return co_await Request(SetLevelRequest, "EmptyResultSchema", Options);
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<JSON>
Client<RequestT, NotificationT, ResultT>::GetPrompt(const JSON& Params,
                                                    const optional<RequestOptions>& Options) {
    JSON GetPromptRequest = JSON{{MSG_KEY_METHOD, "prompts/get"}, {MSG_KEY_PARAMS, Params}};
    return co_await Request(GetPromptRequest, "GetPromptResultSchema", Options);
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<JSON>
Client<RequestT, NotificationT, ResultT>::ListPrompts(const optional<JSON>& Params,
                                                      const optional<RequestOptions>& Options) {
    JSON ListPromptsRequest = JSON{{MSG_KEY_METHOD, "prompts/list"}};
    if (Params) { ListPromptsRequest[MSG_KEY_PARAMS] = *Params; }
    return co_await Request(ListPromptsRequest, "ListPromptsResultSchema", Options);
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<JSON>
Client<RequestT, NotificationT, ResultT>::ListResources(const optional<JSON>& Params,
                                                        const optional<RequestOptions>& Options) {
    JSON ListResourcesRequest = JSON{{MSG_KEY_METHOD, "resources/list"}};
    if (Params) { ListResourcesRequest[MSG_KEY_PARAMS] = *Params; }
    return co_await Request(ListResourcesRequest, "ListResourcesResultSchema", Options);
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<JSON> Client<RequestT, NotificationT, ResultT>::ListResourceTemplates(
    const optional<JSON>& Params, const optional<RequestOptions>& Options) {
    JSON ListResourceTemplatesRequest = JSON{{MSG_KEY_METHOD, "resources/templates/list"}};
    if (Params) { ListResourceTemplatesRequest[MSG_KEY_PARAMS] = *Params; }
    return co_await Request(ListResourceTemplatesRequest, "ListResourceTemplatesResultSchema",
                            Options);
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<JSON>
Client<RequestT, NotificationT, ResultT>::ReadResource(const JSON& Params,
                                                       const optional<RequestOptions>& Options) {
    JSON ReadResourceRequest = JSON{{MSG_KEY_METHOD, "resources/read"}, {MSG_KEY_PARAMS, Params}};
    return co_await Request(ReadResourceRequest, "ReadResourceResultSchema", Options);
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<JSON> Client<RequestT, NotificationT, ResultT>::SubscribeResource(
    const JSON& Params, const optional<RequestOptions>& Options) {
    JSON SubscribeRequest = JSON{{MSG_KEY_METHOD, "resources/subscribe"}, {MSG_KEY_PARAMS, Params}};
    return co_await Request(SubscribeRequest, "EmptyResultSchema", Options);
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<JSON> Client<RequestT, NotificationT, ResultT>::UnsubscribeResource(
    const JSON& Params, const optional<RequestOptions>& Options) {
    JSON UnsubscribeRequest =
        JSON{{MSG_KEY_METHOD, "resources/unsubscribe"}, {MSG_KEY_PARAMS, Params}};
    return co_await Request(UnsubscribeRequest, "EmptyResultSchema", Options);
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<JSON>
Client<RequestT, NotificationT, ResultT>::CallTool(const JSON& Params, const string& ResultSchema,
                                                   const optional<RequestOptions>& Options) {
    JSON CallToolRequest = JSON{{MSG_KEY_METHOD, MTHD_TOOLS_CALL}, {MSG_KEY_PARAMS, Params}};

    JSON Result = co_await Request(CallToolRequest, ResultSchema, Options);

    // Check if the tool has an outputSchema
    string ToolName = Params[MSG_KEY_NAME];
    auto Validator = GetToolOutputValidator(ToolName);
    if (Validator) {
        // If tool has outputSchema, it MUST return structuredContent (unless it's an error)
        if (!Result.contains("structuredContent") && !Result.value(MSG_KEY_IS_ERROR, false)) {
            throw MCP_Error(ErrorCode::InvalidRequest,
                            "Tool " + ToolName
                                + " has an output schema but did not return structured content");
        }

        // Only validate structured content if present (not when there's an error)
        if (Result.contains("structuredContent")) {
            try {
                bool IsValid = (*Validator)(Result["structuredContent"]);
                if (!IsValid) {
                    throw MCP_Error(ErrorCode::InvalidParams,
                                    "Structured content does not match the tool's output schema: "
                                        + Ajv_.ErrorsText(Ajv_.Errors));
                }
            } catch (const MCP_Error& Error) { throw; } catch (const exception& Error) {
                throw MCP_Error(ErrorCode::InvalidParams,
                                "Failed to validate structured content: " + string(Error.what()));
            }
        }
    }

    co_return Result;
}

template <typename RequestT, typename NotificationT, typename ResultT>
void Client<RequestT, NotificationT, ResultT>::CacheToolOutputSchemas(const vector<Tool>& Tools) {
    CachedToolOutputValidators_.clear();

    for (const auto& ToolItem : Tools) {
        // If the tool has an outputSchema, create and cache the validator
        // TODO: Implement proper tool schema access and compilation
        // if (ToolItem.OutputSchema) {
        //     try {
        //         auto Validator = Ajv_.Compile(ToolItem.OutputSchema);
        //         CachedToolOutputValidators_[ToolItem.Name] = Validator;
        //     } catch (const exception& Error) {
        //         // Log warning: "Failed to compile output schema for tool " + ToolItem.Name + ":
        //         " + Error.what()
        //     }
        // }
    }
}

template <typename RequestT, typename NotificationT, typename ResultT>
optional<ValidateFunction>
Client<RequestT, NotificationT, ResultT>::GetToolOutputValidator(const string& ToolName) {
    auto It = CachedToolOutputValidators_.find(ToolName);
    if (It != CachedToolOutputValidators_.end()) { return It->second; }
    return nullopt;
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<JSON>
Client<RequestT, NotificationT, ResultT>::ListTools(const optional<JSON>& Params,
                                                    const optional<RequestOptions>& Options) {
    JSON ListToolsRequest = JSON{{MSG_KEY_METHOD, MTHD_TOOLS_LIST}};
    if (Params) { ListToolsRequest[MSG_KEY_PARAMS] = *Params; }

    JSON Result = co_await Request(ListToolsRequest, "ListToolsResultSchema", Options);

    // Cache the tools and their output schemas for future validation
    // TODO: Extract tools from Result and call CacheToolOutputSchemas
    // vector<Tool> Tools = ExtractToolsFromResult(Result);
    // CacheToolOutputSchemas(Tools);

    co_return Result;
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<void> Client<RequestT, NotificationT, ResultT>::SendRootsListChanged() {
    JSON RootsListChangedNotification = JSON{{MSG_KEY_METHOD, "notifications/roots/list_changed"}};
    co_await Notification(RootsListChangedNotification);
}

MCP_NAMESPACE_END
