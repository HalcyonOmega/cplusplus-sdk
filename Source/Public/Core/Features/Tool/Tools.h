#pragma once

#include "Core/Types/Content.h"
#include "Core/Types/Roles.h"
#include "MethodConstants.h"
#include "NotificationBase.h"
#include "RequestBase.h"
#include "ResponseBase.h"
#include "ToolBase.h"

MCP_NAMESPACE_BEGIN

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
    ListToolsRequest() : PaginatedRequest(MTHD_TOOLS_LIST) {}
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
//           MSG_TYPE : MSG_BOOLEAN
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
struct CallToolResult : public ResponseBase {
    vector<variant<TextContent, ImageContent, AudioContent, EmbeddedResource>> Content =
        {}; // A list of content objects that represent the result of the tool call. If the Tool
            // does not define an output, this field MUST be present in the result. For backwards
            // compatibility, this field is always present, but it may be empty.
    optional<bool>
        IsError; // Whether the tool call ended in an error. If not set, this is assumed
                 // to be false (the call was successful). Any errors that originate from
                 // the tool SHOULD be reported inside the result object, with `isError`
                 // set to true, _not_ as an MCP protocol - level error response.
                 // Otherwise, the LLM would not be able to see that an error occurred
                 // and self-correct. However, any errors in _finding_ the tool, an
                 // error indicating that the *server does not support tool calls, or any other
                 // exceptional conditions, should be reported as an MCP error response.
    optional<JSONSchema>
        StructuredContent; // An object containing structured tool output. If the Tool
                           // defines an output, this field MUST be present in the result,
                           // and contain a JSON object that matches the schema.
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
struct CallToolRequest : public RequestBase {
    struct CallToolRequestParams {
        string Name;
        optional<AdditionalProperties> Arguments;
    };

    CallToolRequestParams Params;

    CallToolRequest() : RequestBase(MTHD_TOOLS_CALL) {}
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

// An optional notification from the server to the client, informing it that the list of tools it
// offers has changed. This may be issued by servers without any previous subscription from the
// client.
struct ToolListChangedNotification : public NotificationBase {
    ToolListChangedNotification() : NotificationBase(MTHD_NOTIFICATIONS_TOOLS_LIST_CHANGED) {}
};

MCP_NAMESPACE_END