// Server-side schemas for Model Context Protocol
#pragma once

#include "Constants.h"
#include "Core.h"
#include "Schemas/Common/CommonSchemas.h"

MCP_NAMESPACE_BEGIN

struct ServerCapabilitiesPrompts {
    /**
     * Whether this server supports notifications for changes to the prompt
     * list.
     */
    optional<bool> listChanged;
};

struct ServerCapabilitiesResources {
    /**
     * Whether this server supports subscribing to resource updates.
     */
    optional<bool> subscribe;
    /**
     * Whether this server supports notifications for changes to the resource
     * list.
     */
    optional<bool> listChanged;
};

struct ServerCapabilitiesTools {
    /**
     * Whether this server supports notifications for changes to the tool list.
     */
    optional<bool> listChanged;
};

// ServerCapabilities {
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

/**
 * Capabilities that a server may support. Known capabilities are defined here,
 * in this schema, but this is not a closed set: any server can define its own,
 * additional capabilities.
 */
struct ServerCapabilities {
    /**
     * Experimental, non-standard capabilities that the server supports.
     */
    optional<AdditionalObjects> experimental;
    /**
     * Present if the server supports sending log messages to the client.
     */
    optional<JSON> logging;
    /**
     * Present if the server supports argument autocompletion suggestions.
     */
    optional<JSON> completions;
    /**
     * Present if the server offers any prompt templates.
     */
    optional<ServerCapabilitiesPrompts> prompts;
    /**
     * Present if the server offers any resources to read.
     */
    optional<ServerCapabilitiesResources> resources;
    /**
     * Present if the server offers any tools to call.
     */
    optional<ServerCapabilitiesTools> tools;
};

MCP_NAMESPACE_END