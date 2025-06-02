#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// struct Prompt {
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

// struct PromptArgument {
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

// struct PromptListChangedNotification {
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

// struct PromptMessage {
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

// struct PromptReference {
//   "description" : "Identifies a prompt.",
//                   "properties"
//       : {
//         "name" : {
//           "description" : "The name of the prompt or prompt template",
//           "type" : "string"
//         },
//         "type" : {"const" : "ref/prompt", "type" : "string"}
//       },
//         "required" : [ "name", "type" ],
//                      "type" : "object"
// };

// struct GetPromptRequest {
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

// struct GetPromptResult {
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

// struct ListPromptsRequest {
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

// struct ListPromptsResult {
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

/* Prompts */
/**
 * Sent from the client to request a list of prompts and prompt templates the
 * server has.
 */
export interface ListPromptsRequest extends PaginatedRequest {
method:
  "prompts/list";
}

/**
 * The server's response to a prompts/list request from the client.
 */
export interface ListPromptsResult extends PaginatedResult {
prompts:
  Prompt[];
}

/**
 * Used by the client to get a prompt provided by the server.
 */
export interface GetPromptRequest extends Request {
method:
  "prompts/get";
params: {
/**
 * The name of the prompt or prompt template.
 */
name:
  string;
  /**
   * Arguments to use for templating the prompt.
   */
  arguments ?: {[key:string] : string};
};
}

/**
 * The server's response to a prompts/get request from the client.
 */
export interface GetPromptResult extends Result {
  /**
   * An optional description for the prompt.
   */
  description ?: string;
messages:
  PromptMessage[];
}

/**
 * A prompt or prompt template that the server offers.
 */
export interface Prompt {
/**
 * The name of the prompt or prompt template.
 */
name:
  string;
  /**
   * An optional description of what this prompt provides
   */
  description ?: string;
  /**
   * A list of arguments to use for templating the prompt.
   */
  arguments ?: PromptArgument[];
}

/**
 * Describes an argument that a prompt can accept.
 */
export interface PromptArgument {
/**
 * The name of the argument.
 */
name:
  string;
  /**
   * A human-readable description of the argument.
   */
  description ?: string;
  /**
   * Whether this argument must be provided.
   */
  required ?: boolean;
}

/**
 * Describes a message returned as part of a prompt.
 *
 * This is similar to `SamplingMessage`, but also supports the embedding of
 * resources from the MCP server.
 */
export interface PromptMessage {
role:
  Role;
content:
  TextContent | ImageContent | AudioContent | EmbeddedResource;
}

/**
 * An optional notification from the server to the client, informing it that the
 * list of prompts it offers has changed. This may be issued by servers without
 * any previous subscription from the client.
 */
export interface PromptListChangedNotification extends Notification {
method:
  "notifications/prompts/list_changed";
}

/**
 * Identifies a prompt.
 */
export interface PromptReference {
type:
  "ref/prompt";
/**
 * The name of the prompt or prompt template
 */
name:
  string;
}

MCP_NAMESPACE_END