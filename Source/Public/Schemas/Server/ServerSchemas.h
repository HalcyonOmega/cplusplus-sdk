// Server-side schemas for Model Context Protocol
#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

struct ServerCapabilities {
    "description" : "Capabilities that a server may support. Known capabilities are defined here, "
                    "in this schema, but this is not a closed set: any server can define its own, "
                    "additional capabilities.",
                    "properties"
        : {
            "completions": {
                "additionalProperties": true,
                "description":
                    "Present if the server supports argument autocompletion suggestions.",
                "properties": {},
                "type": "object"
            },
            "experimental": {
                "additionalProperties":
                    {"additionalProperties": true, "properties": {}, "type": "object"},
                "description": "Experimental, non-standard capabilities that the server supports.",
                "type": "object"
            },
            "logging": {
                "additionalProperties": true,
                "description": "Present if the server supports sending log messages to the client.",
                "properties": {},
                "type": "object"
            },
            "prompts": {
                "description": "Present if the server offers any prompt templates.",
                "properties": {
                    "listChanged": {
                        "description":
                            "Whether this server supports notifications for changes to the "
                            "prompt list.",
                        "type": "boolean"
                    }
                },
                "type": "object"
            },
            "resources": {
                "description": "Present if the server offers any resources to read.",
                "properties": {
                    "listChanged": {
                        "description":
                            "Whether this server supports notifications for changes to the "
                            "resource list.",
                        "type": "boolean"
                    },
                    "subscribe": {
                        "description":
                            "Whether this server supports subscribing to resource updates.",
                        "type": "boolean"
                    }
                },
                "type": "object"
            },
            "tools": {
                "description": "Present if the server offers any tools to call.",
                "properties": {
                    "listChanged": {
                        "description": "Whether this server supports notifications for changes to "
                                       "the tool list.",
                        "type": "boolean"
                    }
                },
                "type": "object"
            }
        },
          "type" : "object"
};

struct ServerNotification {
    "anyOf" : [
        {"$ref": "#/definitions/CancelledNotification"},
        {"$ref": "#/definitions/ProgressNotification"},
        {"$ref": "#/definitions/ResourceListChangedNotification"},
        {"$ref": "#/definitions/ResourceUpdatedNotification"},
        {"$ref": "#/definitions/PromptListChangedNotification"},
        {"$ref": "#/definitions/ToolListChangedNotification"},
        {"$ref": "#/definitions/LoggingMessageNotification"}
    ]
};

struct ServerRequest {
    "anyOf" : [
        {"$ref": "#/definitions/PingRequest"}, {"$ref": "#/definitions/CreateMessageRequest"},
        {"$ref": "#/definitions/ListRootsRequest"}
    ]
};

struct ServerResult {
    "anyOf" : [
        {"$ref": "#/definitions/Result"}, {"$ref": "#/definitions/InitializeResult"},
        {"$ref": "#/definitions/ListResourcesResult"},
        {"$ref": "#/definitions/ListResourceTemplatesResult"},
        {"$ref": "#/definitions/ReadResourceResult"}, {"$ref": "#/definitions/ListPromptsResult"},
        {"$ref": "#/definitions/GetPromptResult"}, {"$ref": "#/definitions/ListToolsResult"},
        {"$ref": "#/definitions/CallToolResult"}, {"$ref": "#/definitions/CompleteResult"}
    ]
};

MCP_NAMESPACE_END