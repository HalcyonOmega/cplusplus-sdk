#pragma once

#include "Core.h"
#include "Core/Features/Notification/NotificationSchemas.h"
#include "Core/Features/Request/RequestSchemas.h"
#include "Core/Features/Result/ResultSchemas.h"
#include "Core/Features/Roles/RolesSchemas.h"
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
//           MSG_TYPE : "boolean"
//         }
//       },
//         MSG_REQUIRED : [MSG_NAME],
//                      MSG_TYPE : MSG_OBJECT
// };

// Describes an argument that a prompt can accept.
struct PromptArgument {
    string Name;                  // The name of the argument.
    optional<string> Description; // A human-readable description of the argument.
    optional<bool> Required;      // Whether this argument must be provided.
    Passthrough Additional;       // Additional properties.
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
//           "role" : {"$ref" : "#/definitions/Role"}
//         },
//                        MSG_REQUIRED : [ MSG_CONTENT, "role" ],
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

// ListPromptsRequest {
//   MSG_DESCRIPTION : "Sent from the client to request a list of prompts and "
//                   "prompt templates the server has.",
//                   MSG_PROPERTIES
//       : {
//         MSG_METHOD : {MSG_CONST : MTHD_PROMPTS_LIST, MSG_TYPE : MSG_STRING},
//         MSG_PARAMS : {
//           MSG_PROPERTIES : {
//             MSG_CURSOR : {
//               MSG_DESCRIPTION :
//                   "An opaque token representing the current pagination "
//                   "position.\nIf provided, the server should return "
//                   "results starting after this cursor.",
//               MSG_TYPE : MSG_STRING
//             }
//           },
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED : [MSG_METHOD],
//                      MSG_TYPE : MSG_OBJECT
// };

/**
 * Sent from the client to request a list of prompts and prompt templates the
 * server has.
 */
struct ListPromptsRequest : public PaginatedRequest {
    ListPromptsRequest() {
        method = MTHD_PROMPTS_LIST;
    }
};

// ListPromptsResult {
//   MSG_DESCRIPTION
//       : "The server's response to a prompts/list request from the client.",
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
//         MSG_PROMPTS :
//             {MSG_ITEMS : {"$ref" : "#/definitions/Prompt"}, MSG_TYPE : MSG_ARRAY}
//       },
//         MSG_REQUIRED : [MSG_PROMPTS],
//                      MSG_TYPE : MSG_OBJECT
// };

/**
 * The server's response to a prompts/list request from the client.
 */
struct ListPromptsResult : public PaginatedResult {
    vector<Prompt> prompts;
};

struct GetPromptRequestParams {
    /**
     * The name of the prompt or prompt template.
     */
    string name;
    /**
     * Arguments to use for templating the prompt.
     */
    optional<AdditionalStrings> arguments;
};

// GetPromptRequest {
//   MSG_DESCRIPTION : "Used by the client to get a prompt provided by the
//   server.",
//                   MSG_PROPERTIES
//       : {
//         MSG_METHOD : {MSG_CONST : MTHD_PROMPTS_GET, MSG_TYPE : MSG_STRING},
//         MSG_PARAMS : {
//           MSG_PROPERTIES : {
//             MSG_ARGUMENTS : {
//               MSG_ADDITIONAL_PROPERTIES : {MSG_TYPE : MSG_STRING},
//               MSG_DESCRIPTION : "Arguments to use for templating the prompt.",
//               MSG_TYPE : MSG_OBJECT
//             },
//             MSG_NAME : {
//               MSG_DESCRIPTION : "The name of the prompt or prompt template.",
//               MSG_TYPE : MSG_STRING
//             }
//           },
//           MSG_REQUIRED : [MSG_NAME],
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED : [ MSG_METHOD, MSG_PARAMS ],
//                      MSG_TYPE : MSG_OBJECT
// };

/**
 * Used by the client to get a prompt provided by the server.
 */
struct GetPromptRequest : public Request {
    GetPromptRequestParams params;

    GetPromptRequest() {
        method = MTHD_PROMPTS_GET;
    }
};

// GetPromptResult {
//   MSG_DESCRIPTION
//       : "The server's response to a prompts/get request from the client.",
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
//         MSG_DESCRIPTION : {
//           MSG_DESCRIPTION : "An optional description for the prompt.",
//           MSG_TYPE : MSG_STRING
//         },
//         "messages" : {
//           MSG_ITEMS : {"$ref" : "#/definitions/PromptMessage"},
//           MSG_TYPE : MSG_ARRAY
//         }
//       },
//         MSG_REQUIRED : ["messages"],
//                      MSG_TYPE : MSG_OBJECT
// };

/**
 * The server's response to a prompts/get request from the client.
 */
struct GetPromptResult : public Result {
    /**
     * An optional description for the prompt.
     */
    optional<string> description;
    vector<PromptMessage> messages;
};

// PromptListChangedNotification {
//   MSG_DESCRIPTION : "An optional notification from the server to the client, "
//                   "informing it that the list of prompts it offers has "
//                   "changed. This may be issued by servers without any "
//                   "previous subscription from the client.",
//                   MSG_PROPERTIES
//       : {
//         MSG_METHOD :
//             {MSG_CONST : MTHD_NOTIFICATIONS_PROMPTS_LIST_CHANGED, MSG_TYPE :
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

/**
 * An optional notification from the server to the client, informing it that the
 * list of prompts it offers has changed. This may be issued by servers without
 * any previous subscription from the client.
 */
struct PromptListChangedNotification : public Notification {
    PromptListChangedNotification() {
        method = MTHD_NOTIFICATIONS_PROMPTS_LIST_CHANGED;
    }
};

MCP_NAMESPACE_END