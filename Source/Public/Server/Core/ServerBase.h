#pragma once

#include "Core.h"
#include "IMCP.h"

MCP_NAMESPACE_BEGIN

/**
 * Callback to complete one variable within a resource template's URI template.
 */
using CompleteResourceTemplateCallback = function<vector<string>(const string&)>;

/**
 * Callback to list all resources matching a given template.
 */
using ListResourcesCallback =
    function<ListResourcesResult>(const RequestHandlerExtra<ServerRequest, ServerNotification>&);

/**
 * Callback to read a resource at a given URI.
 */
using ReadResourceCallback = function<ReadResourceResult(
    const string& InURI, const RequestHandlerExtra<ServerRequest, ServerNotification>&)>;

/**
 * Callback to read a resource at a given URI, following a filled-in URI template.
 */
using ReadResourceTemplateCallback = function<ReadResourceResult(
    const string& InURI, const unordered_map<string, string>& InVariables,
    const RequestHandlerExtra<ServerRequest, ServerNotification>&)>;

/**
 * A resource template combines a URI pattern with optional functionality to enumerate
 * all resources matching that pattern.
 */
class ResourceTemplate {
  private:
    string m_URITemplate;
    optional<ListResourcesCallback> m_ListCallback;
    unordered_map<string, CompleteResourceTemplateCallback> m_CompleteCallbacks;

  public:
    ResourceTemplate(const string& InURITemplate,
                     const optional<ListResourcesCallback>& InListCallback,
                     const optional<unordered_map<string, CompleteResourceTemplateCallback>>&
                         InCompleteCallbacks = nullopt)
        : m_URITemplate(InURITemplate), m_ListCallback(InListCallback) {
        if (InCompleteCallbacks) { m_CompleteCallbacks = *InCompleteCallbacks; }
    }

    /**
     * Gets the URI template pattern.
     */
    const string& GetURITemplate() const {
        return m_URITemplate;
    }

    /**
     * Gets the list callback, if one was provided.
     */
    const optional<ListResourcesCallback>& GetListCallback() const {
        return m_ListCallback;
    }

    /**
     * Gets the callback for completing a specific URI template variable, if one was provided.
     */
    optional<CompleteResourceTemplateCallback> GetCompleteCallback(const string& InVariable) const {
        auto it = m_CompleteCallbacks.find(InVariable);
        return it != m_CompleteCallbacks.end()
                   ? optional<CompleteResourceTemplateCallback>(it->second)
                   : nullopt;
    }

    /**
     * Matches a URI against this template and returns variables if successful.
     */
    optional<unordered_map<string, string>> Match(const string& InURI) const {
        URITemplate URITemplate(m_URITemplate);
        auto Variables = URITemplate.Match(InURI);

        if (Variables.empty()) { return nullopt; }

        // Convert Variables type to unordered_map<string, string>
        unordered_map<string, string> Result;
        for (const auto& [Key, Value] : Variables) {
            if (std::holds_alternative<string>(Value)) {
                Result[Key] = std::get<string>(Value);
            } else if (std::holds_alternative<vector<string>>(Value)) {
                // Join vector values with comma for compatibility
                const auto& Values = std::get<vector<string>>(Value);
                string Joined;
                for (size_t i = 0; i < Values.size(); ++i) {
                    if (i > 0) Joined += ",";
                    Joined += Values[i];
                }
                Result[Key] = Joined;
            }
        }
        return Result;
    }
};

// Type aliases for tool callbacks
template <typename Args = void>
using ToolCallback = typename std::conditional<
    std::is_same_v<Args, void>,
    function<CallToolResult(const RequestHandlerExtra<ServerRequest, ServerNotification>&)>,
    function<CallToolResult(const Args&,
                            const RequestHandlerExtra<ServerRequest, ServerNotification>&)>>::type;

// Type aliases for prompt callbacks
template <typename Args = void>
using PromptCallback = typename std::conditional<
    std::is_same_v<Args, void>,
    function<GetPromptResult(const RequestHandlerExtra<ServerRequest, ServerNotification>&)>,
    function<GetPromptResult(const Args&,
                             const RequestHandlerExtra<ServerRequest, ServerNotification>&)>>::type;

struct RegisteredTool {
    optional<string> Description;
    optional<JSON> InputSchema;  // JSON schema for input validation (replaces Zod)
    optional<JSON> OutputSchema; // JSON schema for output validation (replaces Zod)
    optional<ToolAnnotations> Annotations;
    function<CallToolResult(const JSON&,
                            const RequestHandlerExtra<ServerRequest, ServerNotification>&)>
        Callback;
    bool Enabled = true;

    void Enable() {
        Update({{"enabled", true}});
    }
    void Disable() {
        Update({{"enabled", false}});
    }
    void Remove() {
        Update({{MSG_NAME, nullptr}});
    }

