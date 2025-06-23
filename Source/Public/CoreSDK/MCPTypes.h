#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "../Macros.h"
#include "../Proxies/JSONProxy.h"
#include "../Utilities/ThirdParty/json.hpp"

MCP_NAMESPACE_BEGIN

using JSONValue = nlohmann::json;
using RequestID = std::variant<std::string, int64_t>;

// Error structure
struct MCPError {
    int64_t Code;
    std::string Message;
    std::optional<JSONValue> Data;

    JKEY(CODEKEY, Code, "code")
    JKEY(MESSAGEKEY, Message, "message")
    JKEY(DATAKEY, Data, "data")

    DEFINE_TYPE_JSON(MCPError, CODEKEY, MESSAGEKEY, DATAKEY)
};

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
struct ToolAnnotations {
    std::optional<std::vector<Role>> Audience;
    std::optional<double> Priority;

    JKEY(AUDIENCEKEY, Audience, "audience")
    JKEY(PRIORITYKEY, Priority, "priority")

    DEFINE_TYPE_JSON(ToolAnnotations, AUDIENCEKEY, PRIORITYKEY)
};

struct Tool {
    std::string Name;
    std::optional<std::string> Description;
    JSONSchema InputSchema;
    std::optional<ToolAnnotations> ToolAnnotations;

    JKEY(NAMEKEY, Name, "name")
    JKEY(DESCRIPTIONKEY, Description, "description")
    JKEY(INPUTSCHEMAKEY, InputSchema, "inputSchema")
    JKEY(TOOLANNOTATIONSKEY, ToolAnnotations, "annotations")

    DEFINE_TYPE_JSON(Tool, NAMEKEY, DESCRIPTIONKEY, INPUTSCHEMAKEY, TOOLANNOTATIONSKEY)
};

// Prompt types
struct PromptArgument {
    std::string Name;
    std::optional<std::string> Description;
    std::optional<bool> Required;

    JKEY(NAMEKEY, Name, "name")
    JKEY(DESCRIPTIONKEY, Description, "description")
    JKEY(REQUIREDKEY, Required, "required")

    DEFINE_TYPE_JSON(PromptArgument, NAMEKEY, DESCRIPTIONKEY, REQUIREDKEY)
};

struct Prompt {
    std::string Name;
    std::optional<std::string> Description;
    std::optional<std::vector<PromptArgument>> Arguments;

    JKEY(NAMEKEY, Name, "name")
    JKEY(DESCRIPTIONKEY, Description, "description")
    JKEY(ARGUMENTSKEY, Arguments, "arguments")

    DEFINE_TYPE_JSON(Prompt, NAMEKEY, DESCRIPTIONKEY, ARGUMENTSKEY)
};

// Resource types
struct Resource {
    std::string URI;
    std::string Name;
    std::optional<std::string> Description;
    std::optional<std::string> MimeType;

    JKEY(URIKEY, URI, "uri")
    JKEY(NAMEKEY, Name, "name")
    JKEY(DESCRIPTIONKEY, Description, "description")
    JKEY(MIMETYPEKEY, MimeType, "mimeType")

    DEFINE_TYPE_JSON(Resource, URIKEY, NAMEKEY, DESCRIPTIONKEY, MIMETYPEKEY)
};

struct ResourceTemplate {
    std::string URITemplate;
    std::string Name;
    std::optional<std::string> Description;
    std::optional<std::string> MimeType;

    JKEY(URITEMPLATEKEY, URITemplate, "uriTemplate")
    JKEY(NAMEKEY, Name, "name")
    JKEY(DESCRIPTIONKEY, Description, "description")
    JKEY(MIMETYPEKEY, MimeType, "mimeType")

    DEFINE_TYPE_JSON(ResourceTemplate, URITEMPLATEKEY, NAMEKEY, DESCRIPTIONKEY, MIMETYPEKEY)
};

// Root
struct Root {
    std::string URI;
    std::optional<std::string> Name;

    JKEY(URIKEY, URI, "uri")
    JKEY(NAMEKEY, Name, "name")

    DEFINE_TYPE_JSON(Root, URIKEY, NAMEKEY)
};

// Model preferences for sampling
struct ModelHint {
    std::optional<std::string> Name;

    JKEY(NAMEKEY, Name, "name")

