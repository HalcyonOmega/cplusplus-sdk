#pragma once

#include "CommonSchemas.h"
#include "Constants.h"
#include "ContentSchemas.h"
#include "Core.h"
#include "NotificationSchemas.h"
#include "RequestSchemas.h"
#include "ResultSchemas.h"

MCP_NAMESPACE_BEGIN

// ToolAnnotations {
//   "description"
//       : "Additional properties describing a Tool to clients.\n\nNOTE: all "
//         "properties in ToolAnnotations are **hints**.\nThey are not "
//         "guaranteed to provide a faithful description of\ntool behavior "
//         "(including descriptive properties like `title`).\n\nClients should "
//         "never make tool use decisions based on ToolAnnotations\nreceived "
//         "from untrusted servers.",
//         "properties"
//       : {
//         "destructiveHint" : {
//           "description" :
//               "If true, the tool may perform destructive updates to its "
//               "environment.\nIf false, the tool performs only additive "
//               "updates.\n\n(This property is meaningful only when "
//               "`readOnlyHint == false`)\n\nDefault: true",
//           "type" : "boolean"
//         },
//         "idempotentHint" : {
//           "description" :
//               "If true, calling the tool repeatedly with the same "
//               "arguments\nwill have no additional effect on the its "
//               "environment.\n\n(This property is meaningful only when "
//               "`readOnlyHint == false`)\n\nDefault: false",
//           "type" : "boolean"
//         },
//         "openWorldHint" : {
//           "description" :
//               "If true, this tool may interact with an \"open world\" of "
//               "external\nentities. If false, the tool's domain of interaction
//               " "is closed.\nFor example, the world of a web search tool is "
//               "open, whereas that\nof a memory tool is not.\n\nDefault:
//               true",
//           "type" : "boolean"
//         },
//         "readOnlyHint" : {
//           "description" : "If true, the tool does not modify its "
//                           "environment.\n\nDefault: false",
//           "type" : "boolean"
//         },
//         "title" : {
//           "description" : "A human-readable title for the tool.",
//           "type" : "string"
//         }
//       },
//         "type" : "object"
// };

/**
 * Additional properties describing a Tool to clients.
 *
 * NOTE: all properties in ToolAnnotations are **hints**.
 * They are not guaranteed to provide a faithful description of
 * tool behavior (including descriptive properties like `title`).
 *
 * Clients should never make tool use decisions based on ToolAnnotations
 * received from untrusted servers.
 */
struct ToolAnnotations {
    /**
     * A human-readable title for the tool.
     */
    optional<string> title;

    /**
     * If true, the tool does not modify its environment.
     *
     * Default: false
     */
    optional<bool> readOnlyHint;

    /**
     * If true, the tool may perform destructive updates to its environment.
     * If false, the tool performs only additive updates.
     *
     * (This property is meaningful only when `readOnlyHint == false`)
     *
     * Default: true
     */
    optional<bool> destructiveHint;

    /**
     * If true, calling the tool repeatedly with the same arguments
     * will have no additional effect on the its environment.
     *
     * (This property is meaningful only when `readOnlyHint == false`)
     *
     * Default: false
     */
    optional<bool> idempotentHint;

    /**
     * If true, this tool may interact with an "open world" of external
     * entities. If false, the tool's domain of interaction is closed.
     * For example, the world of a web search tool is open, whereas that
     * of a memory tool is not.
     *
     * Default: true
     */
    optional<bool> openWorldHint;
};

struct ToolInputSchema {
    string type = "object";
    optional<AdditionalObjects> properties;
    optional<vector<string>> required;
};

// Tool {
//   "description" : "Definition for a tool the client can call.",
//                   "properties"
//       : {
//         "annotations" : {
//           "$ref" : "#/definitions/ToolAnnotations",
//           "description" : "Optional additional tool information."
//         },
//         "description" : {
//           "description" : "A human-readable description of the tool.\n\nThis
//           "
//                           "can be used by "
//                           "clients to improve the LLM's understanding of "
//                           "available tools. It "
//                           "can be thought of like a \"hint\" to the model.",
//           "type" : "string"
//         },
//         "inputSchema" : {
//           "description" : "A JSON Schema object defining the expected "
//                           "parameters for the tool.",
//           "properties" : {
//             "properties" : {
//               "additionalProperties" : {
//                 "additionalProperties" : true,
//                 "properties" : {},
//                 "type" : "object"
//               },
//               "type" : "object"
//             },
//             "required" : {"items" : {"type" : "string"}, "type" : "array"},
//             "type" : {"const" : "object", "type" : "string"}
//           },
//           "required" : ["type"],
//           "type" : "object"
//         },
//         "name" : {"description" : "The name of the tool.", "type" : "string"}
//       },
//         "required" : [ "inputSchema", "name" ],
//                      "type" : "object"
// };

/**
 * Definition for a tool the client can call.
 */
struct Tool {
    /**
     * The name of the tool.
     */
    string name;

    /**
     * A human-readable description of the tool.
     *
     * This can be used by clients to improve the LLM's understanding of available
     * tools. It can be thought of like a "hint" to the model.
     */
    optional<string> description;