    void Update(const unordered_map<string, JSON>& updates) {
        // Implementation for updating tool properties
        for (const auto& [key, value] : updates) {
            if (key == "enabled" && value.is_boolean()) { Enabled = value.get<bool>(); }
            // Handle other update fields
        }
        // Trigger tool list changed notification
    }
};

struct RegisteredResource {
    string Name;
    optional<ResourceMetadata> Metadata;
    ReadResourceCallback Callback;
    bool Enabled = true;

    void Enable() {
        Update({{"enabled", true}});
    }
    void Disable() {
        Update({{"enabled", false}});
    }
    void Remove() {
        Update({{MSG_URI, nullptr}});
    }

    void Update(const unordered_map<string, JSON>& updates) {
        // Implementation for updating resource properties
        for (const auto& [key, value] : updates) {
            if (key == "enabled" && value.is_boolean()) { Enabled = value.get<bool>(); }
            // Handle other update fields
        }
        // Trigger resource list changed notification
    }
};

struct RegisteredResourceTemplate {
    ResourceTemplate Template;
    optional<ResourceMetadata> Metadata;
    ReadResourceTemplateCallback Callback;
    bool Enabled = true;

    RegisteredResourceTemplate(const ResourceTemplate& tmpl) : Template(tmpl) {}

    void Enable() {
        Update({{"enabled", true}});
    }
    void Disable() {
        Update({{"enabled", false}});
    }
    void Remove() {
        Update({{MSG_NAME, nullptr}});
    }

    void Update(const unordered_map<string, JSON>& updates) {
        // Implementation for updating resource template properties
        for (const auto& [key, value] : updates) {
            if (key == "enabled" && value.is_boolean()) { Enabled = value.get<bool>(); }
            // Handle other update fields
        }
        // Trigger resource list changed notification
    }
};

struct RegisteredPrompt {
    optional<string> Description;
    optional<JSON> ArgsSchema; // JSON schema for args validation (replaces Zod)
    function<GetPromptResult(const JSON&,
                             const RequestHandlerExtra<ServerRequest, ServerNotification>&)>
        Callback;
    bool Enabled = true;

    void Enable() {
        Update({{"enabled", true}});
    }
    void Disable() {
        Update({{"enabled", false}});
    }
    void Remove() {
        Update({{MSG_NAME, nullptr}});
    }

    void Update(const unordered_map<string, JSON>& updates) {
        // Implementation for updating prompt properties
        for (const auto& [key, value] : updates) {
            if (key == "enabled" && value.is_boolean()) { Enabled = value.get<bool>(); }
            // Handle other update fields
        }
        // Trigger prompt list changed notification
    }
};

// Server Interface
class IServerAPI {
    // TODO: @HalcyonOmega This does not seem to align with actual server role
  public:
    virtual ~IServerAPI() = default;

    // Tool Management
    virtual MCPTask<vector<Tool>> ListTools(const optional<string>& InCursor = {}) = 0;
    virtual MCPTask<ToolResult> CallTool(const ToolCall& InCall) = 0;

    // Tool Registration
    virtual void RegisterTool(const Tool& InTool,
                              function<MCPTask<ToolResult>(const ToolCall&)> InHandler) = 0;
    virtual void UnregisterTool(const string& InToolName) = 0;

    // Resource Management
    virtual MCPTask<vector<Resource>> ListResources(const optional<string>& InCursor = {}) = 0;
    virtual MCPTask<vector<ResourceContent>> ReadResource(const string& InURI) = 0;
    virtual MCPTask<vector<ResourceTemplate>>
    ListResourceTemplates(const optional<string>& InCursor = {}) = 0;

    // Resource Registration
    virtual void RegisterResource(const Resource& InResource,
                                  function<MCPTask<vector<ResourceContent>>()> InProvider) = 0;
    virtual void UnregisterResource(const string& InURI) = 0;

    // Resource Subscription
    virtual MCPTask_Void SubscribeToResource(const string& InURI) = 0;
    virtual MCPTask_Void UnsubscribeFromResource(const string& InURI) = 0;

    // Prompt Management
    virtual MCPTask<vector<Prompt>> ListPrompts(const optional<string>& InCursor = {}) = 0;
    virtual MCPTask<PromptMessage>
    GetPrompt(const string& InName, const unordered_map<string, string>& InArguments = {}) = 0;

    // Prompt Registration
    virtual void RegisterPrompt(
        const Prompt& InPrompt,
        function<MCPTask<PromptMessage>(const unordered_map<string, string>&)> InHandler) = 0;
    virtual void UnregisterPrompt(const string& InPromptName) = 0;

    // Logging
    virtual MCPTask_Void LogMessage(const string& InLevel, const string& InMessage,
                                    const optional<string>& InLogger = {}) = 0;

