#pragma once

#include "Core.h"
#include "Schemas/Common/AutocompleteSchemas.h"
#include "Schemas/Common/CommonSchemas.h"
#include "Schemas/Common/InitializeSchemas.h"
#include "Schemas/Common/LoggingSchemas.h"
#include "Schemas/Common/NotificationSchemas.h"
#include "Schemas/Common/PromptSchemas.h"
#include "Schemas/Common/RequestSchemas.h"
#include "Schemas/Common/ResourceSchemas.h"
#include "Schemas/Common/ResultSchemas.h"
#include "Schemas/Common/RootsSchemas.h"
#include "Schemas/Common/SamplingSchemas.h"
#include "Schemas/Common/ToolSchemas.h"

MCP_NAMESPACE_BEGIN

/* General */

// Cursor {
//   "description" : "An opaque token used to represent a cursor for
//   pagination.",
//                   "type" : "string"
// };

/**
 * An opaque token used to represent a cursor for pagination.
 */
using Cursor = string;

/* Client messages */

// ClientRequest {
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

using ClientRequest =
    std::variant<PingRequest, InitializeRequest, CompleteRequest, SetLevelRequest, GetPromptRequest,
                 ListPromptsRequest, ListResourcesRequest, ListResourceTemplatesRequest,
                 ReadResourceRequest, SubscribeRequest, UnsubscribeRequest, CallToolRequest,
                 ListToolsRequest>;

// ClientNotification {
//     "anyOf" : [
//         {"$ref": "#/definitions/CancelledNotification"},
//         {"$ref": "#/definitions/InitializedNotification"},
//         {"$ref": "#/definitions/ProgressNotification"},
//         {"$ref": "#/definitions/RootsListChangedNotification"}
//     ]
// };

using ClientNotification = std::variant<CancelledNotification, ProgressNotification,
                                        InitializedNotification, RootsListChangedNotification>;

// ClientResult {
//     "anyOf" : [
//         {"$ref": "#/definitions/Result"}, {"$ref":
//         "#/definitions/CreateMessageResult"},
//         {"$ref": "#/definitions/ListRootsResult"}
//     ]
// };

using ClientResult = std::variant<EmptyResult, CreateMessageResult, ListRootsResult>;

/* Server messages */

// ServerRequest {
//     "anyOf" : [
//         {"$ref": "#/definitions/PingRequest"}, {"$ref":
//         "#/definitions/CreateMessageRequest"},
//         {"$ref": "#/definitions/ListRootsRequest"}
//     ]
// };

using ServerRequest = std::variant<PingRequest, CreateMessageRequest, ListRootsRequest>;

// ServerNotification {
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

using ServerNotification =
    std::variant<CancelledNotification, ProgressNotification, LoggingMessageNotification,
                 ResourceUpdatedNotification, ResourceListChangedNotification,
                 ToolListChangedNotification, PromptListChangedNotification>;

// ServerResult {
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

using ServerResult =
    std::variant<EmptyResult, InitializeResult, CompleteResult, GetPromptResult, ListPromptsResult,
                 ListResourceTemplatesResult, ListResourcesResult, ReadResourceResult,
                 CallToolResult, ListToolsResult>;

MCP_NAMESPACE_END