    DEFINE_TYPE_JSON(ModelHint, NAMEKEY)
};

struct ModelPreferences {
    std::optional<std::vector<ModelHint>> Hints;
    std::optional<double> CostPriority;         // 0-1
    std::optional<double> SpeedPriority;        // 0-1
    std::optional<double> IntelligencePriority; // 0-1

    JKEY(HINTSKEY, Hints, "hints")
    JKEY(COSTPRIORITYKEY, CostPriority, "costPriority")
    JKEY(SPEEDPRIORITYKEY, SpeedPriority, "speedPriority")
    JKEY(INTELLIGENCEPRIORITYKEY, IntelligencePriority, "intelligencePriority")

    DEFINE_TYPE_JSON(ModelPreferences, HINTSKEY, COSTPRIORITYKEY, SPEEDPRIORITYKEY,
                     INTELLIGENCEPRIORITYKEY)
};

// Sampling message
struct SamplingMessage {
    Role MessageRole;
    Content MessageContent;

    JKEY(ROLENAMEKEY, MessageRole, "role")
    JKEY(CONTENTKEY, MessageContent, "content")

    DEFINE_TYPE_JSON(SamplingMessage, ROLENAMEKEY, CONTENTKEY)
};

// Progress token
struct ProgressToken {
    std::variant<std::string, int64_t> Token;
};

// Individual capability structures (defined before they are used)
struct RootsCapability {
    std::optional<bool> ListChanged;

    JKEY(LISTCHANGEDKEY, ListChanged, "listChanged")

    DEFINE_TYPE_JSON(RootsCapability, LISTCHANGEDKEY)
};

struct SamplingCapability {
    // Empty object in spec

    static void to_json(nlohmann::json& JSON, const SamplingCapability& Capability) {
        (void)Capability;
        JSON = nlohmann::json::object();
    }

    static void from_json(const nlohmann::json& JSON, SamplingCapability& Capability) {
        (void)JSON;
        (void)Capability;
    }
};

struct ExperimentalCapability {
    std::unordered_map<std::string, JSONValue> Features;

    JKEY(FEATURESKEY, Features, "features")

    DEFINE_TYPE_JSON(ExperimentalCapability, FEATURESKEY)
};

struct LoggingCapability {
    static void to_json(nlohmann::json& JSON, const LoggingCapability& Capability) {
        (void)Capability;
        JSON = nlohmann::json::object();
    }

    static void from_json(const nlohmann::json& JSON, LoggingCapability& Capability) {
        (void)JSON;
        (void)Capability;
    }
};

struct PromptsCapability {
    std::optional<bool> ListChanged;

    JKEY(LISTCHANGEDKEY, ListChanged, "listChanged")

    DEFINE_TYPE_JSON(PromptsCapability, LISTCHANGEDKEY)
};

struct ResourcesCapability {
    std::optional<bool> Subscribe;
    std::optional<bool> ListChanged;

    JKEY(SUBSCRIBEKEY, Subscribe, "subscribe")
    JKEY(LISTCHANGEDKEY, ListChanged, "listChanged")

    DEFINE_TYPE_JSON(ResourcesCapability, SUBSCRIBEKEY, LISTCHANGEDKEY)
};

struct ToolsCapability {
    std::optional<bool> ListChanged;

    JKEY(LISTCHANGEDKEY, ListChanged, "listChanged")

    DEFINE_TYPE_JSON(ToolsCapability, LISTCHANGEDKEY)
};

// Now define the main capability containers
struct ClientCapabilities {
    std::optional<RootsCapability> Roots;
    std::optional<SamplingCapability> Sampling;
    std::optional<std::unordered_map<std::string, JSONValue>> Experimental;

    JKEY(ROOTSKEY, Roots, "roots")
    JKEY(SAMPLINGKEY, Sampling, "sampling")
    JKEY(EXPERIMENTALKEY, Experimental, "experimental")

    DEFINE_TYPE_JSON(ClientCapabilities, ROOTSKEY, SAMPLINGKEY, EXPERIMENTALKEY)
};

struct ServerCapabilities {
    std::optional<ExperimentalCapability> Experimental;
    std::optional<LoggingCapability> Logging;
    std::optional<PromptsCapability> Prompts;
    std::optional<ResourcesCapability> Resources;
    std::optional<ToolsCapability> Tools;

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