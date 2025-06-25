#pragma once

#include "CoreSDK/Common/Content.h"
#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Common/Roles.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

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
    MCP::Role Role;
    Content Content;

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

template <typename T>
concept IsPrompt = requires(T Type) {
    { Type.Name } -> std::convertible_to<std::string>;
    { Type.Description } -> std::same_as<std::optional<std::string>>;
    { Type.Arguments } -> std::same_as<std::optional<std::vector<PromptArgument>>>;
};

MCP_NAMESPACE_END