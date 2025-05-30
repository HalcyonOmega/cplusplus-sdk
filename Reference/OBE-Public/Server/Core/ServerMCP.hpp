#pragma once

#include "Constants.h"
#include "Core.h"

// Additional Includes
#include "Client/Core/Client.hpp" // For AjvValidator
#include "Utilities/URI/URI_Template.h"

MCP_NAMESPACE_BEGIN

// Forward declarations
struct Tool;
struct ListToolsResult;
struct CallToolResult;
struct CompleteRequest;
struct CompleteResult;
struct PromptReference;
struct ResourceReference;
struct Resource;
struct ListResourcesResult;
struct ListResourceTemplatesResult;
struct ReadResourceResult;
struct ListPromptsResult;
struct Prompt;
struct PromptArgument;
struct GetPromptResult;
struct ServerRequest;
struct ServerNotification;
struct ToolAnnotations;
class Server;
struct RequestHandlerExtra;

/**
 * Callback to complete one variable within a resource template's URI template.
 */
using CompleteResourceTemplateCallback = function<vector<string>(const string&)>;

/**
 * Additional, optional information for annotating a resource.
 */
struct ResourceMetadata {
    optional<string> Name;
    optional<string> Description;
    optional<string> MimeType;
    // Additional metadata fields can be added here
};

/**
 * Callback to list all resources matching a given template.
 */
using ListResourcesCallback =
    function<ListResourcesResult>(const RequestHandlerExtra<ServerRequest, ServerNotification>&);

/**
 * Callback to read a resource at a given URI.
 */
using ReadResourceCallback = function<ReadResourceResult(
    const string& uri, const RequestHandlerExtra<ServerRequest, ServerNotification>&)>;

/**
 * Callback to read a resource at a given URI, following a filled-in URI template.
 */
using ReadResourceTemplateCallback =
    function<ReadResourceResult(const string& uri, const unordered_map<string, string>& variables,
                                const RequestHandlerExtra<ServerRequest, ServerNotification>&)>;

/**
 * A resource template combines a URI pattern with optional functionality to enumerate
 * all resources matching that pattern.
 */
class ResourceTemplate {
  private:
    string URI_Template_;
    optional<ListResourcesCallback> ListCallback_;
    unordered_map<string, CompleteResourceTemplateCallback> CompleteCallbacks_;

  public:
    ResourceTemplate(const string& uriTemplate, const optional<ListResourcesCallback>& listCallback,
                     const optional<unordered_map<string, CompleteResourceTemplateCallback>>&
                         completeCallbacks = nullopt)
        : URI_Template_(uriTemplate), ListCallback_(listCallback) {
        if (completeCallbacks) {
            CompleteCallbacks_ = *completeCallbacks;
        }
    }

    /**
     * Gets the URI template pattern.
     */
    const string& GetURI_Template() const {
        return URI_Template_;
    }

    /**
     * Gets the list callback, if one was provided.
     */
    const optional<ListResourcesCallback>& GetListCallback() const {
        return ListCallback_;
    }

    /**
     * Gets the callback for completing a specific URI template variable, if one was provided.
     */
    optional<CompleteResourceTemplateCallback> GetCompleteCallback(const string& variable) const {
        auto it = CompleteCallbacks_.find(variable);
        return it != CompleteCallbacks_.end()
                   ? optional<CompleteResourceTemplateCallback>(it->second)
                   : nullopt;
    }

    /**
     * Matches a URI against this template and returns variables if successful.
     */
    optional<unordered_map<string, string>> Match(const string& uri) const {
        URI_Template uriTemplate(URI_Template_);
        auto variables = uriTemplate.Match(uri);

        if (variables.empty()) {
            return nullopt;
        }

        // Convert Variables type to unordered_map<string, string>
        unordered_map<string, string> result;
        for (const auto& [key, value] : variables) {
            if (std::holds_alternative<string>(value)) {
                result[key] = std::get<string>(value);
            } else if (std::holds_alternative<vector<string>>(value)) {
                // Join vector values with comma for compatibility
                const auto& values = std::get<vector<string>>(value);
                string joined;
                for (size_t i = 0; i < values.size(); ++i) {
                    if (i > 0)
                        joined += ",";
                    joined += values[i];
                }
                result[key] = joined;
            }
        }
        return result;
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

// Forward declarations for registered types
struct RegisteredTool;
struct RegisteredResource;
struct RegisteredResourceTemplate;
struct RegisteredPrompt;

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
        Update({{MSG_KEY_NAME, nullptr}});
    }

