// Server-side schemas for Model Context Protocol
#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// ServerCapabilities_JSON{
//     "description": "Capabilities that a server may support. Known
//     capabilities are defined here,
//     "
//                    "in this schema, but this is not a closed set: any server
//                    can define its own, " "additional capabilities.",
//     "properties": {
//         "completions": {
//             "additionalProperties": true,
//             "description": "Present if the server supports argument
//             autocompletion suggestions.", "properties": {}, "type": "object"
//         },
//         "experimental": {
//             "additionalProperties":
//                 {"additionalProperties": true, "properties": {}, "type":
//                 "object"},
//             "description": "Experimental, non-standard capabilities that the
//             server supports.", "type": "object"
//         },
//         "logging": {
//             "additionalProperties": true,
//             "description": "Present if the server supports sending log
//             messages to the client.", "properties": {}, "type": "object"
//         },
//         "prompts": {
//             "description": "Present if the server offers any prompt
//             templates.", "properties": {
//                 "listChanged": {
//                     "description": "Whether this server supports
//                     notifications for changes to the
//                     "
//                                    "prompt list.",
//                     "type": "boolean"
//                 }
//             },
//             "type": "object"
//         },
//         "resources": {
//             "description": "Present if the server offers any resources to
//             read.", "properties": {
//                 "listChanged": {
//                     "description": "Whether this server supports
//                     notifications for changes to the
//                     "
//                                    "resource list.",
//                     "type": "boolean"
//                 },
//                 "subscribe": {
//                     "description": "Whether this server supports subscribing
//                     to resource updates.", "type": "boolean"
//                 }
//             },
//             "type": "object"
//         },
//         "tools": {
//             "description": "Present if the server offers any tools to call.",
//             "properties": {
//                 "listChanged": {
//                     "description": "Whether this server supports
//                     notifications for changes to "
//                                    "the tool list.",
//                     "type": "boolean"
//                 }
//             },
//             "type": "object"
//         }
//     },
//     "type": "object"
// };

// ServerNotification_JSON {
//     "anyOf" : [
//         {"$ref": "#/definitions/CancelledNotification"},
//         {"$ref": "#/definitions/ProgressNotification"},
//         {"$ref": "#/definitions/ResourceListChangedNotification"},
//         {"$ref": "#/definitions/ResourceUpdatedNotification"},
//         {"$ref": "#/definitions/PromptListChangedNotification"},
//         {"$ref": "#/definitions/ToolListChangedNotification"},
//         {"$ref": "#/definitions/LoggingMessageNotification"}
//     ]
// };

// ServerRequest_JSON {
//     "anyOf" : [
//         {"$ref": "#/definitions/PingRequest"}, {"$ref":
//         "#/definitions/CreateMessageRequest"},
//         {"$ref": "#/definitions/ListRootsRequest"}
//     ]
// };

// ServerResult_JSON {
//     "anyOf" : [
//         {"$ref": "#/definitions/Result"}, {"$ref":
//         "#/definitions/InitializeResult"},
//         {"$ref": "#/definitions/ListResourcesResult"},
//         {"$ref": "#/definitions/ListResourceTemplatesResult"},
//         {"$ref": "#/definitions/ReadResourceResult"}, {"$ref":
//         "#/definitions/ListPromptsResult"},
//         {"$ref": "#/definitions/GetPromptResult"}, {"$ref":
//         "#/definitions/ListToolsResult"},
//         {"$ref": "#/definitions/CallToolResult"}, {"$ref":
//         "#/definitions/CompleteResult"}
//     ]
// };

/**
 * Capabilities that a server may support. Known capabilities are defined here,
 * in this schema, but this is not a closed set: any server can define its own,
 * additional capabilities.
 */
export interface ServerCapabilities {
  /**
   * Experimental, non-standard capabilities that the server supports.
   */
  experimental ?: {[key:string] : object};
  /**
   * Present if the server supports sending log messages to the client.
   */
  logging ?: object;
  /**
   * Present if the server supports argument autocompletion suggestions.
   */
  completions ?: object;
  /**
   * Present if the server offers any prompt templates.
   */
  prompts ?: {
    /**
     * Whether this server supports notifications for changes to the prompt
     * list.
     */
    listChanged ?: boolean;
  };
  /**
   * Present if the server offers any resources to read.
   */
  resources ?: {
    /**
     * Whether this server supports subscribing to resource updates.
     */
    subscribe ?: boolean;
    /**
     * Whether this server supports notifications for changes to the resource
     * list.
     */
    listChanged ?: boolean;
  };
  /**
   * Present if the server offers any tools to call.
   */
  tools ?: {
    /**
     * Whether this server supports notifications for changes to the tool list.
     */
    listChanged ?: boolean;
  };
}

/* Server messages */
using ServerRequest =
    std::variant<PingRequest, CreateMessageRequest, ListRootsRequest>;

using ServerNotification =
    std::variant<CancelledNotification, ProgressNotification,
                 LoggingMessageNotification, ResourceUpdatedNotification,
                 ResourceListChangedNotification, ToolListChangedNotification,
                 PromptListChangedNotification>;

using ServerResult =
    std::variant<EmptyResult, InitializeResult, CompleteResult, GetPromptResult,
                 ListPromptsResult, ListResourceTemplatesResult,
                 ListResourcesResult, ReadResourceResult, CallToolResult,
                 ListToolsResult>;

MCP_NAMESPACE_END