    // Progress Tracking
    virtual MCPTask_Void ReportProgress(const string& InProgressToken, double InProgress,
                                        const optional<string>& InTotal = {}) = 0;

    // Notifications
    virtual void OnToolListChanged(function<void()> InCallback) = 0;
    virtual void OnResourceListChanged(function<void()> InCallback) = 0;
    virtual void OnPromptListChanged(function<void()> InCallback) = 0;
    virtual void OnResourceUpdated(function<void(const string& InURI)> InCallback) = 0;

    // Capabilities
    virtual void SetCapabilities(const ServerCapabilities& InCapabilities) = 0;
    virtual ServerCapabilities GetCapabilities() const = 0;

    // Registers new capabilities. This can only be called before connecting to a transport.
    //
    // The new capabilities will be merged with any existing capabilities previously given (e.g., at
    // initialization).
    void RegisterCapabilities(const ServerCapabilities& InCapabilities);

    // TODO: @HalcyonOmega Identify if needed. Direct Translation server methods
    future<JSON> CreateMessage(const JSON& InParams);
    future<JSON> ListRoots(const optional<JSON>& InParams = nullopt);
    future<void> SendLoggingMessage(const JSON& InParams);
    future<void> SendResourceUpdated(const JSON& InParams);
    future<void> SendResourceListChanged();
    future<void> SendToolListChanged();
    future<void> SendPromptListChanged();
};

/**
 * An MCP server on top of a pluggable transport.
 *
 * This server will automatically respond to the initialization flow as initiated from the client.
 *
 * To use with custom types, extend the base Request/Notification/Result types and pass them as type
 * parameters.
 */
class ServerBase : public IServerAPI, public IMCP {
  public:
    struct ServerOptions : public ProtocolOptions {
        optional<ServerCapabilities>
            Capabilities; // Capabilities to advertise as being supported by this server.
        optional<string> Instructions; // Optional instructions describing how to use the server and
                                       // its features.
    };

    ServerBase(const ServerOptions& InOptions);
    ~ServerBase() override;

    /* MCP Server Interface */
    /**
     * Attaches to the given transport, starts it, and starts listening for messages.
     *
     * The server object assumes ownership of the Transport, replacing any callbacks that have
     * already been set, and expects that it is the only user of the Transport instance going
     * forward.
     */
    MCPTask_Void Connect(shared_ptr<Transport> InTransport); // TODO: Maybe make optional, if not
                                                             // present, use default transport

    /**
     * Closes the connection.
     */
    future<void> Close() {
        return m_ServerInstance->Close();
    }

  private:
    void SetToolRequestHandlers() {
        if (m_ToolHandlersInitialized) { return; }

        // Assert can set request handlers
        m_ServerInstance->AssertCanSetRequestHandler(MTHD_TOOLS_LIST);
        m_ServerInstance->AssertCanSetRequestHandler(MTHD_TOOLS_CALL);

        // Register capabilities
        ServerCapabilities caps; // TODO: Implement ServerCapabilities
        // TODO: Set up tools capability with listChanged = true
        m_ServerInstance->RegisterCapabilities(caps);

        // Set handler for tools/list
        m_ServerInstance->SetRequestHandler(
            MTHD_TOOLS_LIST,
            [this](const JSON& InRequest,
                   const RequestHandlerExtra<ServerRequest, ServerNotification>& InExtra)
                -> ListToolsResult {
                ListToolsResult Result;

                for (const auto& [Name, Tool] : m_RegisteredTools) {
                    if (!Tool.Enabled) continue;

                    Tool ToolDef;
                    ToolDef.Name = Name;
                    ToolDef.Description = Tool.Description;
                    ToolDef.InputSchema = Tool.InputSchema.value_or(m_EmptyObjectJSONSchema);
                    if (Tool.OutputSchema) { ToolDef.OutputSchema = *Tool.OutputSchema; }
                    ToolDef.Annotations = Tool.Annotations;

                    Result.Tools.push_back(ToolDef);
                }

                return Result;
            });

        // Set handler for tools/call
        m_ServerInstance->SetRequestHandler(
            MTHD_TOOLS_CALL,
            [this](const JSON& InRequest,
                   const RequestHandlerExtra<ServerRequest, ServerNotification>& InExtra)
                -> CallToolResult {
                auto ToolName = InRequest[MSG_PARAMS][MSG_NAME].get<string>();
                auto ToolArgs = InRequest[MSG_PARAMS].value(MSG_ARGUMENTS, JSON::object());

                auto it = m_RegisteredTools.find(ToolName);
                if (it == m_RegisteredTools.end()) {
                    throw ErrorBase(ErrorCode::InvalidParams, "Tool " + ToolName + " not found");
                }

                const auto& Tool = it->second;
                if (!Tool.Enabled) {
                    throw ErrorBase(ErrorCode::InvalidParams, "Tool " + ToolName + " disabled");
                }

                CallToolResult result;
                try {
                    // Validate input against InputSchema if present
                    if (Tool.InputSchema) {
                        auto Validator = m_SchemaValidator.Compile(*Tool.InputSchema);
                        if (!Validator(ToolArgs)) {
                            throw ErrorBase(
                                ErrorCode::InvalidParams,
                                "Invalid arguments for tool " + ToolName + ": "
                                    + m_SchemaValidator.ErrorsText(m_SchemaValidator.Errors));
                        }
                    }

                    result = Tool.Callback(ToolArgs, extra);

                    // Validate output against OutputSchema if present
                    if (Tool.OutputSchema && result.StructuredContent) {
                        auto validator = m_SchemaValidator.Compile(*Tool.OutputSchema);
                        if (!validator(*result.StructuredContent)) {
                            throw ErrorBase(
                                ErrorCode::InvalidParams,
                                "Invalid structured content for tool " + ToolName + ": "
                                    + m_SchemaValidator.ErrorsText(m_SchemaValidator.Errors));
                        }
                    }

                    return Result;
                } catch (const exception& E) {
                    CallToolResult ErrorResult;
                    ErrorResult.IsError = true;
                    // Create error content
                    Content ErrorContent;
                    ErrorContent.Type = MSG_TEXT;
                    ErrorContent.Text = E.what();
                    ErrorResult.Content.push_back(ErrorContent);
                    return ErrorResult;
                }
            });

        m_ToolHandlersInitialized = true;
    }