    /**
     * A JSON Schema object defining the expected parameters for the tool.
     */
    ToolInputSchema inputSchema;

    /**
     * Optional additional tool information.
     */
    optional<ToolAnnotations> annotations;
};

// ListToolsRequest {
//   "description"
//       : "Sent from the client to request a list of tools the server has.",
//         "properties" : {
//           "method" : {"const" : "tools/list", "type" : "string"},
//           "params" : {
//             "properties" : {
//               "cursor" : {
//                 "description" :
//                     "An opaque token representing the current pagination "
//                     "position.\nIf provided, the server should return "
//                     "results starting after this cursor.",
//                 "type" : "string"
//               }
//             },
//             "type" : "object"
//           }
//         },
//                        "required" : ["method"],
//                                     "type" : "object"
// };

/**
 * Sent from the client to request a list of tools the server has.
 */
struct ListToolsRequest : public PaginatedRequest {
    ListToolsRequest() {
        method = MTHD_TOOLS_LIST;
    }
};

// ListToolsResult {
//   "description"
//       : "The server's response to a tools/list request from the client.",
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
//         "tools" : {"items" : {"$ref" : "#/definitions/Tool"}, "type" :
//         "array"}
//       },
//         "required" : ["tools"],
//                      "type" : "object"
// };

/**
 * The server's response to a tools/list request from the client.
 */
struct ListToolsResult : public PaginatedResult {
    vector<Tool> tools;
};

// CallToolResult {
//   "description"
//       : "The server's response to a tool call.\n\nAny errors that originate "
//         "from the tool SHOULD be reported inside the result\nobject, with "
//         "`isError` set to true, _not_ as an MCP protocol-level "
//         "error\nresponse. Otherwise, the LLM would not be able to see that "
//         "an error occurred\nand self-correct.\n\nHowever, any errors in "
//         "_finding_ the tool, an error indicating that the\nserver does not "
//         "support tool calls, or any other exceptional conditions,\nshould be
//         " "reported as an MCP error response.", "properties"
//       : {
//         "_meta" : {
//           "additionalProperties" : {},
//           "description" : "This result property is reserved by the protocol
//           to "
//                           "allow clients and servers to attach additional "
//                           "metadata to their responses.",
//           "type" : "object"
//         },
//         "content" : {
//           "items" : {
//             "anyOf" : [
//               {"$ref" : "#/definitions/TextContent"},
//               {"$ref" : "#/definitions/ImageContent"},
//               {"$ref" : "#/definitions/AudioContent"},
//               {"$ref" : "#/definitions/EmbeddedResource"}
//             ]
//           },
//           "type" : "array"
//         },
//         "isError" : {
//           "description" :
//               "Whether the tool call ended in an error.\n\nIf not set, this
//               is " "assumed to be false (the call was successful).",
//           "type" : "boolean"
//         }
//       },
//         "required" : ["content"],
//                      "type" : "object"
// };

/**
 * The server's response to a tool call.
 *
 * Any errors that originate from the tool SHOULD be reported inside the result
 * object, with `isError` set to true, _not_ as an MCP protocol-level error
 * response. Otherwise, the LLM would not be able to see that an error occurred
 * and self-correct.
 *
 * However, any errors in _finding_ the tool, an error indicating that the
 * server does not support tool calls, or any other exceptional conditions,
 * should be reported as an MCP error response.
 */
struct CallToolResult : public Result {
    vector<variant<TextContent, ImageContent, AudioContent, EmbeddedResource>> content;

    /**
     * Whether the tool call ended in an error.
     *
     * If not set, this is assumed to be false (the call was successful).
     */
    optional<bool> isError;
};

struct CallToolRequestParams {
    string name;
    optional<AdditionalProperties> arguments;
};

// CallToolRequest {
//   "description" : "Used by the client to invoke a tool provided by the
//   server.",
//                   "properties"
//       : {
//         "method" : {"const" : "tools/call", "type" : "string"},
//         "params" : {
//           "properties" : {
//             "arguments" : {"additionalProperties" : {}, "type" : "object"},
//             "name" : {"type" : "string"}
//           },
//           "required" : ["name"],
//           "type" : "object"
//         }
//       },
//         "required" : [ "method", "params" ],
//                      "type" : "object"
// };

/**
 * Used by the client to invoke a tool provided by the server.
 */
struct CallToolRequest : public Request {
    CallToolRequestParams params;

    CallToolRequest() {
        method = MTHD_TOOLS_CALL;
    }
};

// ToolListChangedNotification {
//   "description"
//       : "An optional notification from the server to the client, informing "
//         "it that the list of tools it offers has changed. This may be issued
//         " "by servers without any previous subscription from the client.",
//         "properties"
//       : {
//         "method" :
//             {"const" : "notifications/tools/list_changed", "type" :
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
 * list of tools it offers has changed. This may be issued by servers without
 * any previous subscription from the client.
 */
struct ToolListChangedNotification : public Notification {
    ToolListChangedNotification() {
        method = MTHD_NOTIFICATIONS_TOOLS_LIST_CHANGED;
    }
};

MCP_NAMESPACE_END