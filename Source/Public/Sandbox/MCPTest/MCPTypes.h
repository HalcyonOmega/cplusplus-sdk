#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "json.hpp"

// Wrapper macro for enum JSON serialization (follows codebase convention)
#define DEFINE_ENUM_JSON(EnumType, ...) NLOHMANN_JSON_SERIALIZE_ENUM(EnumType, __VA_ARGS__)

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
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Annotations, Audience, Priority)

// Content types
struct TextContent {
    std::string Type = "text";
    std::string Text;
    std::optional<Annotations> ContentAnnotations;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TextContent, Type, Text, ContentAnnotations)

struct ImageContent {
    std::string Type = "image";
    std::string Data; // base64
    std::string MimeType;
    std::optional<Annotations> ContentAnnotations;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ImageContent, Type, Data, MimeType,
                                                ContentAnnotations)

struct AudioContent {
    std::string Type = "audio";
    std::string Data; // base64
    std::string MimeType;
    std::optional<Annotations> ContentAnnotations;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AudioContent, Type, Data, MimeType,
                                                ContentAnnotations)

struct EmbeddedResource {
    std::string Type = "resource";
    std::optional<Annotations> ContentAnnotations;
    struct Resource {
        std::string URI;
        std::optional<std::string> Text;
        std::optional<std::string> MimeType;
    } ResourceData;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(EmbeddedResource::Resource, URI, Text, MimeType)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(EmbeddedResource, Type, ContentAnnotations,
                                                ResourceData)

using Content = std::variant<TextContent, ImageContent, AudioContent, EmbeddedResource>;

// JSON Schema
struct JSONSchema {
    std::string Type = "object";
    std::optional<std::unordered_map<std::string, JSONValue>> Properties;
    std::optional<std::vector<std::string>> Required;
    std::optional<JSONValue> AdditionalProperties;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(JSONSchema, Type, Properties, Required,
                                                AdditionalProperties)

// Tool types
struct ToolAnnotations {
    std::optional<std::vector<Role>> Audience;
    std::optional<double> Priority;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ToolAnnotations, Audience, Priority)

struct Tool {
    std::string Name;
    std::optional<std::string> Description;
    JSONSchema InputSchema;
    std::optional<ToolAnnotations> ToolAnnotations;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Tool, Name, Description, InputSchema,
                                                ToolAnnotations)

// Prompt types
struct PromptArgument {
    std::string Name;
    std::optional<std::string> Description;
    std::optional<bool> Required;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PromptArgument, Name, Description, Required)

struct Prompt {
    std::string Name;
    std::optional<std::string> Description;
    std::optional<std::vector<PromptArgument>> Arguments;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Prompt, Name, Description, Arguments)

// Resource types
struct Resource {
    std::string URI;
    std::string Name;
    std::optional<std::string> Description;
    std::optional<std::string> MimeType;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Resource, URI, Name, Description, MimeType)

struct ResourceTemplate {
    std::string URITemplate;
    std::string Name;
    std::optional<std::string> Description;
    std::optional<std::string> MimeType;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ResourceTemplate, URITemplate, Name, Description,
                                                MimeType)

// Root
struct Root {
    std::string URI;
    std::optional<std::string> Name;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Root, URI, Name)

// Model preferences for sampling
struct ModelPreferences {
    std::optional<std::vector<struct ModelHint>> Hints;
    std::optional<double> CostPriority;         // 0-1
    std::optional<double> SpeedPriority;        // 0-1
    std::optional<double> IntelligencePriority; // 0-1
};

struct ModelHint {
    std::optional<std::string> Name;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ModelHint, Name)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ModelPreferences, Hints, CostPriority,
                                                SpeedPriority, IntelligencePriority)

// Sampling message
struct SamplingMessage {
    Role MessageRole;
    Content MessageContent;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SamplingMessage, MessageRole, MessageContent)

// Progress token
struct ProgressToken {
    std::variant<std::string, int64_t> Token;
};

// Individual capability structures (defined before they are used)
struct RootsCapability {
    std::optional<bool> ListChanged;
};

struct SamplingCapability {
    // Empty object in spec
};

struct ExperimentalCapability {
    std::unordered_map<std::string, JSONValue> Features;
};

struct LoggingCapability {
    // Empty object in spec
};

struct PromptsCapability {
    std::optional<bool> ListChanged;
};

struct ResourcesCapability {
    std::optional<bool> Subscribe;
    std::optional<bool> ListChanged;
};

struct ToolsCapability {
    std::optional<bool> ListChanged;
};

// Now define the main capability containers
struct ClientCapabilities {
    std::optional<RootsCapability> Roots;
    std::optional<SamplingCapability> Sampling;
    std::optional<std::unordered_map<std::string, JSONValue>> Experimental;
};

struct ServerCapabilities {
    std::optional<ExperimentalCapability> Experimental;
    std::optional<LoggingCapability> Logging;
    std::optional<PromptsCapability> Prompts;
    std::optional<ResourcesCapability> Resources;
    std::optional<ToolsCapability> Tools;
};

// JSON serialization for all capability types
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RootsCapability, ListChanged)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SamplingCapability)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ExperimentalCapability, Features)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LoggingCapability)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PromptsCapability, ListChanged)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ResourcesCapability, Subscribe, ListChanged)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ToolsCapability, ListChanged)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ClientCapabilities, Roots, Sampling, Experimental)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ServerCapabilities, Experimental, Logging, Prompts,
                                                Resources, Tools)

// Implementation info
struct Implementation {
    std::string Name;
    std::string Version;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Implementation, Name, Version)