    void SetCompletionRequestHandler() {
        if (m_CompletionHandlerInitialized) { return; }

        // Assert can set request handler
        m_ServerInstance->AssertCanSetRequestHandler(MTHD_COMPLETION_COMPLETE);

        m_ServerInstance->SetRequestHandler(
            MTHD_COMPLETION_COMPLETE,
            [this](const JSON& InRequest,
                   const RequestHandlerExtra<ServerRequest, ServerNotification>& InExtra)
                -> CompleteResult {
                auto RefType = InRequest[MSG_PARAMS][MSG_REF][MSG_TYPE].get<string>();

                if (RefType == "ref/prompt") {
                    return HandlePromptCompletion(InRequest, InExtra);
                } else if (RefType == "ref/resource") {
                    return HandleResourceCompletion(InRequest, InExtra);
                } else {
                    throw ErrorBase(ErrorCode::InvalidParams,
                                    "Invalid completion reference: " + RefType);
                }
            });

        m_CompletionHandlerInitialized = true;
    }

    CompleteResult
    HandlePromptCompletion(const JSON& InRequest,
                           const RequestHandlerExtra<ServerRequest, ServerNotification>& InExtra) {
        auto PromptName = InRequest[MSG_PARAMS][MSG_REF][MSG_NAME].get<string>();

        auto It = m_RegisteredPrompts.find(PromptName);
        if (It == m_RegisteredPrompts.end()) {
            throw ErrorBase(ErrorCode::InvalidParams, "Prompt " + PromptName + " not found");
        }

        const auto& Prompt = It->second;
        if (!Prompt.Enabled) {
            throw ErrorBase(ErrorCode::InvalidParams, "Prompt " + PromptName + " disabled");
        }

        if (!Prompt.ArgsSchema) { return m_EmptyCompletionResult; }

        // TODO: Implement completion logic for prompt arguments
        return m_EmptyCompletionResult;
    }

    CompleteResult HandleResourceCompletion(
        const JSON& InRequest,
        const RequestHandlerExtra<ServerRequest, ServerNotification>& InExtra) {
        auto URI = InRequest[MSG_PARAMS][MSG_REF][MSG_URI].get<string>();

        // Find matching template
        for (const auto& [Name, TemplateEntry] : m_RegisteredResourceTemplates) {
            auto Variables = TemplateEntry.Template.Match(URI);
            if (Variables) {
                auto ArgName = InRequest[MSG_PARAMS]["argument"][MSG_NAME].get<string>();
                auto Completer = TemplateEntry.Template.GetCompleteCallback(ArgName);
                if (Completer) {
                    auto ArgValue = InRequest[MSG_PARAMS]["argument"][MSG_VALUE].get<string>();
                    auto Suggestions = (*Completer)(ArgValue);
                    return CreateCompletionResult(Suggestions);
                }
            }
        }

        // Check if it's a fixed resource URI
        if (m_RegisteredResources.find(URI) != m_RegisteredResources.end()) {
            return m_EmptyCompletionResult;
        }

        throw ErrorBase(ErrorCode::InvalidParams, "Resource template " + URI + " not found");
    }

