#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "../Proxies/JSONProxy.h"
#include "../Proxies/URIProxy.h"
#include "../Utilities/ThirdParty/json.hpp"
#include "CoreSDK/Common/Macros.h"

MCP_NAMESPACE_BEGIN

using JSONValue = nlohmann::json;
using RequestID = std::variant<std::string, int64_t>;

// Role enumeration
enum class Role { User, Assistant };

DEFINE_ENUM_JSON(Role, {{Role::User, "user"}, {Role::Assistant, "assistant"}})

// Logging levels
enum class LoggingLevel { Debug, Info, Notice, Warning, Error, Critical, Alert, Emergency };

DEFINE_ENUM_JSON(LoggingLevel, {{LoggingLevel::Debug, "debug"},
                                {LoggingLevel::Info, "info"},
                                {LoggingLevel::Notice, "notice"},
                                {LoggingLevel::Warning, "warning"},
                                {LoggingLevel::Error, "error"},
                                {LoggingLevel::Critical, "critical"},
                                {LoggingLevel::Alert, "alert"},
                                {LoggingLevel::Emergency, "emergency"}})

// Annotations
struct Annotations {
    std::optional<std::vector<Role>> Audience;
    std::optional<double> Priority; // 0-1 range

    JKEY(AUDIENCEKEY, Audience, "audience")
    JKEY(PRIORITYKEY, Priority, "priority")

    DEFINE_TYPE_JSON(Annotations, AUDIENCEKEY, PRIORITYKEY)
};

// Content types
struct TextContent {
    std::string Type{"text"};
    std::string Text;
    std::optional<Annotations> ContentAnnotations;

    JKEY(TYPEKEY, Type, "type")
    JKEY(TEXTKEY, Text, "text")
    JKEY(ANNOTATIONSKEY, ContentAnnotations, "annotations")

    DEFINE_TYPE_JSON(TextContent, TYPEKEY, TEXTKEY, ANNOTATIONSKEY)
};

struct ImageContent {
    std::string Type{"image"};
    std::string Data; // base64
    std::string MimeType;
    std::optional<Annotations> ContentAnnotations;

    JKEY(TYPEKEY, Type, "type")
    JKEY(DATAKEY, Data, "data")
    JKEY(MIMETYPEKEY, MimeType, "mimeType")
    JKEY(ANNOTATIONSKEY, ContentAnnotations, "annotations")

    DEFINE_TYPE_JSON(ImageContent, TYPEKEY, DATAKEY, MIMETYPEKEY, ANNOTATIONSKEY)
};

struct AudioContent {
    std::string Type{"audio"};
    std::string Data; // base64
    std::string MimeType;
    std::optional<Annotations> ContentAnnotations;

    JKEY(TYPEKEY, Type, "type")
    JKEY(DATAKEY, Data, "data")
    JKEY(MIMETYPEKEY, MimeType, "mimeType")
    JKEY(ANNOTATIONSKEY, ContentAnnotations, "annotations")

    DEFINE_TYPE_JSON(AudioContent, TYPEKEY, DATAKEY, MIMETYPEKEY, ANNOTATIONSKEY)
};

struct EmbeddedResource {
    std::string Type{"resource"};
    std::optional<Annotations> ContentAnnotations;
    struct Resource {
        std::string URI;
        std::optional<std::string> Text;
        std::optional<std::string> MimeType;

        JKEY(URIKEY, URI, "uri")
        JKEY(TEXTKEY, Text, "text")
        JKEY(MIMETYPEKEY, MimeType, "mimeType")

        DEFINE_TYPE_JSON(Resource, URIKEY, TEXTKEY, MIMETYPEKEY)
    } ResourceData;

    JKEY(TYPEKEY, Type, "type")
    JKEY(ANNOTATIONSKEY, ContentAnnotations, "annotations")
    JKEY(RESOURCEDATAKEY, ResourceData, "resource")

    DEFINE_TYPE_JSON(EmbeddedResource, TYPEKEY, ANNOTATIONSKEY, RESOURCEDATAKEY)
};

using Content = std::variant<TextContent, ImageContent, AudioContent, EmbeddedResource>;

// JSON Schema
struct JSONSchema {
    std::string Type{"object"};
    std::optional<std::unordered_map<std::string, JSONValue>> Properties;
    std::optional<std::vector<std::string>> Required;
    std::optional<JSONValue> AdditionalProperties;

