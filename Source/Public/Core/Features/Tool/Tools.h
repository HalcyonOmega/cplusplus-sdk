#pragma once

#include "Core.h"
#include "Core/Constants/MessageConstants.h"
#include "Core/Constants/MethodConstants.h"

MCP_NAMESPACE_BEGIN

// Forward Declarations
// TODO: @HalcyonOmega create JSONSchema class. Needed to define ToolInputSchema and
// ToolOutputSchema.
class JSONSchema;

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
//           MSG_TYPE : "boolean"
//         },
//         "idempotentHint" : {
//           MSG_DESCRIPTION :
//               "If true, calling the tool repeatedly with the same "
//               "arguments\nwill have no additional effect on the its "
//               "environment.\n\n(This property is meaningful only when "
//               "`readOnlyHint == false`)\n\nDefault: false",
//           MSG_TYPE : "boolean"
//         },
//         "openWorldHint" : {
//           MSG_DESCRIPTION :
//               "If true, this tool may interact with an \"open world\" of "
//               "external\nentities. If false, the tool's domain of interaction
//               " "is closed.\nFor example, the world of a web search tool is "
//               "open, whereas that\nof a memory tool is not.\n\nDefault:
//               true",
//           MSG_TYPE : "boolean"
//         },
//         "readOnlyHint" : {
//           MSG_DESCRIPTION : "If true, the tool does not modify its "
//                           "environment.\n\nDefault: false",
//           MSG_TYPE : "boolean"
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

// ListToolsRequest {
//   MSG_DESCRIPTION
//       : "Sent from the client to request a list of tools the server has.",
//         MSG_PROPERTIES : {
//           MSG_METHOD : {MSG_CONST : MTHD_TOOLS_LIST, MSG_TYPE : MSG_STRING},
//           MSG_PARAMS : {
//             MSG_PROPERTIES : {
//               MSG_CURSOR : {
//                 MSG_DESCRIPTION :
//                     "An opaque token representing the current pagination "
//                     "position.\nIf provided, the server should return "
//                     "results starting after this cursor.",
//                 MSG_TYPE : MSG_STRING
//               }
//             },
//             MSG_TYPE : MSG_OBJECT
//           }
//         },
//                        MSG_REQUIRED : [MSG_METHOD],
//                                     MSG_TYPE : MSG_OBJECT
// };

// Sent from the client to request a list of tools the server has.
struct ListToolsRequest : public PaginatedRequest {
    ListToolsRequest() {
        Method = MTHD_TOOLS_LIST;
    }
};

// ListToolsResult {
//   MSG_DESCRIPTION
//       : "The server's response to a tools/list request from the client.",
//         MSG_PROPERTIES
//       : {
//         MSG_META : {
//           MSG_ADDITIONAL_PROPERTIES : {},
//           MSG_DESCRIPTION : "This result property is reserved by the protocol
//           to "
//                           "allow clients and servers to attach additional "
//                           "metadata to their responses.",
//           MSG_TYPE : MSG_OBJECT
//         },
//         MSG_NEXT_CURSOR : {
//           MSG_DESCRIPTION : "An opaque token representing the pagination "
//                           "position after the last returned result.\nIf "
//                           "present, there may be more results available.",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_TOOLS : {MSG_ITEMS : {"$ref" : "#/definitions/Tool"}, MSG_TYPE :
//         MSG_ARRAY}
//       },
//         MSG_REQUIRED : [MSG_TOOLS],
//                      MSG_TYPE : MSG_OBJECT
// };

// The server's response to a tools/list request from the client.
struct ListToolsResult : public PaginatedResult {
    vector<Tool> Tools;
};