    void SetResourceRequestHandlers() {
        if (m_ResourceHandlersInitialized) { return; }

        // Assert can set request handlers
        m_ServerInstance->AssertCanSetRequestHandler(MTHD_RESOURCES_LIST);
        m_ServerInstance->AssertCanSetRequestHandler(MTHD_RESOURCES_TEMPLATES_LIST);
        m_ServerInstance->AssertCanSetRequestHandler(MTHD_RESOURCES_READ);

        // Register capabilities
        ServerCapabilities Caps;
        // TODO: Set up resources capability with listChanged = true
        m_ServerInstance->RegisterCapabilities(Caps);

        // Set handler for resources/list
        m_ServerInstance->SetRequestHandler(
            MTHD_RESOURCES_LIST,
            [this](const JSON& InRequest,
                   const RequestHandlerExtra<ServerRequest, ServerNotification>& InExtra)
                -> ListResourcesResult {
                ListResourcesResult Result;

                // Add fixed resources
                for (const auto& [URI, Resource] : m_RegisteredResources) {
                    if (!Resource.Enabled) continue;

                    Resource Res;
                    Res.URI = URI;
                    Res.Name = Resource.Name;
                    if (Resource.Metadata) {
                        // TODO: Copy metadata fields
                    }
                    Result.Resources.push_back(Res);
                }

                // Add template resources
                for (const auto& [Name, TemplateEntry] : m_RegisteredResourceTemplates) {
                    if (TemplateEntry.Template.GetListCallback()) {
                        auto TemplateResult = (*TemplateEntry.Template.GetListCallback())(InExtra);
                        for (const auto& Resource : TemplateResult.Resources) {
                            Result.Resources.push_back(Resource);
                        }
                    }
                }

                return Result;
            });

        // Set handler for resources/templates/list
        m_ServerInstance->SetRequestHandler(
            MTHD_RESOURCES_TEMPLATES_LIST,
            [this](const JSON& InRequest,
                   const RequestHandlerExtra<ServerRequest, ServerNotification>& InExtra)
                -> ListResourceTemplatesResult {
                ListResourceTemplatesResult Result;

                for (const auto& [Name, TemplateEntry] : m_RegisteredResourceTemplates) {
                    ResourceTemplate ResTmpl;
                    ResTmpl.Name = Name;
                    ResTmpl.URITemplate = TemplateEntry.Template.GetURITemplate();
                    if (TemplateEntry.Metadata) {
                        // TODO: Copy metadata fields
                    }
                    Result.ResourceTemplates.push_back(ResTmpl);
                }

                return Result;
            });

        // Set handler for resources/read
        m_ServerInstance->SetRequestHandler(
            MTHD_RESOURCES_READ,
            [this](const JSON& InRequest,
                   const RequestHandlerExtra<ServerRequest, ServerNotification>& InExtra)
                -> ReadResourceResult {
                auto URI = InRequest[MSG_PARAMS][MSG_URI].get<string>();

                // Check for exact resource match first
                auto ResourceIt = m_RegisteredResources.find(URI);
                if (ResourceIt != m_RegisteredResources.end()) {
                    if (!ResourceIt->second.Enabled) {
                        throw ErrorBase(ErrorCode::InvalidParams, "Resource " + URI + " disabled");
                    }
                    return ResourceIt->second.Callback(URI, InExtra);
                }

                // Check templates
                for (const auto& [Name, TemplateEntry] : m_RegisteredResourceTemplates) {
                    auto Variables = TemplateEntry.Template.Match(URI);
                    if (Variables) { return TemplateEntry.Callback(URI, *Variables, InExtra); }
                }

                throw ErrorBase(ErrorCode::InvalidParams, "Resource " + URI + " not found");
            });

        SetCompletionRequestHandler();
        m_ResourceHandlersInitialized = true;
    }