    JKEY(TYPEKEY, Type, "type")
    JKEY(PROPERTIESKEY, Properties, "properties")
    JKEY(REQUIREDKEY, Required, "required")
    JKEY(ADDITIONALPROPERTIESKEY, AdditionalProperties, "additionalProperties")

    DEFINE_TYPE_JSON(JSONSchema, TYPEKEY, PROPERTIESKEY, REQUIREDKEY, ADDITIONALPROPERTIESKEY)
};

// Tool types

// Additional properties describing a Tool to clients.
// NOTE: all properties in ToolAnnotations are **hints**.
// They are not guaranteed to provide a faithful description of
// tool behavior (including descriptive properties like `title`).
// Clients should never make tool use decisions based on ToolAnnotations
// received from untrusted servers.
struct ToolAnnotations {
    std::optional<std::string> Title; // A human-readable title for the tool.
    std::optional<bool>
        ReadOnlyHint; // If true, the tool does not modify its environment. Default: false
    std::optional<bool> DestructiveHint; // If true, the tool may perform destructive updates to
                                         // its environment. If false, the tool performs only
                                         // additive updates. (This property is meaningful only
                                         // when `readOnlyHint == false`) Default: true
    std::optional<bool>
        IdempotentHint; // If true, calling the tool repeatedly with the same arguments will
                        // have no additional effect on the its environment. (This property is
                        // meaningful only when `readOnlyHint == false`) Default: false
    std::optional<bool>
        OpenWorldHint; // If true, this tool may interact with an "open world" of
                       // external entities. If false, the tool's domain of interaction
                       // is closed. For example, the world of a web search tool is open,
                       // whereas that of a memory tool is not. Default: true

    JKEY(TITLEKEY, Title, "title")
    JKEY(READONLYHINTKEY, ReadOnlyHint, "readOnlyHint")
    JKEY(DESTRUCTIVEHINTKEY, DestructiveHint, "destructiveHint")
    JKEY(IDEMPOTENTHINTKEY, IdempotentHint, "idempotentHint")
    JKEY(OPENWORLDHINTKEY, OpenWorldHint, "openWorldHint")

    DEFINE_TYPE_JSON(ToolAnnotations, TITLEKEY, READONLYHINTKEY, DESTRUCTIVEHINTKEY,
                     IDEMPOTENTHINTKEY, OPENWORLDHINTKEY)
};

// Definition for a tool the client can call.
struct Tool {
    std::string Name; // The name of the tool.
    std::optional<std::string>
        Description;        // A human-readable description of the tool. This can be used by
                            // clients to improve the LLM's understanding of available tools.
                            // It can be thought of like a "hint" to the model.
    JSONSchema InputSchema; // A JSON Schema object defining the expected parameters for the tool.
    std::optional<JSONSchema>
        OutputSchema; // An optional JSON object defining the structure of the tool's output
                      // returned in the StructuredContent field of a CallToolResult.
    std::optional<ToolAnnotations> Annotations; // Optional additional tool information.

    JKEY(NAMEKEY, Name, "name")
    JKEY(DESCRIPTIONKEY, Description, "description")
    JKEY(INPUTSCHEMAKEY, InputSchema, "inputSchema")
    JKEY(OUTPUTSCHEMAKEY, OutputSchema, "outputSchema")
    JKEY(ANNOTATIONSKEY, Annotations, "annotations")

    DEFINE_TYPE_JSON(Tool, NAMEKEY, DESCRIPTIONKEY, INPUTSCHEMAKEY, OUTPUTSCHEMAKEY, ANNOTATIONSKEY)
};

// Prompt types

// Describes an argument that a prompt can accept.
struct PromptArgument {
    std::string Name;                       // The name of the argument.
    std::optional<std::string> Description; // A human-readable description of the argument.
    std::optional<bool> Required;           // Whether this argument must be provided.

    JKEY(NAMEKEY, Name, "name")
    JKEY(DESCRIPTIONKEY, Description, "description")
    JKEY(REQUIREDKEY, Required, "required")

    DEFINE_TYPE_JSON(PromptArgument, NAMEKEY, DESCRIPTIONKEY, REQUIREDKEY)
};

// Describes a message returned as part of a prompt.
struct PromptMessage {
    Role Role;
    std::variant<TextContent, ImageContent, AudioContent, EmbeddedResource> Content;

    JKEY(ROLEKEY, Role, "role")
    JKEY(CONTENTKEY, Content, "content")

    DEFINE_TYPE_JSON(PromptMessage, ROLEKEY, CONTENTKEY)
};

