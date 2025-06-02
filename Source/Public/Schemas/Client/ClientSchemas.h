// Client-side schemas for Model Context Protocol
#pragma once

#include "Constants.h"
#include "Core.h"
#include "Schemas/Common/CommonSchemas.h"

MCP_NAMESPACE_BEGIN

// struct ClientCapabilities {
//     "description"
//         : "Capabilities a client may support. Known capabilities are defined
//         here, in this schema, "
//           "but this is not a closed set: any client can define its own,
//           additional capabilities.", "properties"
//         : {
//             "experimental": {
//                 "additionalProperties":
//                     {"additionalProperties": true, "properties": {}, "type":
//                     "object"},
//                 "description": "Experimental, non-standard capabilities that
//                 the client supports.", "type": "object"
//             },
//             "roots": {
//                 "description": "Present if the client supports listing
//                 roots.", "properties": {
//                     "listChanged": {
//                         "description": "Whether the client supports
//                         notifications for changes to "
//                                        "the roots list.",
//                         "type": "boolean"
//                     }
//                 },
//                 "type": "object"
//             },
//             "sampling": {
//                 "additionalProperties": true,
//                 "description": "Present if the client supports sampling from
//                 an LLM.", "properties": {}, "type": "object"
//             }
//         },
//           "type" : "object"
// };

// struct ClientNotification {
//     "anyOf" : [
//         {"$ref": "#/definitions/CancelledNotification"},
//         {"$ref": "#/definitions/InitializedNotification"},
//         {"$ref": "#/definitions/ProgressNotification"},
//         {"$ref": "#/definitions/RootsListChangedNotification"}
//     ]
// };

// struct ClientRequest {
//     "anyOf" : [
//         {"$ref": "#/definitions/InitializeRequest"}, {"$ref":
//         "#/definitions/PingRequest"},
//         {"$ref": "#/definitions/ListResourcesRequest"},
//         {"$ref": "#/definitions/ListResourceTemplatesRequest"},
//         {"$ref": "#/definitions/ReadResourceRequest"}, {"$ref":
//         "#/definitions/SubscribeRequest"},
//         {"$ref": "#/definitions/UnsubscribeRequest"}, {"$ref":
//         "#/definitions/ListPromptsRequest"},
//         {"$ref": "#/definitions/GetPromptRequest"}, {"$ref":
//         "#/definitions/ListToolsRequest"},
//         {"$ref": "#/definitions/CallToolRequest"}, {"$ref":
//         "#/definitions/SetLevelRequest"},
//         {"$ref": "#/definitions/CompleteRequest"}
//     ]
// };

// struct ClientResult {
//     "anyOf" : [
//         {"$ref": "#/definitions/Result"}, {"$ref":
//         "#/definitions/CreateMessageResult"},
//         {"$ref": "#/definitions/ListRootsResult"}
//     ]
// };

// struct Annotations {
//     "description" : "Optional annotations for the client. The client can use
//     annotations to inform "
//                     "how objects are used or displayed",
//                     "properties"
//         : {
//             "audience": {
//                 "description":
//                     "Describes who the intended customer of this object or
//                     data is.\n\nIt " "can include multiple entries to
//                     indicate content useful for multiple " "audiences (e.g.,
//                     `[\"user\", \"assistant\"]`).",
//                 "items": {"$ref": "#/definitions/Role"},
//                 "type": "array"
//             },
//             "priority": {
//                 "description":
//                     "Describes how important this data is for operating the
//                     server.\n\nA " "value of 1 means \"most important,\" and
//                     indicates that the data " "is\neffectively required,
//                     while 0 means \"least important,\" and " "indicates
//                     that\nthe data is entirely optional.",
//                 "maximum": 1,
//                 "minimum": 0,
//                 "type": "number"
//             }
//         },
//           "type" : "object"
// };

/**
 * Capabilities a client may support. Known capabilities are defined here, in
 * this schema, but this is not a closed set: any client can define its own,
 * additional capabilities.
 */
struct ClientCapabilities {
  /**
   * Experimental, non-standard capabilities that the client supports.
   */
  experimental ?: {[key:string] : object};
  /**
   * Present if the client supports listing roots.
   */
  roots ?: {
    /**
     * Whether the client supports notifications for changes to the roots list.
     */
    optional<bool> listChanged;
  };
  /**
   * Present if the client supports sampling from an LLM.
   */
  sampling ?: object;
};

/**
 * Optional annotations for the client. The client can use annotations to inform
 * how objects are used or displayed
 */
struct Annotations {
  /**
   * Describes who the intended customer of this object or data is.
   *
   * It can include multiple entries to indicate content useful for multiple
   * audiences (e.g.,
   * `["user", "assistant"]`).
   */
  audience ?: Role[];

  /**
   * Describes how important this data is for operating the server.
   *
   * A value of 1 means "most important," and indicates that the data is
   * effectively required, while 0 means "least important," and indicates that
   * the data is entirely optional.
   *
   * @TJS-type number
   * @minimum 0
   * @maximum 1
   */
  priority ?: number;
};

/* Client messages */
using ClientRequest =
    std::variant<PingRequest, InitializeRequest, CompleteRequest,
                 SetLevelRequest, GetPromptRequest, ListPromptsRequest,
                 ListResourcesRequest, ListResourceTemplatesRequest,
                 ReadResourceRequest, SubscribeRequest, UnsubscribeRequest,
                 CallToolRequest, ListToolsRequest>;

using ClientNotification =
    std::variant<CancelledNotification, ProgressNotification,
                 InitializedNotification, RootsListChangedNotification>;

using ClientResult =
    std::variant<EmptyResult, CreateMessageResult, ListRootsResult>;

MCP_NAMESPACE_END