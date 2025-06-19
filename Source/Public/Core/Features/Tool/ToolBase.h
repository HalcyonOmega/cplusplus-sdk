#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// ToolAnnotations {
//   MSG_DESCRIPTION
//       : "Additional properties describing a Tool to clients.\n\nNOTE: all "
//         "properties in ToolAnnotations are **hints**.\nThey are not "
//         "guaranteed to provide a faithful description of\ntool behavior "
//         "(including descriptive properties like `title`).\n\nClients should "
//         "never make tool use decisions based on ToolAnnotations\nreceived "
//         "from untrusted servers.",
//         MSG_PROPERTIES
//       : {
//         "destructiveHint" : {
//           MSG_DESCRIPTION :
//               "If true, the tool may perform destructive updates to its "
//               "environment.\nIf false, the tool performs only additive "
//               "updates.\n\n(This property is meaningful only when "
//               "`readOnlyHint == false`)\n\nDefault: true",
//           MSG_TYPE : MSG_BOOLEAN
//         },
//         "idempotentHint" : {
//           MSG_DESCRIPTION :
//               "If true, calling the tool repeatedly with the same "
//               "arguments\nwill have no additional effect on the its "
//               "environment.\n\n(This property is meaningful only when "
//               "`readOnlyHint == false`)\n\nDefault: false",
//           MSG_TYPE : MSG_BOOLEAN
//         },
//         "openWorldHint" : {
//           MSG_DESCRIPTION :
//               "If true, this tool may interact with an \"open world\" of "
//               "external\nentities. If false, the tool's domain of interaction
//               " "is closed.\nFor example, the world of a web search tool is "
//               "open, whereas that\nof a memory tool is not.\n\nDefault:
//               true",
//           MSG_TYPE : MSG_BOOLEAN
//         },
//         "readOnlyHint" : {
//           MSG_DESCRIPTION : "If true, the tool does not modify its "
//                           "environment.\n\nDefault: false",
//           MSG_TYPE : MSG_BOOLEAN
//         },
//         "title" : {
//           MSG_DESCRIPTION : "A human-readable title for the tool.",
//           MSG_TYPE : MSG_STRING
//         }
//       },
//         MSG_TYPE : MSG_OBJECT
// };

// Additional properties describing a Tool to clients.
// NOTE: all properties in ToolAnnotations are **hints**.
// They are not guaranteed to provide a faithful description of
// tool behavior (including descriptive properties like `title`).
// Clients should never make tool use decisions based on ToolAnnotations
// received from untrusted servers.
struct ToolAnnotations {
    optional<string> Title; // A human-readable title for the tool.
    optional<bool>
        ReadOnlyHint; // If true, the tool does not modify its environment. Default: false
    optional<bool>
        DestructiveHint; // If true, the tool may perform destructive updates to its environment. If
                         // false, the tool performs only additive updates. (This property is
                         // meaningful only when `readOnlyHint == false`) Default: true
    optional<bool>
        IdempotentHint; // If true, calling the tool repeatedly with the same arguments will have no
                        // additional effect on the its environment. (This property is meaningful
                        // only when `readOnlyHint == false`) Default: false
    optional<bool> OpenWorldHint; // If true, this tool may interact with an "open world" of
                                  // external entities. If false, the tool's domain of interaction
                                  // is closed. For example, the world of a web search tool is open,
                                  // whereas that of a memory tool is not. Default: true
};

// Tool {
//   MSG_DESCRIPTION : "Definition for a tool the client can call.",
//                   MSG_PROPERTIES
//       : {
//         MSG_ANNOTATIONS : {
//           "$ref" : "#/definitions/ToolAnnotations",
//           MSG_DESCRIPTION : "Optional additional tool information."
//         },
//         MSG_DESCRIPTION : {
//           MSG_DESCRIPTION : "A human-readable description of the tool.\n\nThis
//           "
//                           "can be used by "
//                           "clients to improve the LLM's understanding of "
//                           "available tools. It "
//                           "can be thought of like a \"hint\" to the model.",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_INPUT_SCHEMA : {
//           MSG_DESCRIPTION : "A JSON Schema object defining the expected "
//                           "parameters for the tool.",
//           MSG_PROPERTIES : {
//             MSG_PROPERTIES : {
//               MSG_ADDITIONAL_PROPERTIES : {
//                 MSG_ADDITIONAL_PROPERTIES : true,
//                 MSG_PROPERTIES : {},
//                 MSG_TYPE : MSG_OBJECT
//               },
//               MSG_TYPE : MSG_OBJECT
//             },
//             MSG_REQUIRED : {MSG_ITEMS : {MSG_TYPE : MSG_STRING}, MSG_TYPE : MSG_ARRAY},
//             MSG_TYPE : {MSG_CONST : MSG_OBJECT, MSG_TYPE : MSG_STRING}
//           },
//           MSG_REQUIRED : [MSG_TYPE],
//           MSG_TYPE : MSG_OBJECT
//         },
//         MSG_NAME : {MSG_DESCRIPTION : "The name of the tool.", MSG_TYPE : MSG_STRING}
//       },
//         MSG_REQUIRED : [ MSG_INPUT_SCHEMA, MSG_NAME ],
//                      MSG_TYPE : MSG_OBJECT
// };

// Definition for a tool the client can call.
struct Tool {
    string Name;                  // The name of the tool.
    optional<string> Description; // A human-readable description of the tool. This can be used by
                                  // clients to improve the LLM's understanding of available tools.
                                  // It can be thought of like a "hint" to the model.
    JSONSchema InputSchema; // A JSON Schema object defining the expected parameters for the tool.
    optional<JSONSchema>
        OutputSchema; // An optional JSON object defining the structure of the tool's output
                      // returned in the StructuredContent field of a CallToolResult.
    optional<ToolAnnotations> Annotations; // Optional additional tool information.
};

MCP_NAMESPACE_END