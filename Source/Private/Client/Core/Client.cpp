#include "Client/Core/Client.h"

#include "Core/Features/Root/Roots.h"
#include "Core/Features/Tool/Tools.h"

MCP_NAMESPACE_BEGIN

void Client::RegisterCapabilities(const ClientCapabilities& Capabilities) {
    if (GetTransport()) {
        throw runtime_error("Cannot register capabilities after connecting to transport");
    }

    // Exactly matching TypeScript: this._capabilities = mergeCapabilities(this._capabilities,
    // capabilities);
    // TODO: Implement MergeCapabilities function
    // Capabilities_ = MergeCapabilities(Capabilities_, Capabilities);
}

void Client::AssertCapability(const string& Capability, const string& Method) {
    // TODO: Implement capability checking logic based on ServerCapabilities_
    if (!m_ServerCapabilities) {
        throw runtime_error("Server does not support " + Capability + " (required for " + Method
                            + ")");
    }
}

future<void> Client::Connect(shared_ptr<Transport> TransportPtr,
                             const optional<RequestOptions>& Options) {
    co_await IMCPProtocol<ClientRequest, ClientNotification, ClientResult>::Connect(TransportPtr);

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

optional<ServerCapabilities> Client::GetServerCapabilities() const {
    return m_ServerCapabilities;
}

optional<Implementation> Client::GetServerVersion() const {
    return m_ServerVersion;
}

optional<string> Client::GetInstructions() const {
    return m_Instructions;
}

void Client::AssertCapabilityForMethod(const string& Method) {
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

void Client::AssertNotificationCapability(const string& Method) {
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

void Client::AssertRequestHandlerCapability(const string& Method) {
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

future<JSON> Client::Ping(const optional<RequestOptions>& Options) {
    return co_await Request(JSON{{MSG_METHOD, MTHD_PING}}, "EmptyResultSchema", Options);
}

future<JSON> Client::Complete(const JSON& Params, const optional<RequestOptions>& Options) {
    JSON CompleteRequest = JSON{{MSG_METHOD, MTHD_COMPLETION_COMPLETE}, {MSG_PARAMS, Params}};
    return co_await Request(CompleteRequest, "CompleteResultSchema", Options);
}

future<JSON> Client::SetLoggingLevel(LoggingLevel Level, const optional<RequestOptions>& Options) {
    JSON SetLevelRequest = JSON{{MSG_METHOD, MTHD_LOGGING_SET_LEVEL},
                                {MSG_PARAMS, JSON{{MSG_LEVEL, static_cast<int>(Level)}}}};
    return co_await Request(SetLevelRequest, "EmptyResultSchema", Options);
}

future<JSON> Client::GetPrompt(const JSON& Params, const optional<RequestOptions>& Options) {
    JSON GetPromptRequest = JSON{{MSG_METHOD, MTHD_PROMPTS_GET}, {MSG_PARAMS, Params}};
    return co_await Request(GetPromptRequest, "GetPromptResultSchema", Options);
}

future<JSON> Client::ListPrompts(const optional<JSON>& Params,
                                 const optional<RequestOptions>& Options) {
    JSON ListPromptsRequest = JSON{{MSG_METHOD, MTHD_PROMPTS_LIST}};
    if (Params) { ListPromptsRequest[MSG_PARAMS] = *Params; }
    return co_await Request(ListPromptsRequest, "ListPromptsResultSchema", Options);
}

future<JSON> Client::ListResources(const optional<JSON>& Params,
                                   const optional<RequestOptions>& Options) {
    JSON ListResourcesRequest = JSON{{MSG_METHOD, MTHD_RESOURCES_LIST}};
    if (Params) { ListResourcesRequest[MSG_PARAMS] = *Params; }
    return co_await Request(ListResourcesRequest, "ListResourcesResultSchema", Options);
}

future<JSON> Client::ListResourceTemplates(const optional<JSON>& Params,
                                           const optional<RequestOptions>& Options) {
    JSON ListResourceTemplatesRequest = JSON{{MSG_METHOD, MTHD_RESOURCES_TEMPLATES_LIST}};
    if (Params) { ListResourceTemplatesRequest[MSG_PARAMS] = *Params; }
    return co_await Request(ListResourceTemplatesRequest, "ListResourceTemplatesResultSchema",
                            Options);
}

future<JSON> Client::ReadResource(const JSON& Params, const optional<RequestOptions>& Options) {
    JSON ReadResourceRequest = JSON{{MSG_METHOD, MTHD_RESOURCES_READ}, {MSG_PARAMS, Params}};
    return co_await Request(ReadResourceRequest, "ReadResourceResultSchema", Options);
}

future<JSON> Client::SubscribeResource(const JSON& Params,
                                       const optional<RequestOptions>& Options) {
    JSON SubscribeRequest = JSON{{MSG_METHOD, MTHD_RESOURCES_SUBSCRIBE}, {MSG_PARAMS, Params}};
    return co_await Request(SubscribeRequest, "EmptyResultSchema", Options);
}

future<JSON> Client::UnsubscribeResource(const JSON& Params,
                                         const optional<RequestOptions>& Options) {
    JSON UnsubscribeRequest = JSON{{MSG_METHOD, MTHD_RESOURCES_UNSUBSCRIBE}, {MSG_PARAMS, Params}};
    return co_await Request(UnsubscribeRequest, "EmptyResultSchema", Options);
}

future<JSON> Client::CallTool(const JSON& Params, const string& ResultSchema,
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

void Client::CacheToolOutputSchemas(const vector<Tool>& Tools) {
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

optional<ValidateFunction> Client::GetToolOutputValidator(const string& ToolName) {
    auto It = m_CachedToolOutputValidators.find(ToolName);
    if (It != m_CachedToolOutputValidators.end()) { return It->second; }
    return nullopt;
}

future<ListToolsResult> Client::ListTools() {
    SendRequest<ListToolsRequest>();
    ListToolsResult Result;
    co_await ListenResponse(Result);
    // ListToolsResult Result = co_await Request(Request, "ListToolsResultSchema");

    // Cache the tools and their output schemas for future validation
    // TODO: Extract tools from Result and call CacheToolOutputSchemas
    // vector<Tool> Tools = ExtractToolsFromResult(Result);
    // CacheToolOutputSchemas(Tools);

    co_return Result;
}

void Client::SendRootsListChanged() {
    RootsListChangedNotification Notification;
    SendNotification(Notification);
    return;
}

MCP_NAMESPACE_END