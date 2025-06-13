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
    optional<ServerCapabilities> m_ServerCapabilities;
    optional<Implementation> m_ServerVersion;
    ClientCapabilities m_Capabilities;
    optional<string> m_Instructions;
    map<string, ValidateFunction> m_CachedToolOutputValidators;
    AjvValidator m_Ajv;
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
    : Protocol<ClientRequest, ClientNotification, ClientResult>(Options), m_ClientInfo(ClientInfo),
      m_Capabilities(Options ? Options->Capabilities.value_or(ClientCapabilities{})
                             : ClientCapabilities{}),
      m_Ajv() {}

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
    if (!m_ServerCapabilities) {
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
    if (TransportPtr->GetSessionID()) { co_return; }

    try {
        JSON InitializeRequest =
            JSON{{MSG_METHOD, MTHD_INITIALIZE},
                 {MSG_PARAMS,
                  JSON{
                      {MSG_PROTOCOL_VERSION, MCP_LATEST_PROTOCOL_VERSION},
                      {MSG_CAPABILITIES, JSON::object()}, // TODO: Serialize m_Capabilities properly
                      {MSG_CLIENT_INFO, JSON::object()}   // TODO: Serialize m_ClientInfo properly
                  }}};

        JSON Result = co_await Request(InitializeRequest, "InitializeResultSchema", Options);

        if (Result.is_null()) {
            throw runtime_error("Server sent invalid initialize result: " + Result.dump());
        }

        // TODO: Implement SUPPORTED_PROTOCOL_VERSIONS validation
        // if (!SUPPORTED_PROTOCOL_VERSIONS.contains(Result[MSG_PROTOCOL_VERSION])) {
        //     throw runtime_error("Server's protocol version is not supported: " +
        //     Result[MSG_PROTOCOL_VERSION].get<string>());
        // }

        // TODO: Parse and set ServerCapabilities_, ServerVersion_, Instructions_ from Result
        // ServerCapabilities_ = ParseServerCapabilities(Result[MSG_CAPABILITIES]);
        // ServerVersion_ = ParseImplementation(Result[MSG_SERVER_INFO]);
        // if (Result.contains(MSG_INSTRUCTIONS)) {
        //     Instructions_ = Result[MSG_INSTRUCTIONS].get<string>();
        // }

        JSON InitializedNotification = JSON{{MSG_METHOD, MTHD_NOTIFICATION_INITIALIZED}};
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
    return m_ServerCapabilities;
}

template <typename RequestT, typename NotificationT, typename ResultT>
optional<Implementation> Client<RequestT, NotificationT, ResultT>::GetServerVersion() const {
    return m_ServerVersion;
}

template <typename RequestT, typename NotificationT, typename ResultT>
optional<string> Client<RequestT, NotificationT, ResultT>::GetInstructions() const {
    return m_Instructions;
}

template <typename RequestT, typename NotificationT, typename ResultT>
void Client<RequestT, NotificationT, ResultT>::AssertCapabilityForMethod(const string& Method) {
    if (Method == MTHD_LOGGING_SET_LEVEL) {
        if (!m_ServerCapabilities
            || !m_ServerCapabilities->Logging) { // TODO: Access logging capability properly
            throw runtime_error("Server does not support logging (required for " + Method + ")");
        }
    } else if (Method == MTHD_PROMPTS_GET || Method == MTHD_PROMPTS_LIST) {
        if (!m_ServerCapabilities
            || !m_ServerCapabilities->Prompts) { // TODO: Access prompts capability properly
            throw runtime_error("Server does not support prompts (required for " + Method + ")");
        }
    } else if (Method == MTHD_RESOURCES_LIST || Method == MTHD_RESOURCES_TEMPLATES_LIST
               || Method == MTHD_RESOURCES_READ || Method == MTHD_RESOURCES_SUBSCRIBE
               || Method == MTHD_RESOURCES_UNSUBSCRIBE) {
        if (!m_ServerCapabilities
            || !m_ServerCapabilities->Resources) { // TODO: Access resources capability properly
            throw runtime_error("Server does not support resources (required for " + Method + ")");
        }

        if (Method == MTHD_RESOURCES_SUBSCRIBE) {
            // TODO: Check ServerCapabilities_->Resources->Subscribe
            // if (!ServerCapabilities_->Resources->Subscribe) {
            //     throw runtime_error("Server does not support resource subscriptions (required for
            //     " + Method + ")");
            // }
        }
    } else if (Method == MTHD_TOOLS_CALL || Method == MTHD_TOOLS_LIST) {
        if (!m_ServerCapabilities
            || !m_ServerCapabilities->Tools) { // TODO: Access tools capability properly
            throw runtime_error("Server does not support tools (required for " + Method + ")");
        }
    } else if (Method == MTHD_COMPLETION_COMPLETE) {
        if (!m_ServerCapabilities
            || !m_ServerCapabilities->Completions) { // TODO: Access completions capability properly
            throw runtime_error("Server does not support completions (required for " + Method
                                + ")");
        }
    }
    // MTHD_INITIALIZE and MTHD_PING require no specific capability
}

template <typename RequestT, typename NotificationT, typename ResultT>
void Client<RequestT, NotificationT, ResultT>::AssertNotificationCapability(const string& Method) {
    if (Method == MTHD_NOTIFICATIONS_ROOTS_LIST_CHANGED) {
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
    if (Method == MTHD_SAMPLING_CREATE_MESSAGE) {
        // TODO: Check Capabilities_.Sampling properly
        // if (!Capabilities_.Sampling) {
        //     throw runtime_error("Client does not support sampling capability (required for " +
        //     Method + ")");
        // }
    } else if (Method == MTHD_ROOTS_LIST) {
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
    return co_await Request(JSON{{MSG_METHOD, MTHD_PING}}, "EmptyResultSchema", Options);
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<JSON>
Client<RequestT, NotificationT, ResultT>::Complete(const JSON& Params,
                                                   const optional<RequestOptions>& Options) {
    JSON CompleteRequest = JSON{{MSG_METHOD, MTHD_COMPLETION_COMPLETE}, {MSG_PARAMS, Params}};
    return co_await Request(CompleteRequest, "CompleteResultSchema", Options);
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<JSON>
Client<RequestT, NotificationT, ResultT>::SetLoggingLevel(LoggingLevel Level,
                                                          const optional<RequestOptions>& Options) {
    JSON SetLevelRequest = JSON{{MSG_METHOD, MTHD_LOGGING_SET_LEVEL},
                                {MSG_PARAMS, JSON{{MSG_LEVEL, static_cast<int>(Level)}}}};
    return co_await Request(SetLevelRequest, "EmptyResultSchema", Options);
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<JSON>
Client<RequestT, NotificationT, ResultT>::GetPrompt(const JSON& Params,
                                                    const optional<RequestOptions>& Options) {
    JSON GetPromptRequest = JSON{{MSG_METHOD, MTHD_PROMPTS_GET}, {MSG_PARAMS, Params}};
    return co_await Request(GetPromptRequest, "GetPromptResultSchema", Options);
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<JSON>
Client<RequestT, NotificationT, ResultT>::ListPrompts(const optional<JSON>& Params,
                                                      const optional<RequestOptions>& Options) {
    JSON ListPromptsRequest = JSON{{MSG_METHOD, MTHD_PROMPTS_LIST}};
    if (Params) { ListPromptsRequest[MSG_PARAMS] = *Params; }
    return co_await Request(ListPromptsRequest, "ListPromptsResultSchema", Options);
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<JSON>
Client<RequestT, NotificationT, ResultT>::ListResources(const optional<JSON>& Params,
                                                        const optional<RequestOptions>& Options) {
    JSON ListResourcesRequest = JSON{{MSG_METHOD, MTHD_RESOURCES_LIST}};
    if (Params) { ListResourcesRequest[MSG_PARAMS] = *Params; }
    return co_await Request(ListResourcesRequest, "ListResourcesResultSchema", Options);
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<JSON> Client<RequestT, NotificationT, ResultT>::ListResourceTemplates(
    const optional<JSON>& Params, const optional<RequestOptions>& Options) {
    JSON ListResourceTemplatesRequest = JSON{{MSG_METHOD, MTHD_RESOURCES_TEMPLATES_LIST}};
    if (Params) { ListResourceTemplatesRequest[MSG_PARAMS] = *Params; }
    return co_await Request(ListResourceTemplatesRequest, "ListResourceTemplatesResultSchema",
                            Options);
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<JSON>
Client<RequestT, NotificationT, ResultT>::ReadResource(const JSON& Params,
                                                       const optional<RequestOptions>& Options) {
    JSON ReadResourceRequest = JSON{{MSG_METHOD, MTHD_RESOURCES_READ}, {MSG_PARAMS, Params}};
    return co_await Request(ReadResourceRequest, "ReadResourceResultSchema", Options);
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<JSON> Client<RequestT, NotificationT, ResultT>::SubscribeResource(
    const JSON& Params, const optional<RequestOptions>& Options) {
    JSON SubscribeRequest = JSON{{MSG_METHOD, MTHD_RESOURCES_SUBSCRIBE}, {MSG_PARAMS, Params}};
    return co_await Request(SubscribeRequest, "EmptyResultSchema", Options);
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<JSON> Client<RequestT, NotificationT, ResultT>::UnsubscribeResource(
    const JSON& Params, const optional<RequestOptions>& Options) {
    JSON UnsubscribeRequest = JSON{{MSG_METHOD, MTHD_RESOURCES_UNSUBSCRIBE}, {MSG_PARAMS, Params}};
    return co_await Request(UnsubscribeRequest, "EmptyResultSchema", Options);
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<JSON>
Client<RequestT, NotificationT, ResultT>::CallTool(const JSON& Params, const string& ResultSchema,
                                                   const optional<RequestOptions>& Options) {
    JSON CallToolRequest = JSON{{MSG_METHOD, MTHD_TOOLS_CALL}, {MSG_PARAMS, Params}};

    JSON Result = co_await Request(CallToolRequest, ResultSchema, Options);

    // Check if the tool has an outputSchema
    string ToolName = Params[MSG_NAME];
    auto Validator = GetToolOutputValidator(ToolName);
    if (Validator) {
        // If tool has outputSchema, it MUST return structuredContent (unless it's an error)
        if (!Result.contains("structuredContent") && !Result.value(MSG_IS_ERROR, false)) {
            throw ErrorBase(ErrorCode::InvalidRequest,
                            "Tool " + ToolName
                                + " has an output schema but did not return structured content");
        }

        // Only validate structured content if present (not when there's an error)
        if (Result.contains("structuredContent")) {
            try {
                bool IsValid = (*Validator)(Result["structuredContent"]);
                if (!IsValid) {
                    throw ErrorBase(ErrorCode::InvalidParams,
                                    "Structured content does not match the tool's output schema: "
                                        + m_Ajv.ErrorsText(m_Ajv.Errors));
                }
            } catch (const ErrorBase& Error) { throw; } catch (const exception& Error) {
                throw ErrorBase(ErrorCode::InvalidParams,
                                "Failed to validate structured content: " + string(Error.what()));
            }
        }
    }

    co_return Result;
}

template <typename RequestT, typename NotificationT, typename ResultT>
void Client<RequestT, NotificationT, ResultT>::CacheToolOutputSchemas(const vector<Tool>& Tools) {
    m_CachedToolOutputValidators.clear();

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
    auto It = m_CachedToolOutputValidators.find(ToolName);
    if (It != m_CachedToolOutputValidators.end()) { return It->second; }
    return nullopt;
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<JSON>
Client<RequestT, NotificationT, ResultT>::ListTools(const optional<JSON>& Params,
                                                    const optional<RequestOptions>& Options) {
    JSON ListToolsRequest = JSON{{MSG_METHOD, MTHD_TOOLS_LIST}};
    if (Params) { ListToolsRequest[MSG_PARAMS] = *Params; }

    JSON Result = co_await Request(ListToolsRequest, "ListToolsResultSchema", Options);

    // Cache the tools and their output schemas for future validation
    // TODO: Extract tools from Result and call CacheToolOutputSchemas
    // vector<Tool> Tools = ExtractToolsFromResult(Result);
    // CacheToolOutputSchemas(Tools);

    co_return Result;
}

template <typename RequestT, typename NotificationT, typename ResultT>
async<void> Client<RequestT, NotificationT, ResultT>::SendRootsListChanged() {
    JSON RootsListChangedNotification = JSON{{MSG_METHOD, MTHD_NOTIFICATIONS_ROOTS_LIST_CHANGED}};
    co_await Notification(RootsListChangedNotification);
}

MCP_NAMESPACE_END