    void SetPromptRequestHandlers() {
        if (m_PromptHandlersInitialized) { return; }

        // Assert can set request handlers
        m_ServerInstance->AssertCanSetRequestHandler(MTHD_PROMPTS_LIST);
        m_ServerInstance->AssertCanSetRequestHandler(MTHD_PROMPTS_GET);

        // Register capabilities
        ServerCapabilities Caps;
        // TODO: Set up prompts capability with listChanged = true
        m_ServerInstance->RegisterCapabilities(Caps);

        // Set handler for prompts/list
        m_ServerInstance->SetRequestHandler(
            MTHD_PROMPTS_LIST,
            [this](const JSON& InRequest,
                   const RequestHandlerExtra<ServerRequest, ServerNotification>& InExtra)
                -> ListPromptsResult {
                ListPromptsResult Result;

                for (const auto& [Name, Prompt] : m_RegisteredPrompts) {
                    if (!Prompt.Enabled) continue;

                    Prompt promptDef;
                    PromptDef.Name = Name;
                    PromptDef.Description = Prompt.Description;
                    if (Prompt.ArgsSchema) {
                        // TODO: Convert JSON schema to PromptArgument vector
                    }

                    Result.Prompts.push_back(PromptDef);
                }

                return Result;
            });

        // Set handler for prompts/get
        m_ServerInstance->SetRequestHandler(
            MTHD_PROMPTS_GET,
            [this](const JSON& InRequest,
                   const RequestHandlerExtra<ServerRequest, ServerNotification>& InExtra)
                -> GetPromptResult {
                auto PromptName = InRequest[MSG_PARAMS][MSG_NAME].get<string>();
                auto Args = InRequest[MSG_PARAMS].value(MSG_ARGUMENTS, JSON::object());

                auto It = m_RegisteredPrompts.find(PromptName);
                if (It == m_RegisteredPrompts.end()) {
                    throw ErrorBase(ErrorCode::InvalidParams,
                                    "Prompt " + PromptName + " not found");
                }

                const auto& Prompt = It->second;
                if (!Prompt.Enabled) {
                    throw ErrorBase(ErrorCode::InvalidParams, "Prompt " + PromptName + " disabled");
                }

                // Validate arguments against ArgsSchema if present
                if (Prompt.ArgsSchema) {
                    auto Validator = m_SchemaValidator.Compile(*Prompt.ArgsSchema);
                    if (!Validator(Args)) {
                        throw ErrorBase(
                            ErrorCode::InvalidParams,
                            "Invalid arguments for prompt " + PromptName + ": "
                                + m_SchemaValidator.ErrorsText(m_SchemaValidator.Errors));
                    }
                }

                return Prompt.Callback(Args, InExtra);
            });

        SetCompletionRequestHandler();
        m_PromptHandlersInitialized = true;
    }

    RegisteredTool CreateRegisteredTool(
        const string& Name, const optional<string>& Description, const optional<JSON>& InputSchema,
        const optional<JSON>& OutputSchema, const optional<ToolAnnotations>& Annotations,
        const function<CallToolResult(
            const JSON&, const RequestHandlerExtra<ServerRequest, ServerNotification>&)>&
            callback) {
        RegisteredTool Tool;
        Tool.Description = Description;
        Tool.InputSchema = InputSchema;
        Tool.OutputSchema = OutputSchema;
        Tool.Annotations = Annotations;
        Tool.Callback = Callback;
        Tool.Enabled = true;

        m_RegisteredTools[Name] = Tool;
        SetToolRequestHandlers();
        SendToolListChanged();

        return Tool;
    }

    static CompleteResult CreateCompletionResult(const vector<string>& Suggestions) {
        CompleteResult Result;

        // Limit to maximum 100 suggestions as per MCP protocol
        size_t MaxCount = min(Suggestions.size(), static_cast<size_t>(100));
        vector<string> LimitedValues(Suggestions.begin(), Suggestions.begin() + MaxCount);

        // TODO: Set up completion structure with values, total, and hasMore
        // result.Completion.Values = limitedValues;
        // result.Completion.Total = suggestions.size();
        // result.Completion.HasMore = suggestions.size() > 100;

        return Result;
    }

    static vector<PromptArgument> PromptArgumentsFromSchema(const JSON& Schema) {
        vector<PromptArgument> Args;
        // TODO: Convert JSON schema to PromptArgument vector
        return Args;
    }

  public:
    RegisteredResource Resource(const string& Name, const string& URI,
                                const optional<ResourceMetadata>& Metadata = nullopt,
                                const ReadResourceCallback& Callback) {
        if (m_RegisteredResources.find(URI) != m_RegisteredResources.end()) {
            throw runtime_error("Resource " + URI + " is already registered");
        }

        RegisteredResource Resource;
        Resource.Name = Name;
        Resource.Metadata = Metadata;
        Resource.Callback = Callback;
        Resource.Enabled = true;

        m_RegisteredResources[URI] = Resource;
        SetResourceRequestHandlers();
        SendResourceListChanged();

        return Resource;
    }

    RegisteredResourceTemplate Resource(const string& Name, const ResourceTemplate& Tmpl,
                                        const optional<ResourceMetadata>& Metadata = nullopt,
                                        const ReadResourceTemplateCallback& Callback) {
        if (m_RegisteredResourceTemplates.find(Name) != m_RegisteredResourceTemplates.end()) {
            throw runtime_error("Resource template " + Name + " is already registered");
        }

        RegisteredResourceTemplate ResourceTemplate(Tmpl);
        ResourceTemplate.Metadata = Metadata;
        ResourceTemplate.Callback = Callback;
        ResourceTemplate.Enabled = true;

        m_RegisteredResourceTemplates[Name] = ResourceTemplate;
        SetResourceRequestHandlers();
        SendResourceListChanged();

        return ResourceTemplate;
    }

    // Tool registration methods
    RegisteredTool Tool(const string& Name, const ToolCallback<void>& Callback) {
        return CreateRegisteredTool(
            Name, nullopt, nullopt, nullopt, nullopt,
            [Callback](const JSON& Args,
                       const RequestHandlerExtra<ServerRequest, ServerNotification>& InExtra) {
                return Callback(InExtra);
            });
    }