// A prompt or prompt template that the server offers.
struct Prompt {
    std::string Name;                       // The name of the prompt or prompt template.
    std::optional<std::string> Description; // An optional description of what this prompt provides.
    std::optional<std::vector<PromptArgument>>
        Arguments; // A list of arguments to use for templating the prompt.

    JKEY(NAMEKEY, Name, "name")
    JKEY(DESCRIPTIONKEY, Description, "description")
    JKEY(ARGUMENTSKEY, Arguments, "arguments")

    DEFINE_TYPE_JSON(Prompt, NAMEKEY, DESCRIPTIONKEY, ARGUMENTSKEY)
};

// Resource types

// A known resource that the server is capable of reading.
struct Resource {
    URI URI;          // The URI of this resource.
    std::string Name; // A human-readable name for this resource. This can be used by clients to
                      // populate UI elements.
    std::optional<std::string>
        Description; // A description of what this resource represents. This can be
                     // used by clients to improve the LLM's understanding of available
                     // resources. It can be thought of like a "hint" to the model.
    std::optional<std::string> MIMEType;    // The MIME type of this resource, if known.
    std::optional<Annotations> Annotations; // Optional annotations for the client.
    std::optional<int64_t>
        Size; // The size of the raw resource content, in bytes (i.e., before base64
              // encoding or any tokenization), if known. This can be used by Hosts
              // to display file sizes and estimate context window usage.

    JKEY(URIKEY, URI, "uri")
    JKEY(NAMEKEY, Name, "name")
    JKEY(DESCRIPTIONKEY, Description, "description")
    JKEY(MIMETYPEKEY, MimeType, "mimeType")
    JKEY(ANNOTATIONSKEY, Annotations, "annotations")
    JKEY(SIZEKEY, Size, "size")

    DEFINE_TYPE_JSON(Resource, URIKEY, NAMEKEY, DESCRIPTIONKEY, MIMETYPEKEY, ANNOTATIONSKEY,
                     SIZEKEY)
};

// A template description for resources available on the server.
struct ResourceTemplate {
    URITemplate URITemplate; // A URI template (according to RFC 6570) that can be used to
                             // construct resource URIs.
    std::string Name; // A human-readable name for the type of resource this template refers to.
                      // This can be used by clients to populate UI elements.
    std::optional<std::string> Description; // A description of what this template is for. This
                                            // can be used by clients to improve the LLM's
                                            // understanding of available resources. It can be
                                            // thought of like a "hint" to the model.
    std::optional<std::string>
        MIMEType; // The MIME type for all resources that match this template. This should only
                  // be included if all resources matching this template have the same type.
    std::optional<Annotations> Annotations; // Optional annotations for the client.

    JKEY(URITEMPLATEKEY, URITemplate, "uriTemplate")
    JKEY(NAMEKEY, Name, "name")
    JKEY(DESCRIPTIONKEY, Description, "description")
    JKEY(MIMETYPEKEY, MimeType, "mimeType")
    JKEY(ANNOTATIONSKEY, Annotations, "annotations")

    DEFINE_TYPE_JSON(ResourceTemplate, URITEMPLATEKEY, NAMEKEY, DESCRIPTIONKEY, MIMETYPEKEY,
                     ANNOTATIONSKEY)
};

// Root

// Represents a root directory or file that the server can operate on.
struct Root {
    URIFile URI; // The URI identifying the root. This *must* start with file:// for now. This
                 // restriction may be relaxed in future versions of the protocol to allow other
                 // URI schemes.
    std::optional<std::string>
        Name; // An optional name for the root. This can be used to provide a human-readable
              // identifier for the root, which may be useful for display purposes or for
              // referencing the root in other parts of the application.

    JKEY(URIKEY, URI, "uri")
    JKEY(NAMEKEY, Name, "name")

    DEFINE_TYPE_JSON(Root, URIKEY, NAMEKEY)
};

// Hints to use for model selection. Keys not declared here are currently left unspecified by
// the spec and are up to the client to interpret.
struct ModelHint {
    std::optional<std::string>
        Name; // A hint for a model name. The client SHOULD treat this as a substring
              // of a model name; for example: - `claude-3-5-sonnet` should match
              // `claude-3-5-sonnet-20241022` - `sonnet` should match
              // `claude-3-5-sonnet-20241022`, `claude-3-sonnet-20240229`, etc. - `claude`
              // should match any Claude model The client MAY also map the string to a different
              // provider's model name or a different model family, as long as it fills a
              // similar niche; for example: - `gemini-1.5-flash` could match
              // `claude-3-haiku-20240307`

