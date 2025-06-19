#pragma once

#include "Core.h"
#include "Core/Types/Content.h"
#include "Core/Types/Roles.h"

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

// Describes an argument that a prompt can accept.
struct PromptArgument {
    string Name;                     // The name of the argument.
    optional<string> Description;    // A human-readable description of the argument.
    optional<bool> Required;         // Whether this argument must be provided.
    AdditionalProperties Additional; // Additional properties.
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
//           MSG_ROLE : {"$ref" : "#/definitions/Role"}
//         },
//                        MSG_REQUIRED : [ MSG_CONTENT, MSG_ROLE ],
//                                     MSG_TYPE : MSG_OBJECT
// };

// Describes a message returned as part of a prompt.
struct PromptMessage {
    Role Role;
    variant<TextContent, ImageContent, AudioContent, EmbeddedResource> Content;
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

// A prompt or prompt template that the server offers.
struct Prompt {
    string Name;                  // The name of the prompt or prompt template.
    optional<string> Description; // An optional description of what this prompt provides.
    optional<vector<PromptArgument>>
        Arguments; // A list of arguments to use for templating the prompt.
};

MCP_NAMESPACE_END