    RegisteredTool Tool(const string& Name, const string& Description,
                        const ToolCallback<void>& Callback) {
        return CreateRegisteredTool(
            Name, Description, nullopt, nullopt, nullopt,
            [Callback](const JSON& Args,
                       const RequestHandlerExtra<ServerRequest, ServerNotification>& InExtra) {
                return Callback(InExtra);
            });
    }

    template <typename Args>
    RegisteredTool Tool(const string& Name, const JSON& ParamsSchemaOrAnnotations,
                        const ToolCallback<Args>& Callback) {
        // TODO: Determine if paramsSchemaOrAnnotations is schema or annotations
        return CreateRegisteredTool(
            Name, nullopt, ParamsSchemaOrAnnotations, nullopt, nullopt,
            [Callback](const JSON& Args,
                       const RequestHandlerExtra<ServerRequest, ServerNotification>& InExtra) {
                // TODO: Parse args to Args type
                Args ParsedArgs; // Would convert JSON to Args
                return Callback(ParsedArgs, InExtra);
            });
    }

    template <typename Args>
    RegisteredTool Tool(const string& Name, const string& Description,
                        const JSON& ParamsSchemaOrAnnotations, const ToolCallback<Args>& Callback) {
        // TODO: Determine if paramsSchemaOrAnnotations is schema or annotations
        return CreateRegisteredTool(
            Name, Description, ParamsSchemaOrAnnotations, nullopt, nullopt,
            [Callback](const JSON& Args,
                       const RequestHandlerExtra<ServerRequest, ServerNotification>& InExtra) {
                // TODO: Parse args to Args type
                Args ParsedArgs; // Would convert JSON to Args
                return Callback(ParsedArgs, InExtra);
            });
    }

    template <typename Args>
    RegisteredTool Tool(const string& Name, const JSON& ParamsSchema,
                        const ToolAnnotations& Annotations, const ToolCallback<Args>& Callback) {
        return CreateRegisteredTool(
            Name, nullopt, ParamsSchema, nullopt, Annotations,
            [Callback](const JSON& Args,
                       const RequestHandlerExtra<ServerRequest, ServerNotification>& InExtra) {
                // TODO: Parse args to Args type
                Args ParsedArgs; // Would convert JSON to Args
                return Callback(ParsedArgs, InExtra);
            });
    }

    template <typename Args>
    RegisteredTool Tool(const string& Name, const string& Description, const JSON& ParamsSchema,
                        const ToolAnnotations& Annotations, const ToolCallback<Args>& Callback) {
        return CreateRegisteredTool(
            Name, Description, ParamsSchema, nullopt, Annotations,
            [Callback](const JSON& Args,
                       const RequestHandlerExtra<ServerRequest, ServerNotification>& InExtra) {
                // TODO: Parse args to Args type
                Args ParsedArgs; // Would convert JSON to Args
                return Callback(ParsedArgs, InExtra);
            });
    }

    template <typename InputArgs, typename OutputArgs>
    RegisteredTool RegisterTool(const string& Name, const unordered_map<string, JSON>& Config,
                                const ToolCallback<InputArgs>& Callback) {
        if (m_RegisteredTools.find(Name) != m_RegisteredTools.end()) {
            throw runtime_error("Tool " + Name + " is already registered");
        }

        optional<string> Description;
        optional<JSON> InputSchema;
        optional<JSON> OutputSchema;
        optional<ToolAnnotations> Annotations;

        auto DescIt = Config.find(MSG_DESCRIPTION);
        if (DescIt != Config.end()) { Description = DescIt->second.get<string>(); }

        auto InputIt = Config.find(MSG_INPUT_SCHEMA);
        if (InputIt != Config.end()) { InputSchema = InputIt->second; }

        auto OutputIt = Config.find("outputSchema");
        if (OutputIt != Config.end()) { OutputSchema = OutputIt->second; }

        auto AnnotIt = Config.find(MSG_ANNOTATIONS);
        if (AnnotIt != Config.end()) {
            // TODO: Parse annotations from JSON
        }

        return CreateRegisteredTool(
            Name, Description, InputSchema, OutputSchema, Annotations,
            [Callback](const JSON& Args,
                       const RequestHandlerExtra<ServerRequest, ServerNotification>& InExtra) {
                // TODO: Parse args to InputArgs type
                InputArgs ParsedArgs; // Would convert JSON to InputArgs
                return Callback(ParsedArgs, InExtra);
            });
    }