    JKEY(NAMEKEY, Name, "name")

    DEFINE_TYPE_JSON(ModelHint, NAMEKEY)
};

// The server's preferences for model selection, requested of the client during sampling.
// Because LLMs can vary along multiple dimensions, choosing the "best" model is rarely
// straightforward. Different models excel in different areas—some are faster but less capable,
// others are more capable but more expensive, and so on. This interface allows servers to
// express their priorities across multiple dimensions to help clients make an appropriate
// selection for their use case. These preferences are always advisory. The client MAY ignore
// them. It is also up to the client to decide how to interpret these preferences and how to
// balance them against other considerations.
struct ModelPreferences {
    std::optional<std::vector<ModelHint>>
        Hints; // Optional hints to use for model selection. If multiple hints are specified,
               // the client MUST evaluate them in order (such that the first match is taken).
               // The client SHOULD prioritize these hints over the numeric priorities, but MAY
               // still use the priorities to select from ambiguous matches.
    // TODO: @HalcyonOmega Enforce min = 0, max = 1
    std::optional<double> CostPriority; // How much to prioritize cost when selecting a model. A
                                        // value of 0 means cost is not important, while a value
                                        // of 1 means cost is the most important factor.
    // TODO: @HalcyonOmega Enforce min = 0, max = 1
    std::optional<double>
        SpeedPriority; // How much to prioritize sampling speed (latency) when
                       // selecting a model. A value of 0 means speed is not important,
                       // while a value of 1 means speed is the most important factor.
    // TODO: @HalcyonOmega Enforce min = 0, max = 1
    std::optional<double>
        IntelligencePriority; // How much to prioritize intelligence and capabilities
                              // when selecting a model. A value of 0 means
                              // intelligence is not important, while a value of 1
                              // means intelligence is the most important factor.

    JKEY(HINTSKEY, Hints, "hints")
    JKEY(COSTPRIORITYKEY, CostPriority, "costPriority")
    JKEY(SPEEDPRIORITYKEY, SpeedPriority, "speedPriority")
    JKEY(INTELLIGENCEPRIORITYKEY, IntelligencePriority, "intelligencePriority")

    DEFINE_TYPE_JSON(ModelPreferences, HINTSKEY, COSTPRIORITYKEY, SPEEDPRIORITYKEY,
                     INTELLIGENCEPRIORITYKEY)
};

// Describes a message issued to or received from an LLM API.
struct SamplingMessage {
    Role Role;
    std::variant<TextContent, ImageContent, AudioContent> Content; // The content of the message.

    JKEY(ROLEKEY, Role, "role")
    JKEY(CONTENTKEY, Content, "content")

    DEFINE_TYPE_JSON(SamplingMessage, ROLEKEY, CONTENTKEY)
};

// Progress token

// A progress token, used to associate progress notifications with the original request.
// TODO: @HalcyonOmega - Relook handling - schema does not have any subfields
struct ProgressToken {
    std::variant<std::string, int64_t> Token;

    JKEY(TOKENKEY, Token, "token")

    DEFINE_TYPE_JSON(ProgressToken, TOKENKEY)
};

// Individual capability structures (defined before they are used)
struct RootsCapability {
    std::optional<bool>
        ListChanged; // Whether the client supports notifications for changes to the roots list.
    JSONValue AdditionalProperties;

    JKEY(LISTCHANGEDKEY, ListChanged, "listChanged")
    JKEY(ADDITIONALPROPERTIESKEY, AdditionalProperties, "additionalProperties")

    DEFINE_TYPE_JSON(RootsCapability, LISTCHANGEDKEY, ADDITIONALPROPERTIESKEY)
};

struct SamplingCapability {
    JSONValue AdditionalProperties;

    JKEY(ADDITIONALPROPERTIESKEY, AdditionalProperties, "additionalProperties")

    DEFINE_TYPE_JSON(SamplingCapability, ADDITIONALPROPERTIESKEY)
};

struct ExperimentalCapability {
    JSONValue AdditionalProperties;

    JKEY(ADDITIONALPROPERTIESKEY, AdditionalProperties, "additionalProperties")

    DEFINE_TYPE_JSON(ExperimentalCapability, ADDITIONALPROPERTIESKEY)
};

struct LoggingCapability {
    JSONValue AdditionalProperties;

    JKEY(ADDITIONALPROPERTIESKEY, AdditionalProperties, "additionalProperties")

