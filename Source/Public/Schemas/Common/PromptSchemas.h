#pragma once

#include "CommonSchemas.h"
#include "Constants.h"
#include "ContentSchemas.h"
#include "Core.h"
#include "NotificationSchemas.h"
#include "RequestSchemas.h"
#include "ResultSchemas.h"
#include "Schemas/Client/ClientSchemas.h"

MCP_NAMESPACE_BEGIN

// PromptArgument {
//   "description" : "Describes an argument that a prompt can accept.",
//                   "properties"
//       : {
//         "description" : {
//           "description" : "A human-readable description of the argument.",
//           "type" : "string"
//         },
//         "name" :
//             {"description" : "The name of the argument.", "type" : "string"},
//         "required" : {
//           "description" : "Whether this argument must be provided.",
//           "type" : "boolean"
//         }
//       },
//         "required" : ["name"],
//                      "type" : "object"
// };

/**
 * Describes an argument that a prompt can accept.
 */
struct PromptArgument {
    /**
     * The name of the argument.
     */
    string name;
    /**
     * A human-readable description of the argument.
     */
    optional<string> description;
    /**
     * Whether this argument must be provided.
     */
    optional<bool> required;
};

// PromptMessage {
//   "description"
//       : "Describes a message returned as part of a prompt.\n\nThis is "
//         "similar to `SamplingMessage`, but also supports the embedding "
//         "of\nresources from the MCP server.",
//         "properties" : {
//           "content" : {
//             "anyOf" : [
//               {"$ref" : "#/definitions/TextContent"},
//               {"$ref" : "#/definitions/ImageContent"},
//               {"$ref" : "#/definitions/AudioContent"},
//               {"$ref" : "#/definitions/EmbeddedResource"}
//             ]
//           },
//           "role" : {"$ref" : "#/definitions/Role"}
//         },
//                        "required" : [ "content", "role" ],
//                                     "type" : "object"
// };

/**
 * Describes a message returned as part of a prompt.
 *
 * This is similar to `SamplingMessage`, but also supports the embedding of
 * resources from the MCP server.
 */
struct PromptMessage {
    Role role;
    variant<TextContent, ImageContent, AudioContent, EmbeddedResource> content;
};

// Prompt {
//   "description" : "A prompt or prompt template that the server offers.",
//                   "properties"
//       : {
//         "arguments" : {
//           "description" :
//               "A list of arguments to use for templating the prompt.",
//           "items" : {"$ref" : "#/definitions/PromptArgument"},
//           "type" : "array"
//         },
//         "description" : {
//           "description" :
//               "An optional description of what this prompt provides",
//           "type" : "string"
//         },
//         "name" : {
//           "description" : "The name of the prompt or prompt template.",
//           "type" : "string"
//         }
//       },
//         "required" : ["name"],
//                      "type" : "object"
// };

/**
 * A prompt or prompt template that the server offers.
 */
struct Prompt {
    /**
     * The name of the prompt or prompt template.
     */
    string name;
    /**
     * An optional description of what this prompt provides
     */
    optional<string> description;
    /**
     * A list of arguments to use for templating the prompt.
     */
    optional<vector<PromptArgument>> arguments;
};

// ListPromptsRequest {
//   "description" : "Sent from the client to request a list of prompts and "
//                   "prompt templates the server has.",
//                   "properties"
//       : {
//         "method" : {"const" : "prompts/list", "type" : "string"},
//         "params" : {
//           "properties" : {
//             "cursor" : {
//               "description" :
//                   "An opaque token representing the current pagination "
//                   "position.\nIf provided, the server should return "
//                   "results starting after this cursor.",
//               "type" : "string"
//             }
//           },
//           "type" : "object"
//         }
//       },
//         "required" : ["method"],
//                      "type" : "object"
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
//   "description"
//       : "The server's response to a prompts/list request from the client.",
//         "properties"
//       : {
//         "_meta" : {
//           "additionalProperties" : {},
//           "description" : "This result property is reserved by the protocol
//           to "
//                           "allow clients and servers to attach additional "
//                           "metadata to their responses.",
//           "type" : "object"
//         },
//         "nextCursor" : {
//           "description" : "An opaque token representing the pagination "
//                           "position after the last returned result.\nIf "
//                           "present, there may be more results available.",
//           "type" : "string"
//         },
//         "prompts" :
//             {"items" : {"$ref" : "#/definitions/Prompt"}, "type" : "array"}
//       },
//         "required" : ["prompts"],
//                      "type" : "object"
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
//   "description" : "Used by the client to get a prompt provided by the
//   server.",
//                   "properties"
//       : {
//         "method" : {"const" : "prompts/get", "type" : "string"},
//         "params" : {
//           "properties" : {
//             "arguments" : {
//               "additionalProperties" : {"type" : "string"},
//               "description" : "Arguments to use for templating the prompt.",
//               "type" : "object"
//             },
//             "name" : {
//               "description" : "The name of the prompt or prompt template.",
//               "type" : "string"
//             }
//           },
//           "required" : ["name"],
//           "type" : "object"
//         }
//       },
//         "required" : [ "method", "params" ],
//                      "type" : "object"
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
//   "description"
//       : "The server's response to a prompts/get request from the client.",
//         "properties"
//       : {
//         "_meta" : {
//           "additionalProperties" : {},
//           "description" : "This result property is reserved by the protocol
//           to "
//                           "allow clients and servers to attach additional "
//                           "metadata to their responses.",
//           "type" : "object"
//         },
//         "description" : {
//           "description" : "An optional description for the prompt.",
//           "type" : "string"
//         },
//         "messages" : {
//           "items" : {"$ref" : "#/definitions/PromptMessage"},
//           "type" : "array"
//         }
//       },
//         "required" : ["messages"],
//                      "type" : "object"
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
//   "description" : "An optional notification from the server to the client, "
//                   "informing it that the list of prompts it offers has "
//                   "changed. This may be issued by servers without any "
//                   "previous subscription from the client.",
//                   "properties"
//       : {
//         "method" :
//             {"const" : "notifications/prompts/list_changed", "type" :
//             "string"},
//         "params" : {
//           "additionalProperties" : {},
//           "properties" : {
//             "_meta" : {
//               "additionalProperties" : {},
//               "description" : "This parameter name is reserved by MCP to
//               allow "
//                               "clients and servers to attach additional "
//                               "metadata to their notifications.",
//               "type" : "object"
//             }
//           },
//           "type" : "object"
//         }
//       },
//         "required" : ["method"],
//                      "type" : "object"
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