// CallToolResult {
//   MSG_DESCRIPTION
//       : "The server's response to a tool call.\n\nAny errors that originate "
//         "from the tool SHOULD be reported inside the result\nobject, with "
//         "`isError` set to true, _not_ as an MCP protocol-level "
//         "error\nresponse. Otherwise, the LLM would not be able to see that "
//         "an error occurred\nand self-correct.\n\nHowever, any errors in "
//         "_finding_ the tool, an error indicating that the\nserver does not "
//         "support tool calls, or any other exceptional conditions,\nshould be
//         " "reported as an MCP error response.", MSG_PROPERTIES
//       : {
//         MSG_META : {
//           MSG_ADDITIONAL_PROPERTIES : {},
//           MSG_DESCRIPTION : "This result property is reserved by the protocol
//           to "
//                           "allow clients and servers to attach additional "
//                           "metadata to their responses.",
//           MSG_TYPE : MSG_OBJECT
//         },
//         MSG_CONTENT : {
//           MSG_ITEMS : {
//             "anyOf" : [
//               {"$ref" : "#/definitions/TextContent"},
//               {"$ref" : "#/definitions/ImageContent"},
//               {"$ref" : "#/definitions/AudioContent"},
//               {"$ref" : "#/definitions/EmbeddedResource"}
//             ]
//           },
//           MSG_TYPE : MSG_ARRAY
//         },
//         MSG_IS_ERROR : {
//           MSG_DESCRIPTION :
//               "Whether the tool call ended in an error.\n\nIf not set, this
//               is " "assumed to be false (the call was successful).",
//           MSG_TYPE : "boolean"
//         }
//       },
//         MSG_REQUIRED : [MSG_CONTENT],
//                      MSG_TYPE : MSG_OBJECT
// };

// The server's response to a tool call.
// Any errors that originate from the tool SHOULD be reported inside the result
// object, with `isError` set to true, _not_ as an MCP protocol-level error
// response. Otherwise, the LLM would not be able to see that an error occurred
// and self-correct.
// However, any errors in _finding_ the tool, an error indicating that the
// server does not support tool calls, or any other exceptional conditions,
// should be reported as an MCP error response.
struct CallToolResult : public ResultMessage {
    vector<variant<TextContent, ImageContent, AudioContent, EmbeddedResource>> Content;
    optional<bool> IsError; // Whether the tool call ended in an error. If not set, this is assumed
                            // to be false (the call was successful).
};

struct CallToolRequestParams {
    string Name;
    optional<AdditionalProperties> Arguments;
};

// CallToolRequest {
//   MSG_DESCRIPTION : "Used by the client to invoke a tool provided by the
//   server.",
//                   MSG_PROPERTIES
//       : {
//         MSG_METHOD : {MSG_CONST : MTHD_TOOLS_CALL, MSG_TYPE : MSG_STRING},
//         MSG_PARAMS : {
//           MSG_PROPERTIES : {
//             MSG_ARGUMENTS : {MSG_ADDITIONAL_PROPERTIES : {}, MSG_TYPE : MSG_OBJECT},
//             MSG_NAME : {MSG_TYPE : MSG_STRING}
//           },
//           MSG_REQUIRED : [MSG_NAME],
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED : [ MSG_METHOD, MSG_PARAMS ],
//                      MSG_TYPE : MSG_OBJECT
// };

// Used by the client to invoke a tool provided by the server.
struct CallToolRequest : public RequestMessage {
    CallToolRequestParams Params;

    CallToolRequest() {
        Method = MTHD_TOOLS_CALL;
    }
};

// ToolListChangedNotification {
//   MSG_DESCRIPTION
//       : "An optional notification from the server to the client, informing "
//         "it that the list of tools it offers has changed. This may be issued
//         " "by servers without any previous subscription from the client.",
//         MSG_PROPERTIES
//       : {
//         MSG_METHOD :
//             {MSG_CONST : MTHD_NOTIFICATIONS_TOOLS_LIST_CHANGED, MSG_TYPE :
//             MSG_STRING},
//         MSG_PARAMS : {
//           MSG_ADDITIONAL_PROPERTIES : {},
//           MSG_PROPERTIES : {
//             MSG_META : {
//               MSG_ADDITIONAL_PROPERTIES : {},
//               MSG_DESCRIPTION : "This parameter name is reserved by MCP to
//               allow "
//                               "clients and servers to attach additional "
//                               "metadata to their notifications.",
//               MSG_TYPE : MSG_OBJECT
//             }
//           },
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED : [MSG_METHOD],
//                      MSG_TYPE : MSG_OBJECT
// };

// An optional notification from the server to the client, informing it that the
// list of tools it offers has changed. This may be issued by servers without
// any previous subscription from the client.
struct ToolListChangedNotification : public NotificationMessage {
    ToolListChangedNotification() {
        Method = MTHD_NOTIFICATIONS_TOOLS_LIST_CHANGED;
    }
};

MCP_NAMESPACE_END