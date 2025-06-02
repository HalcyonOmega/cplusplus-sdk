#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// struct InitializeRequest {
//   "description" : "This request is sent from the client to the server when it "
//                   "first connects, asking it to begin initialization.",
//                   "properties"
//       : {
//         "method" : {"const" : "initialize", "type" : "string"},
//         "params" : {
//           "properties" : {
//             "capabilities" : {"$ref" : "#/definitions/ClientCapabilities"},
//             "clientInfo" : {"$ref" : "#/definitions/Implementation"},
//             "protocolVersion" : {
//               "description" :
//                   "The latest version of the Model Context Protocol "
//                   "that the client supports. The client MAY decide to "
//                   "support older versions as well.",
//               "type" : "string"
//             }
//           },
//           "required" : [ "capabilities", "clientInfo", "protocolVersion" ],
//           "type" : "object"
//         }
//       },
//         "required" : [ "method", "params" ],
//                      "type" : "object"
// };

// struct InitializeResult {
//   "description" : "After receiving an initialize request from the client, "
//                   "the server sends this response.",
//                   "properties"
//       : {
//         "_meta" : {
//           "additionalProperties" : {},
//           "description" : "This result property is reserved by the protocol to "
//                           "allow clients and servers to attach additional "
//                           "metadata to their responses.",
//           "type" : "object"
//         },
//         "capabilities" : {"$ref" : "#/definitions/ServerCapabilities"},
//         "instructions" : {
//           "description" :
//               "Instructions describing how to use the server and its "
//               "features.\n\nThis can be used by clients to improve the LLM's "
//               "understanding of available tools, resources, etc. It can be "
//               "thought of like a \"hint\" to the model. For example, this "
//               "information MAY be added to the system prompt.",
//           "type" : "string"
//         },
//         "protocolVersion" : {
//           "description" : "The version of the Model Context Protocol that the "
//                           "server wants to use. This may not match the version "
//                           "that the client requested. If the client cannot "
//                           "support this version, it MUST disconnect.",
//           "type" : "string"
//         },
//         "serverInfo" : {"$ref" : "#/definitions/Implementation"}
//       },
//         "required" : [ "capabilities", "protocolVersion", "serverInfo" ],
//                      "type" : "object"
// };

// struct InitializedNotification {
//   "description" : "This notification is sent from the client to the "
//                   "server after initialization has finished.",
//                   "properties"
//       : {
//         "method" : {"const" : "notifications/initialized", "type" : "string"},
//         "params" : {
//           "additionalProperties" : {},
//           "properties" : {
//             "_meta" : {
//               "additionalProperties" : {},
//               "description" : "This parameter name is reserved by MCP to "
//                               "allow clients and servers to attach "
//                               "additional metadata to their notifications.",
//               "type" : "object"
//             }
//           },
//           "type" : "object"
//         }
//       },
//         "required" : ["method"],
//                      "type" : "object"
// };

/* Initialization */
/**
 * This request is sent from the client to the server when it first connects,
 * asking it to begin initialization.
 */
struct InitializeRequest extends Request {
    method : "initialize";
    params : {
    /**
     * The latest version of the Model Context Protocol that the client supports.
     * The client MAY decide to support older versions as well.
     */
    protocolVersion:
        string;
    capabilities:
        ClientCapabilities;
    clientInfo:
        Implementation;
    };
}

/**
 * After receiving an initialize request from the client, the server sends this
 * response.
 */
struct InitializeResult extends Result {
    /**
     * The version of the Model Context Protocol that the server wants to use. This
     * may not match the version that the client requested. If the client cannot
     * support this version, it MUST disconnect.
     */
    protocolVersion : string;
    capabilities : ServerCapabilities;
    serverInfo : Implementation;

    /**
     * Instructions describing how to use the server and its features.
     *
     * This can be used by clients to improve the LLM's understanding of available
     * tools, resources, etc. It can be thought of like a "hint" to the model. For
     * example, this information MAY be added to the system prompt.
     */
    instructions ?: string;
}

/**
 * This notification is sent from the client to the server after initialization
 * has finished.
 */
struct InitializedNotification extends Notification {
    method : "notifications/initialized";
}

MCP_NAMESPACE_END