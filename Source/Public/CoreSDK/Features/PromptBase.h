#pragma once

#include "CoreSDK/Common/Content.h"
#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Common/Roles.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

// PromptArgument {
//   MSG_DESCRIPTION : "Describes an argument that a prompt can accept.",
//                   MSG_PROPERTIES
//       : {
//         MSG_DESCRIPTION : {
//           MSG_DESCRIPTION : "A human-readable description of the argument.",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_NAME :
//             {MSG_DESCRIPTION : "The name of the argument.", MSG_TYPE : MSG_STRING},
//         MSG_REQUIRED : {
//           MSG_DESCRIPTION : "Whether this argument must be provided.",
//           MSG_TYPE : MSG_BOOLEAN
//         }
//       },
//         MSG_REQUIRED : [MSG_NAME],
//                      MSG_TYPE : MSG_OBJECT
// };

/**
 * Describes an argument that a prompt can accept.
 */
struct PromptArgument {
    std::string Name;                       // The name of the argument.
    std::optional<std::string> Description; // A human-readable description of the argument.
    std::optional<bool> Required;           // Whether this argument must be provided.

    JKEY(NAMEKEY, Name, "name")
    JKEY(DESCRIPTIONKEY, Description, "description")
    JKEY(REQUIREDKEY, Required, "required")

    DEFINE_TYPE_JSON(PromptArgument, NAMEKEY, DESCRIPTIONKEY, REQUIREDKEY)
};

// PromptMessage {
//   MSG_DESCRIPTION
//       : "Describes a message returned as part of a prompt.\n\nThis is "
//         "similar to `SamplingMessage`, but also supports the embedding "
//         "of\nresources from the MCP server.",
//         MSG_PROPERTIES : {
//           MSG_CONTENT : {
//             "anyOf" : [
//               {"$ref" : "#/definitions/TextContent"},
//               {"$ref" : "#/definitions/ImageContent"},
//               {"$ref" : "#/definitions/AudioContent"},
//               {"$ref" : "#/definitions/EmbeddedResource"}
//             ]
//           },
//           MSG_ROLE : {"$ref" : "#/definitions/ERole"}
//         },
//                        MSG_REQUIRED : [ MSG_CONTENT, MSG_ROLE ],
//                                     MSG_TYPE : MSG_OBJECT
// };

/**
 * Describes a message returned as part of a prompt.
 */
struct PromptMessage {
    MCP::ERole Role;
    std::variant<TextContent, ImageContent, AudioContent, EmbeddedResource> Content;

    JKEY(ROLEKEY, Role, "role")
    JKEY(CONTENTKEY, Content, "content")

    DEFINE_TYPE_JSON(PromptMessage, ROLEKEY, CONTENTKEY)
};

// Prompt {
//   MSG_DESCRIPTION : "A prompt or prompt template that the server offers.",
//                   MSG_PROPERTIES
//       : {
//         MSG_ARGUMENTS : {
//           MSG_DESCRIPTION :
//               "A list of arguments to use for templating the prompt.",
//           MSG_ITEMS : {"$ref" : "#/definitions/PromptArgument"},
//           MSG_TYPE : MSG_ARRAY
//         },
//         MSG_DESCRIPTION : {
//           MSG_DESCRIPTION :
//               "An optional description of what this prompt provides",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_NAME : {
//           MSG_DESCRIPTION : "The name of the prompt or prompt template.",
//           MSG_TYPE : MSG_STRING
//         }
//       },
//         MSG_REQUIRED : [MSG_NAME],
//                      MSG_TYPE : MSG_OBJECT
// };

/**
 * A prompt or prompt template that the server offers.
 */
struct Prompt {
    std::string Name;                       // The name of the prompt or prompt template.
    std::optional<std::string> Description; // An optional description of what this prompt provides.
    std::optional<std::vector<PromptArgument>>
        Arguments; // A list of arguments to use for templating the prompt.

    JKEY(NAMEKEY, Name, "name")
    JKEY(DESCRIPTIONKEY, Description, "description")
    JKEY(ARGUMENTSKEY, Arguments, "arguments")

    DEFINE_TYPE_JSON(Prompt, NAMEKEY, DESCRIPTIONKEY, ARGUMENTSKEY)

    bool operator<(const Prompt& InOther) const {
        if (Name != InOther.Name) { return Name < InOther.Name; }
        if (Description != InOther.Description) { return Description < InOther.Description; }
        return false;
    }

    bool operator==(const Prompt& InOther) const {
        return Name == InOther.Name && Description == InOther.Description;
    }
};

template <typename T>
concept PromptType = requires(T Type) {
    { Type.Name } -> std::convertible_to<std::string>;
    { Type.Description } -> std::same_as<std::optional<std::string>>;
    { Type.Arguments } -> std::same_as<std::optional<std::vector<PromptArgument>>>;
};

MCP_NAMESPACE_END