#pragma once

#include "Core.h"
#include "Core/Types/Capabilities.h"
#include "Core/Types/Implementation.h"
#include "MethodConstants.h"
#include "NotificationBase.h"
#include "RequestBase.h"
#include "ResponseBase.h"

MCP_NAMESPACE_BEGIN

// InitializeRequest {
//   MSG_DESCRIPTION : "This request is sent from the client to the server when it "
//                   "first connects, asking it to begin initialization.",
//                   MSG_PROPERTIES
//       : {
//         MSG_METHOD : {MSG_CONST : MTHD_INITIALIZE, MSG_TYPE : MSG_STRING},
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

// This request is sent from the client to the server when it first connects, asking it to begin
// initialization.
struct InitializeRequest : public RequestBase {
    struct InitializeRequestParams {
        string
            ProtocolVersion; // The latest version of the Model Context Protocol that the client
                             // supports. The client MAY decide to support older versions as well.
        ClientCapabilities Capabilities; // The capabilities of the client.
        Implementation ClientInfo;       // Information about the client.
    };

    InitializeRequestParams Params;

    InitializeRequest() : RequestBase(MTHD_INITIALIZE) {}
};

// TODO: @HalcyonOmega - Fix This
const isInitializeRequest =
    (value : unknown) : value is InitializeRequest = > InitializeRequest.safeParse(value).success;

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
//         MSG_INSTRUCTIONS : {
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

// After receiving an initialize request from the client, the server sends this response.
struct InitializeResult : public ResponseBase {
    string ProtocolVersion; // The version of the Model Context Protocol that the server wants to
                            // use. This may not match the version that the client requested. If the
                            // client cannot support this version, it MUST disconnect.
    ServerCapabilities Capabilities; // The capabilities of the server.
    Implementation ServerInfo;       // Information about the server.
    optional<string> Instructions;   // Instructions describing how to use the server and its
                                     // features. This can be used by clients to improve the LLM's
                                     // understanding of available tools, resources, etc. It can be
                                     // thought of like a "hint" to the model. For example, this
                                     // information MAY be added to the system prompt.
};

// InitializedNotification {
//   MSG_DESCRIPTION : "This notification is sent from the client to the "
//                   "server after initialization has finished.",
//                   MSG_PROPERTIES
//       : {
//         MSG_METHOD : {MSG_CONST : MTHD_NOTIFICATIONS_INITIALIZED, MSG_TYPE : MSG_STRING},
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

// This notification is sent from the client to the server after initialization has finished.
struct InitializedNotification : public NotificationBase {
    InitializedNotification() : NotificationBase(MTHD_NOTIFICATIONS_INITIALIZED) {}
};

// TODO: @HalcyonOmega - Fix This
const isInitializedNotification = (value : unknown)
    : value is InitializedNotification = > InitializedNotification.safeParse(value).success;

MCP_NAMESPACE_END