    // Prompt registration methods
    RegisteredPrompt Prompt(const string& Name, const PromptCallback<void>& Callback) {
        if (m_RegisteredPrompts.find(Name) != m_RegisteredPrompts.end()) {
            throw runtime_error("Prompt " + Name + " is already registered");
        }

        RegisteredPrompt Prompt;
        Prompt.Callback =
            [Callback](const JSON& Args,
                       const RequestHandlerExtra<ServerRequest, ServerNotification>& InExtra) {
                return Callback(InExtra);
            };
        Prompt.Enabled = true;

        m_RegisteredPrompts[Name] = Prompt;
        SetPromptRequestHandlers();
        SendPromptListChanged();

        return Prompt;
    }

    RegisteredPrompt Prompt(const string& Name, const string& Description,
                            const PromptCallback<void>& Callback) {
        if (m_RegisteredPrompts.find(Name) != m_RegisteredPrompts.end()) {
            throw runtime_error("Prompt " + Name + " is already registered");
        }

        RegisteredPrompt Prompt;
        Prompt.Description = Description;
        Prompt.Callback =
            [Callback](const JSON& Args,
                       const RequestHandlerExtra<ServerRequest, ServerNotification>& InExtra) {
                return Callback(InExtra);
            };
        Prompt.Enabled = true;

        m_RegisteredPrompts[Name] = Prompt;
        SetPromptRequestHandlers();
        SendPromptListChanged();

        return Prompt;
    }

    template <typename Args>
    RegisteredPrompt Prompt(const string& Name, const JSON& ArgsSchema,
                            const PromptCallback<Args>& Callback) {
        if (m_RegisteredPrompts.find(Name) != m_RegisteredPrompts.end()) {
            throw runtime_error("Prompt " + Name + " is already registered");
        }

        RegisteredPrompt Prompt;
        Prompt.ArgsSchema = ArgsSchema;
        Prompt.Callback =
            [Callback](const JSON& Args,
                       const RequestHandlerExtra<ServerRequest, ServerNotification>& InExtra) {
                // TODO: Parse args to Args type
                Args ParsedArgs; // Would convert JSON to Args
                return Callback(ParsedArgs, InExtra);
            };
        Prompt.Enabled = true;

        m_RegisteredPrompts[Name] = Prompt;
        SetPromptRequestHandlers();
        SendPromptListChanged();

        return Prompt;
    }

    template <typename Args>
    RegisteredPrompt Prompt(const string& Name, const string& Description, const JSON& ArgsSchema,
                            const PromptCallback<Args>& Callback) {
        if (m_RegisteredPrompts.find(Name) != m_RegisteredPrompts.end()) {
            throw runtime_error("Prompt " + Name + " is already registered");
        }

        RegisteredPrompt Prompt;
        Prompt.Description = Description;
        Prompt.ArgsSchema = ArgsSchema;
        Prompt.Callback =
            [Callback](const JSON& Args,
                       const RequestHandlerExtra<ServerRequest, ServerNotification>& InExtra) {
                // TODO: Parse args to Args type
                Args ParsedArgs; // Would convert JSON to Args
                return Callback(ParsedArgs, InExtra);
            };
        Prompt.Enabled = true;

        m_RegisteredPrompts[Name] = Prompt;
        SetPromptRequestHandlers();
        SendPromptListChanged();

        return Prompt;
    }

    /**
     * Sends a resource list changed event to the client, if connected.
     */
    void SendResourceListChanged() {
        if (IsConnected()) { m_ServerInstance->SendResourceListChanged(); }
    }

    /**
     * Sends a tool list changed event to the client, if connected.
     */
    void SendToolListChanged() {
        if (IsConnected()) { m_ServerInstance->SendToolListChanged(); }
    }

    /**
     * Sends a prompt list changed event to the client, if connected.
     */
    void SendPromptListChanged() {
        if (IsConnected()) { m_ServerInstance->SendPromptListChanged(); }
    }

    /* End MCP Server Interface */
  protected:
    void AssertCapabilityForMethod(const string& InMethod);
    void AssertNotificationCapability(const string& InMethod);
    void AssertRequestHandlerCapability(const string& InMethod);

  private:
    Implementation m_ServerInfo;
    ServerCapabilities m_Capabilities;
    optional<string> m_Instructions;
    optional<Implementation> m_ClientVersion;
    optional<ClientCapabilities> m_ClientCapabilities;

    // TODO: @HalcyonOmega Below from MCP Server Direct Translation
    unordered_map<string, RegisteredResource> m_RegisteredResources;
    unordered_map<string, RegisteredResourceTemplate> m_RegisteredResourceTemplates;
    unordered_map<string, RegisteredTool> m_RegisteredTools;
    unordered_map<string, RegisteredPrompt> m_RegisteredPrompts;

    bool m_ToolHandlersInitialized = false;
    bool m_CompletionHandlerInitialized = false;
    bool m_ResourceHandlersInitialized = false;
    bool m_PromptHandlersInitialized = false;
};

MCP_NAMESPACE_END