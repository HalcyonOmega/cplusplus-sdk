#pragma once

#include "CommonSchemas.h"
#include "Constants.h"
#include "Core.h"
#include "NotificationSchemas.h"
#include "RequestSchemas.h"
#include "ResultSchemas.h"
#include "Schemas/Client/ClientSchemas.h"
#include "Schemas/Server/ServerSchemas.h"

MCP_NAMESPACE_BEGIN

struct InitializeRequestParams {
    /**
     * The latest version of the Model Context Protocol that the client supports.
     * The client MAY decide to support older versions as well.
     */
    string protocolVersion;
    /**
     * The capabilities of the client.
     */
    ClientCapabilities capabilities;
    /**
     * Information about the client.
     */
    Implementation clientInfo;
};

// InitializeRequest {
//   MSG_DESCRIPTION : "This request is sent from the client to the server when it "
//                   "first connects, asking it to begin initialization.",
//                   MSG_PROPERTIES
//       : {
//         MSG_METHOD : {MSG_CONST : "initialize", MSG_TYPE : MSG_STRING},
//         MSG_PARAMS : {
//           MSG_PROPERTIES : {
//             MSG_CAPABILITIES : {"$ref" : "#/definitions/ClientCapabilities"},
//             MSG_CLIENT_INFO : {"$ref" : "#/definitions/Implementation"},
//             MSG_PROTOCOL_VERSION : {
//               MSG_DESCRIPTION :
//                   "The latest version of the Model Context Protocol "
//                   "that the client supports. The client MAY decide to "
//                   "support older versions as well.",
//               MSG_TYPE : MSG_STRING
//             }
//           },
//           MSG_REQUIRED : [ MSG_CAPABILITIES, MSG_CLIENT_INFO, MSG_PROTOCOL_VERSION ],
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED : [ MSG_METHOD, MSG_PARAMS ],
//                      MSG_TYPE : MSG_OBJECT
// };

/**
 * This request is sent from the client to the server when it first connects,
 * asking it to begin initialization.
 */
struct InitializeRequest : public Request {
    InitializeRequestParams params;

    InitializeRequest() {
        method = MTHD_INITIALIZE;
    }
};

// InitializeResult {
//   MSG_DESCRIPTION : "After receiving an initialize request from the client, "
//                   "the server sends this response.",
//                   MSG_PROPERTIES
//       : {
//         MSG_META : {
//           MSG_ADDITIONAL_PROPERTIES : {},
//           MSG_DESCRIPTION : "This result property is reserved by the protocol to "
//                           "allow clients and servers to attach additional "
//                           "metadata to their responses.",
//           MSG_TYPE : MSG_OBJECT
//         },
//         MSG_CAPABILITIES : {"$ref" : "#/definitions/ServerCapabilities"},
//         "instructions" : {
//           MSG_DESCRIPTION :
//               "Instructions describing how to use the server and its "
//               "features.\n\nThis can be used by clients to improve the LLM's "
//               "understanding of available tools, resources, etc. It can be "
//               "thought of like a \"hint\" to the model. For example, this "
//               "information MAY be added to the system prompt.",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_PROTOCOL_VERSION : {
//           MSG_DESCRIPTION : "The version of the Model Context Protocol that the "
//                           "server wants to use. This may not match the version "
//                           "that the client requested. If the client cannot "
//                           "support this version, it MUST disconnect.",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_SERVER_INFO : {"$ref" : "#/definitions/Implementation"}
//       },
//         MSG_REQUIRED : [ MSG_CAPABILITIES, MSG_PROTOCOL_VERSION, MSG_SERVER_INFO ],
//                      MSG_TYPE : MSG_OBJECT
// };

/**
 * After receiving an initialize request from the client, the server sends this
 * response.
 */
struct InitializeResult : public Result {
    /**
     * The version of the Model Context Protocol that the server wants to use. This
     * may not match the version that the client requested. If the client cannot
     * support this version, it MUST disconnect.
     */
    string protocolVersion;
    ServerCapabilities capabilities;
    Implementation serverInfo;

    /**
     * Instructions describing how to use the server and its features.
     *
     * This can be used by clients to improve the LLM's understanding of available
     * tools, resources, etc. It can be thought of like a "hint" to the model. For
     * example, this information MAY be added to the system prompt.
     */
    optional<string> instructions;
};

// InitializedNotification {
//   MSG_DESCRIPTION : "This notification is sent from the client to the "
//                   "server after initialization has finished.",
//                   MSG_PROPERTIES
//       : {
//         MSG_METHOD : {MSG_CONST : "notifications/initialized", MSG_TYPE : MSG_STRING},
//         MSG_PARAMS : {
//           MSG_ADDITIONAL_PROPERTIES : {},
//           MSG_PROPERTIES : {
//             MSG_META : {
//               MSG_ADDITIONAL_PROPERTIES : {},
//               MSG_DESCRIPTION : "This parameter name is reserved by MCP to "
//                               "allow clients and servers to attach "
//                               "additional metadata to their notifications.",
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
 * This notification is sent from the client to the server after initialization
 * has finished.
 */
struct InitializedNotification : public Notification {
    InitializedNotification() {
        method = MTHD_NOTIFICATIONS_INITIALIZED;
    }
};

MCP_NAMESPACE_END