    DEFINE_TYPE_JSON(LoggingCapability, ADDITIONALPROPERTIESKEY)
};

struct PromptsCapability {
    std::optional<bool> ListChanged; // Whether this server supports notifications for changes
                                     // to the prompt list.
    JSONValue AdditionalProperties;

    JKEY(LISTCHANGEDKEY, ListChanged, "listChanged")
    JKEY(ADDITIONALPROPERTIESKEY, AdditionalProperties, "additionalProperties")

    DEFINE_TYPE_JSON(PromptsCapability, LISTCHANGEDKEY, ADDITIONALPROPERTIESKEY)
};

struct ResourcesCapability {
    std::optional<bool> Subscribe; // Whether this server supports subscribing to resource updates.
    std::optional<bool> ListChanged; // Whether this server supports notifications for changes
                                     // to the resource list.
    JSONValue AdditionalProperties;

    JKEY(SUBSCRIBEKEY, Subscribe, "subscribe")
    JKEY(LISTCHANGEDKEY, ListChanged, "listChanged")
    JKEY(ADDITIONALPROPERTIESKEY, AdditionalProperties, "additionalProperties")

    DEFINE_TYPE_JSON(ResourcesCapability, SUBSCRIBEKEY, LISTCHANGEDKEY, ADDITIONALPROPERTIESKEY)
};

struct CompletionCapability {
    JSONValue AdditionalProperties;

    JKEY(ADDITIONALPROPERTIESKEY, AdditionalProperties, "additionalProperties")

    DEFINE_TYPE_JSON(CompletionCapability, ADDITIONALPROPERTIESKEY)
};

struct ToolsCapability {
    std::optional<bool>
        ListChanged; // Whether this server supports notifications for changes to the tool list.
    JSONValue AdditionalProperties;

    JKEY(LISTCHANGEDKEY, ListChanged, "listChanged")
    JKEY(ADDITIONALPROPERTIESKEY, AdditionalProperties, "additionalProperties")

    DEFINE_TYPE_JSON(ToolsCapability, LISTCHANGEDKEY, ADDITIONALPROPERTIESKEY)
};

// Capabilities a client may support. Known capabilities are defined here, in this schema, but
// this is not a closed set: any client can define its own, additional capabilities.
struct ClientCapabilities {
    std::optional<ExperimentalCapability>
        Experimental; // Experimental, non-standard capabilities that the client supports.
    std::optional<SamplingCapability>
        Sampling;                         // Present if the client supports sampling from an LLM.
    std::optional<RootsCapability> Roots; // Present if the client supports listing roots.

    JKEY(ROOTSKEY, Roots, "roots")
    JKEY(SAMPLINGKEY, Sampling, "sampling")
    JKEY(EXPERIMENTALKEY, Experimental, "experimental")

    DEFINE_TYPE_JSON(ClientCapabilities, ROOTSKEY, SAMPLINGKEY, EXPERIMENTALKEY)
};

// Capabilities that a server may support. Known capabilities are defined here, in this schema,
// but this is not a closed set: any server can define its own, additional capabilities.
struct ServerCapabilities {
    std::optional<ExperimentalCapability>
        Experimental; // Experimental, non-standard capabilities that the server supports.
    std::optional<LoggingCapability>
        Logging; // Present if the server supports sending log messages to the client.
    std::optional<CompletionCapability>
        Completions; // Present if the server supports sending completions to the client.
    std::optional<PromptsCapability> Prompts; // Present if the server offers any prompt templates.
    std::optional<ResourcesCapability>
        Resources;                        // Present if the server offers any resources to read.
    std::optional<ToolsCapability> Tools; // Present if the server offers any tools to call.

    JKEY(EXPERIMENTALKEY, Experimental, "experimental")
    JKEY(LOGGINGKEY, Logging, "logging")
    JKEY(PROMPTSKEY, Prompts, "prompts")
    JKEY(RESOURCESKEY, Resources, "resources")
    JKEY(TOOLSKEY, Tools, "tools")

    DEFINE_TYPE_JSON(ServerCapabilities, EXPERIMENTALKEY, LOGGINGKEY, PROMPTSKEY, RESOURCESKEY,
                     TOOLSKEY)
};

// Implementation info
struct Implementation {
    std::string Name;
    std::string Version;

    JKEY(NAMEKEY, Name, "name")
    JKEY(VERSIONKEY, Version, "version")

    DEFINE_TYPE_JSON(Implementation, NAMEKEY, VERSIONKEY)
};

MCP_NAMESPACE_END