    void Update(const unordered_map<string, JSON>& updates) {
        // Implementation for updating tool properties
        for (const auto& [key, value] : updates) {
            if (key == "enabled" && value.is_boolean()) {
                Enabled = value.get<bool>();
            }
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
        Update({{MSG_KEY_URI, nullptr}});
    }

    void Update(const unordered_map<string, JSON>& updates) {
        // Implementation for updating resource properties
        for (const auto& [key, value] : updates) {
            if (key == "enabled" && value.is_boolean()) {
                Enabled = value.get<bool>();
            }
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
        Update({{MSG_KEY_NAME, nullptr}});
    }

    void Update(const unordered_map<string, JSON>& updates) {
        // Implementation for updating resource template properties
        for (const auto& [key, value] : updates) {
            if (key == "enabled" && value.is_boolean()) {
                Enabled = value.get<bool>();
            }
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
        Update({{MSG_KEY_NAME, nullptr}});
    }

    void Update(const unordered_map<string, JSON>& updates) {
        // Implementation for updating prompt properties
        for (const auto& [key, value] : updates) {
            if (key == "enabled" && value.is_boolean()) {
                Enabled = value.get<bool>();
            }
            // Handle other update fields
        }
        // Trigger prompt list changed notification
    }
};

/**
 * High-level MCP server that provides a simpler API for working with resources, tools, and prompts.
 * For advanced usage (like sending notifications or setting custom request handlers), use the
 * underlying Server instance available via the GetServer() method.
 */
class MCPServer {
  public:
    /**
     * The underlying Server instance, useful for advanced operations like sending notifications.
     */
    shared_ptr<Server> GetServer() const {
        return ServerInstance_;
    }

  private:
    shared_ptr<Server> ServerInstance_;
    unordered_map<string, RegisteredResource> RegisteredResources_;
    unordered_map<string, RegisteredResourceTemplate> RegisteredResourceTemplates_;
    unordered_map<string, RegisteredTool> RegisteredTools_;
    unordered_map<string, RegisteredPrompt> RegisteredPrompts_;

    bool ToolHandlersInitialized_ = false;
    bool CompletionHandlerInitialized_ = false;
    bool ResourceHandlersInitialized_ = false;
    bool PromptHandlersInitialized_ = false;

    static const JSON EmptyObjectJSONSchema_;
    static const CompleteResult EmptyCompletionResult_;

    // JSON Schema validator for input/output validation
    AjvValidator SchemaValidator_;

  public:
    MCPServer(const Implementation& serverInfo, const optional<ServerOptions>& options = nullopt) {
        ServerInstance_ = make_shared<Server>(serverInfo, options);
    }

    /**
     * Attaches to the given transport, starts it, and starts listening for messages.
     *
     * The server object assumes ownership of the Transport, replacing any callbacks that have
     * already been set, and expects that it is the only user of the Transport instance going
     * forward.
     */
    future<void> Connect(shared_ptr<Transport> transport) {
        return ServerInstance_->Connect(transport);
    }

    /**
     * Closes the connection.
     */
    future<void> Close() {
        return ServerInstance_->Close();
    }

  private:
    void SetToolRequestHandlers() {
        if (ToolHandlersInitialized_) {
            return;
        }

        // Assert can set request handlers
        ServerInstance_->AssertCanSetRequestHandler(MTHD_TOOLS_LIST);
        ServerInstance_->AssertCanSetRequestHandler(MTHD_TOOLS_CALL);

        // Register capabilities
        ServerCapabilities caps;
        // TODO: Set up tools capability with listChanged = true
        ServerInstance_->RegisterCapabilities(caps);

        // Set handler for tools/list
        ServerInstance_->SetRequestHandler(
            MTHD_TOOLS_LIST,
            [this](const JSON& request,
                   const RequestHandlerExtra<ServerRequest, ServerNotification>& extra)
                -> ListToolsResult {
                ListToolsResult result;

                for (const auto& [name, tool] : RegisteredTools_) {
                    if (!tool.Enabled)
                        continue;

                    Tool toolDef;
                    toolDef.Name = name;
                    toolDef.Description = tool.Description;
                    toolDef.InputSchema = tool.InputSchema.value_or(EmptyObjectJSONSchema_);
                    if (tool.OutputSchema) {
                        toolDef.OutputSchema = *tool.OutputSchema;
                    }
                    toolDef.Annotations = tool.Annotations;

                    result.Tools.push_back(toolDef);
                }

                return result;
            });

        // Set handler for tools/call
        ServerInstance_->SetRequestHandler(
            MTHD_TOOLS_CALL,
            [this](const JSON& request,
                   const RequestHandlerExtra<ServerRequest, ServerNotification>& extra)
                -> CallToolResult {
                auto toolName = request[MSG_KEY_PARAMS][MSG_KEY_NAME].get<string>();
                auto toolArgs = request[MSG_KEY_PARAMS].value(MSG_KEY_ARGUMENTS, JSON::object());

                auto it = RegisteredTools_.find(toolName);
                if (it == RegisteredTools_.end()) {
                    throw MCP_Error(ErrorCode::InvalidParams, "Tool " + toolName + " not found");
                }

                const auto& tool = it->second;
                if (!tool.Enabled) {
                    throw MCP_Error(ErrorCode::InvalidParams, "Tool " + toolName + " disabled");
                }

                CallToolResult result;
                try {
                    // Validate input against InputSchema if present
                    if (tool.InputSchema) {
                        auto validator = SchemaValidator_.Compile(*tool.InputSchema);
                        if (!validator(toolArgs)) {
                            throw MCP_Error(
                                ErrorCode::InvalidParams,
                                "Invalid arguments for tool " + toolName + ": "
                                    + SchemaValidator_.ErrorsText(SchemaValidator_.Errors));
                        }
                    }

                    result = tool.Callback(toolArgs, extra);

                    // Validate output against OutputSchema if present
                    if (tool.OutputSchema && result.StructuredContent) {
                        auto validator = SchemaValidator_.Compile(*tool.OutputSchema);
                        if (!validator(*result.StructuredContent)) {
                            throw MCP_Error(
                                ErrorCode::InvalidParams,
                                "Invalid structured content for tool " + toolName + ": "
                                    + SchemaValidator_.ErrorsText(SchemaValidator_.Errors));
                        }
                    }

                    return result;
                } catch (const exception& e) {
                    CallToolResult errorResult;
                    errorResult.IsError = true;
                    // Create error content
                    Content errorContent;
                    errorContent.Type = "text";
                    errorContent.Text = e.what();
                    errorResult.Content.push_back(errorContent);
                    return errorResult;
                }
            });

        ToolHandlersInitialized_ = true;
    }

    void SetCompletionRequestHandler() {
        if (CompletionHandlerInitialized_) {
            return;
        }

        // Assert can set request handler
        ServerInstance_->AssertCanSetRequestHandler("completion/complete");

        ServerInstance_->SetRequestHandler(
            "completion/complete",
            [this](const JSON& request,
                   const RequestHandlerExtra<ServerRequest, ServerNotification>& extra)
                -> CompleteResult {
                auto refType = request[MSG_KEY_PARAMS]["ref"][MSG_KEY_TYPE].get<string>();

                if (refType == "ref/prompt") {
                    return HandlePromptCompletion(request, extra);
                } else if (refType == "ref/resource") {
                    return HandleResourceCompletion(request, extra);
                } else {
                    throw MCP_Error(ErrorCode::InvalidParams,
                                    "Invalid completion reference: " + refType);
                }
            });

        CompletionHandlerInitialized_ = true;
    }

    CompleteResult
    HandlePromptCompletion(const JSON& request,
                           const RequestHandlerExtra<ServerRequest, ServerNotification>& extra) {
        auto promptName = request[MSG_KEY_PARAMS]["ref"][MSG_KEY_NAME].get<string>();

        auto it = RegisteredPrompts_.find(promptName);
        if (it == RegisteredPrompts_.end()) {
            throw MCP_Error(ErrorCode::InvalidParams, "Prompt " + promptName + " not found");
        }

        const auto& prompt = it->second;
        if (!prompt.Enabled) {
            throw MCP_Error(ErrorCode::InvalidParams, "Prompt " + promptName + " disabled");
        }

        if (!prompt.ArgsSchema) {
            return EmptyCompletionResult_;
        }

        // TODO: Implement completion logic for prompt arguments
        return EmptyCompletionResult_;
    }

    CompleteResult
    HandleResourceCompletion(const JSON& request,
                             const RequestHandlerExtra<ServerRequest, ServerNotification>& extra) {
        auto uri = request[MSG_KEY_PARAMS]["ref"][MSG_KEY_URI].get<string>();

        // Find matching template
        for (const auto& [name, templateEntry] : RegisteredResourceTemplates_) {
            auto variables = templateEntry.Template.Match(uri);
            if (variables) {
                auto argName = request[MSG_KEY_PARAMS]["argument"][MSG_KEY_NAME].get<string>();
                auto completer = templateEntry.Template.GetCompleteCallback(argName);
                if (completer) {
                    auto argValue = request[MSG_KEY_PARAMS]["argument"]["value"].get<string>();
                    auto suggestions = (*completer)(argValue);
                    return CreateCompletionResult(suggestions);
                }
            }
        }

        // Check if it's a fixed resource URI
        if (RegisteredResources_.find(uri) != RegisteredResources_.end()) {
            return EmptyCompletionResult_;
        }

        throw MCP_Error(ErrorCode::InvalidParams, "Resource template " + uri + " not found");
    }

    void SetResourceRequestHandlers() {
        if (ResourceHandlersInitialized_) {
            return;
        }

        // Assert can set request handlers
        ServerInstance_->AssertCanSetRequestHandler("resources/list");
        ServerInstance_->AssertCanSetRequestHandler("resources/templates/list");
        ServerInstance_->AssertCanSetRequestHandler("resources/read");

        // Register capabilities
        ServerCapabilities caps;
        // TODO: Set up resources capability with listChanged = true
        ServerInstance_->RegisterCapabilities(caps);

        // Set handler for resources/list
        ServerInstance_->SetRequestHandler(
            "resources/list",
            [this](const JSON& request,
                   const RequestHandlerExtra<ServerRequest, ServerNotification>& extra)
                -> ListResourcesResult {
                ListResourcesResult result;

                // Add fixed resources
                for (const auto& [uri, resource] : RegisteredResources_) {
                    if (!resource.Enabled)
                        continue;

                    Resource res;
                    res.Uri = uri;
                    res.Name = resource.Name;
                    if (resource.Metadata) {
                        // TODO: Copy metadata fields
                    }
                    result.Resources.push_back(res);
                }

                // Add template resources
                for (const auto& [name, templateEntry] : RegisteredResourceTemplates_) {
                    if (templateEntry.Template.GetListCallback()) {
                        auto templateResult = (*templateEntry.Template.GetListCallback())(extra);
                        for (const auto& resource : templateResult.Resources) {
                            result.Resources.push_back(resource);
                        }
                    }
                }

                return result;
            });

        // Set handler for resources/templates/list
        ServerInstance_->SetRequestHandler(
            "resources/templates/list",
            [this](const JSON& request,
                   const RequestHandlerExtra<ServerRequest, ServerNotification>& extra)
                -> ListResourceTemplatesResult {
                ListResourceTemplatesResult result;

                for (const auto& [name, templateEntry] : RegisteredResourceTemplates_) {
                    ResourceTemplate resTmpl;
                    resTmpl.Name = name;
                    resTmpl.URI_Template = templateEntry.Template.GetURI_Template();
                    if (templateEntry.Metadata) {
                        // TODO: Copy metadata fields
                    }
                    result.ResourceTemplates.push_back(resTmpl);
                }

                return result;
            });

        // Set handler for resources/read
        ServerInstance_->SetRequestHandler(
            "resources/read",
            [this](const JSON& request,
                   const RequestHandlerExtra<ServerRequest, ServerNotification>& extra)
                -> ReadResourceResult {
                auto uri = request[MSG_KEY_PARAMS][MSG_KEY_URI].get<string>();

                // Check for exact resource match first
                auto resourceIt = RegisteredResources_.find(uri);
                if (resourceIt != RegisteredResources_.end()) {
                    if (!resourceIt->second.Enabled) {
                        throw MCP_Error(ErrorCode::InvalidParams, "Resource " + uri + " disabled");
                    }
                    return resourceIt->second.Callback(uri, extra);
                }

                // Check templates
                for (const auto& [name, templateEntry] : RegisteredResourceTemplates_) {
                    auto variables = templateEntry.Template.Match(uri);
                    if (variables) {
                        return templateEntry.Callback(uri, *variables, extra);
                    }
                }

                throw MCP_Error(ErrorCode::InvalidParams, "Resource " + uri + " not found");
            });

        SetCompletionRequestHandler();
        ResourceHandlersInitialized_ = true;
    }

    void SetPromptRequestHandlers() {
        if (PromptHandlersInitialized_) {
            return;
        }

        // Assert can set request handlers
        ServerInstance_->AssertCanSetRequestHandler("prompts/list");
        ServerInstance_->AssertCanSetRequestHandler("prompts/get");

        // Register capabilities
        ServerCapabilities caps;
        // TODO: Set up prompts capability with listChanged = true
        ServerInstance_->RegisterCapabilities(caps);

        // Set handler for prompts/list
        ServerInstance_->SetRequestHandler(
            "prompts/list",
            [this](const JSON& request,
                   const RequestHandlerExtra<ServerRequest, ServerNotification>& extra)
                -> ListPromptsResult {
                ListPromptsResult result;

                for (const auto& [name, prompt] : RegisteredPrompts_) {
                    if (!prompt.Enabled)
                        continue;

                    Prompt promptDef;
                    promptDef.Name = name;
                    promptDef.Description = prompt.Description;
                    if (prompt.ArgsSchema) {
                        // TODO: Convert JSON schema to PromptArgument vector
                    }

                    result.Prompts.push_back(promptDef);
                }

                return result;
            });

        // Set handler for prompts/get
        ServerInstance_->SetRequestHandler(
            "prompts/get",
            [this](const JSON& request,
                   const RequestHandlerExtra<ServerRequest, ServerNotification>& extra)
                -> GetPromptResult {
                auto promptName = request[MSG_KEY_PARAMS][MSG_KEY_NAME].get<string>();
                auto args = request[MSG_KEY_PARAMS].value(MSG_KEY_ARGUMENTS, JSON::object());

                auto it = RegisteredPrompts_.find(promptName);
                if (it == RegisteredPrompts_.end()) {
                    throw MCP_Error(ErrorCode::InvalidParams,
                                    "Prompt " + promptName + " not found");
                }

                const auto& prompt = it->second;
                if (!prompt.Enabled) {
                    throw MCP_Error(ErrorCode::InvalidParams, "Prompt " + promptName + " disabled");
                }

                // Validate arguments against ArgsSchema if present
                if (prompt.ArgsSchema) {
                    auto validator = SchemaValidator_.Compile(*prompt.ArgsSchema);
                    if (!validator(args)) {
                        throw MCP_Error(ErrorCode::InvalidParams,
                                        "Invalid arguments for prompt " + promptName + ": "
                                            + SchemaValidator_.ErrorsText(SchemaValidator_.Errors));
                    }
                }

                return prompt.Callback(args, extra);
            });

        SetCompletionRequestHandler();
        PromptHandlersInitialized_ = true;
    }

    RegisteredTool CreateRegisteredTool(
        const string& name, const optional<string>& description, const optional<JSON>& inputSchema,
        const optional<JSON>& outputSchema, const optional<ToolAnnotations>& annotations,
        const function<CallToolResult(
            const JSON&, const RequestHandlerExtra<ServerRequest, ServerNotification>&)>&
            callback) {
        RegisteredTool tool;
        tool.Description = description;
        tool.InputSchema = inputSchema;
        tool.OutputSchema = outputSchema;
        tool.Annotations = annotations;
        tool.Callback = callback;
        tool.Enabled = true;

        RegisteredTools_[name] = tool;
        SetToolRequestHandlers();
        SendToolListChanged();

        return tool;
    }

    static CompleteResult CreateCompletionResult(const vector<string>& suggestions) {
        CompleteResult result;

        // Limit to maximum 100 suggestions as per MCP protocol
        size_t maxCount = min(suggestions.size(), static_cast<size_t>(100));
        vector<string> limitedValues(suggestions.begin(), suggestions.begin() + maxCount);

        // TODO: Set up completion structure with values, total, and hasMore
        // result.Completion.Values = limitedValues;
        // result.Completion.Total = suggestions.size();
        // result.Completion.HasMore = suggestions.size() > 100;

        return result;
    }

    static vector<PromptArgument> PromptArgumentsFromSchema(const JSON& schema) {
        vector<PromptArgument> args;
        // TODO: Convert JSON schema to PromptArgument vector
        return args;
    }

  public:
    // Resource registration methods
    RegisteredResource Resource(const string& name, const string& uri,
                                const ReadResourceCallback& callback) {
        return Resource(name, uri, nullopt, callback);
    }

    RegisteredResource Resource(const string& name, const string& uri,
                                const optional<ResourceMetadata>& metadata,
                                const ReadResourceCallback& callback) {
        if (RegisteredResources_.find(uri) != RegisteredResources_.end()) {
            throw runtime_error("Resource " + uri + " is already registered");
        }

        RegisteredResource resource;
        resource.Name = name;
        resource.Metadata = metadata;
        resource.Callback = callback;
        resource.Enabled = true;

        RegisteredResources_[uri] = resource;
        SetResourceRequestHandlers();
        SendResourceListChanged();

        return resource;
    }

    RegisteredResourceTemplate Resource(const string& name, const ResourceTemplate& tmpl,
                                        const ReadResourceTemplateCallback& callback) {
        return Resource(name, tmpl, nullopt, callback);
    }

    RegisteredResourceTemplate Resource(const string& name, const ResourceTemplate& tmpl,
                                        const optional<ResourceMetadata>& metadata,
                                        const ReadResourceTemplateCallback& callback) {
        if (RegisteredResourceTemplates_.find(name) != RegisteredResourceTemplates_.end()) {
            throw runtime_error("Resource template " + name + " is already registered");
        }

        RegisteredResourceTemplate resourceTemplate(tmpl);
        resourceTemplate.Metadata = metadata;
        resourceTemplate.Callback = callback;
        resourceTemplate.Enabled = true;

        RegisteredResourceTemplates_[name] = resourceTemplate;
        SetResourceRequestHandlers();
        SendResourceListChanged();

        return resourceTemplate;
    }

    // Tool registration methods
    RegisteredTool Tool(const string& name, const ToolCallback<void>& callback) {
        return CreateRegisteredTool(
            name, nullopt, nullopt, nullopt, nullopt,
            [callback](const JSON& args,
                       const RequestHandlerExtra<ServerRequest, ServerNotification>& extra) {
                return callback(extra);
            });
    }

    RegisteredTool Tool(const string& name, const string& description,
                        const ToolCallback<void>& callback) {
        return CreateRegisteredTool(
            name, description, nullopt, nullopt, nullopt,
            [callback](const JSON& args,
                       const RequestHandlerExtra<ServerRequest, ServerNotification>& extra) {
                return callback(extra);
            });
    }

    template <typename Args>
    RegisteredTool Tool(const string& name, const JSON& paramsSchemaOrAnnotations,
                        const ToolCallback<Args>& callback) {
        // TODO: Determine if paramsSchemaOrAnnotations is schema or annotations
        return CreateRegisteredTool(
            name, nullopt, paramsSchemaOrAnnotations, nullopt, nullopt,
            [callback](const JSON& args,
                       const RequestHandlerExtra<ServerRequest, ServerNotification>& extra) {
                // TODO: Parse args to Args type
                Args parsedArgs; // Would convert JSON to Args
                return callback(parsedArgs, extra);
            });
    }

    template <typename Args>
    RegisteredTool Tool(const string& name, const string& description,
                        const JSON& paramsSchemaOrAnnotations, const ToolCallback<Args>& callback) {
        // TODO: Determine if paramsSchemaOrAnnotations is schema or annotations
        return CreateRegisteredTool(
            name, description, paramsSchemaOrAnnotations, nullopt, nullopt,
            [callback](const JSON& args,
                       const RequestHandlerExtra<ServerRequest, ServerNotification>& extra) {
                // TODO: Parse args to Args type
                Args parsedArgs; // Would convert JSON to Args
                return callback(parsedArgs, extra);
            });
    }

    template <typename Args>
    RegisteredTool Tool(const string& name, const JSON& paramsSchema,
                        const ToolAnnotations& annotations, const ToolCallback<Args>& callback) {
        return CreateRegisteredTool(
            name, nullopt, paramsSchema, nullopt, annotations,
            [callback](const JSON& args,
                       const RequestHandlerExtra<ServerRequest, ServerNotification>& extra) {
                // TODO: Parse args to Args type
                Args parsedArgs; // Would convert JSON to Args
                return callback(parsedArgs, extra);
            });
    }

    template <typename Args>
    RegisteredTool Tool(const string& name, const string& description, const JSON& paramsSchema,
                        const ToolAnnotations& annotations, const ToolCallback<Args>& callback) {
        return CreateRegisteredTool(
            name, description, paramsSchema, nullopt, annotations,
            [callback](const JSON& args,
                       const RequestHandlerExtra<ServerRequest, ServerNotification>& extra) {
                // TODO: Parse args to Args type
                Args parsedArgs; // Would convert JSON to Args
                return callback(parsedArgs, extra);
            });
    }

    template <typename InputArgs, typename OutputArgs>
    RegisteredTool RegisterTool(const string& name, const unordered_map<string, JSON>& config,
                                const ToolCallback<InputArgs>& callback) {
        if (RegisteredTools_.find(name) != RegisteredTools_.end()) {
            throw runtime_error("Tool " + name + " is already registered");
        }

        optional<string> description;
        optional<JSON> inputSchema;
        optional<JSON> outputSchema;
        optional<ToolAnnotations> annotations;

        auto descIt = config.find(MSG_KEY_DESCRIPTION);
        if (descIt != config.end()) {
            description = descIt->second.get<string>();
        }

        auto inputIt = config.find(MSG_KEY_INPUT_SCHEMA);
        if (inputIt != config.end()) {
            inputSchema = inputIt->second;
        }

        auto outputIt = config.find("outputSchema");
        if (outputIt != config.end()) {
            outputSchema = outputIt->second;
        }

        auto annotIt = config.find("annotations");
        if (annotIt != config.end()) {
            // TODO: Parse annotations from JSON
        }

        return CreateRegisteredTool(
            name, description, inputSchema, outputSchema, annotations,
            [callback](const JSON& args,
                       const RequestHandlerExtra<ServerRequest, ServerNotification>& extra) {
                // TODO: Parse args to InputArgs type
                InputArgs parsedArgs; // Would convert JSON to InputArgs
                return callback(parsedArgs, extra);
            });
    }

    // Prompt registration methods
    RegisteredPrompt Prompt(const string& name, const PromptCallback<void>& callback) {
        if (RegisteredPrompts_.find(name) != RegisteredPrompts_.end()) {
            throw runtime_error("Prompt " + name + " is already registered");
        }

        RegisteredPrompt prompt;
        prompt.Callback =
            [callback](const JSON& args,
                       const RequestHandlerExtra<ServerRequest, ServerNotification>& extra) {
                return callback(extra);
            };
        prompt.Enabled = true;

        RegisteredPrompts_[name] = prompt;
        SetPromptRequestHandlers();
        SendPromptListChanged();

        return prompt;
    }

    RegisteredPrompt Prompt(const string& name, const string& description,
                            const PromptCallback<void>& callback) {
        if (RegisteredPrompts_.find(name) != RegisteredPrompts_.end()) {
            throw runtime_error("Prompt " + name + " is already registered");
        }

        RegisteredPrompt prompt;
        prompt.Description = description;
        prompt.Callback =
            [callback](const JSON& args,
                       const RequestHandlerExtra<ServerRequest, ServerNotification>& extra) {
                return callback(extra);
            };
        prompt.Enabled = true;

        RegisteredPrompts_[name] = prompt;
        SetPromptRequestHandlers();
        SendPromptListChanged();

        return prompt;
    }

    template <typename Args>
    RegisteredPrompt Prompt(const string& name, const JSON& argsSchema,
                            const PromptCallback<Args>& callback) {
        if (RegisteredPrompts_.find(name) != RegisteredPrompts_.end()) {
            throw runtime_error("Prompt " + name + " is already registered");
        }

        RegisteredPrompt prompt;
        prompt.ArgsSchema = argsSchema;
        prompt.Callback =
            [callback](const JSON& args,
                       const RequestHandlerExtra<ServerRequest, ServerNotification>& extra) {
                // TODO: Parse args to Args type
                Args parsedArgs; // Would convert JSON to Args
                return callback(parsedArgs, extra);
            };
        prompt.Enabled = true;

        RegisteredPrompts_[name] = prompt;
        SetPromptRequestHandlers();
        SendPromptListChanged();

        return prompt;
    }

    template <typename Args>
    RegisteredPrompt Prompt(const string& name, const string& description, const JSON& argsSchema,
                            const PromptCallback<Args>& callback) {
        if (RegisteredPrompts_.find(name) != RegisteredPrompts_.end()) {
            throw runtime_error("Prompt " + name + " is already registered");
        }

        RegisteredPrompt prompt;
        prompt.Description = description;
        prompt.ArgsSchema = argsSchema;
        prompt.Callback =
            [callback](const JSON& args,
                       const RequestHandlerExtra<ServerRequest, ServerNotification>& extra) {
                // TODO: Parse args to Args type
                Args parsedArgs; // Would convert JSON to Args
                return callback(parsedArgs, extra);
            };
        prompt.Enabled = true;

        RegisteredPrompts_[name] = prompt;
        SetPromptRequestHandlers();
        SendPromptListChanged();

        return prompt;
    }

    /**
     * Checks if the server is connected to a transport.
     * @returns True if the server is connected
     */
    bool IsConnected() const {
        return ServerInstance_->GetTransport() != nullptr;
    }

    /**
     * Sends a resource list changed event to the client, if connected.
     */
    void SendResourceListChanged() {
        if (IsConnected()) {
            ServerInstance_->SendResourceListChanged();
        }
    }

    /**
     * Sends a tool list changed event to the client, if connected.
     */
    void SendToolListChanged() {
        if (IsConnected()) {
            ServerInstance_->SendToolListChanged();
        }
    }

    /**
     * Sends a prompt list changed event to the client, if connected.
     */
    void SendPromptListChanged() {
        if (IsConnected()) {
            ServerInstance_->SendPromptListChanged();
        }
    }
};

// Static member definitions
const JSON MCPServer::EmptyObjectJSONSchema_ = JSON{{MSG_KEY_TYPE, "object"}};
const CompleteResult MCPServer::EmptyCompletionResult_ =
    CompleteResult{/* TODO: Initialize empty completion result */};

MCP_NAMESPACE_END