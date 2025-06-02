#pragma once

#include "CommonSchemas.h"
#include "Constants.h"
#include "Core.h"

MCP_NAMESPACE_BEGIN

// struct Root {
//     "description" : "Represents a root directory or file that the server can operate on.",
//                     "properties"
//         : {
//             "name": {
//                 "description": "An optional name for the root. This can be used to provide a "
//                                "human-readable\nidentifier for the root, which may be useful for
//                                " "display purposes " "or for\nreferencing the root in other parts
//                                of the application.",
//                 "type": "string"
//             },
//             "uri": {
//                 "description": "The URI identifying the root. This *must* start with file:// for
//                 "
//                                "now.\nThis restriction may be relaxed in future versions of the "
//                                "protocol to allow\nother URI schemes.",
//                 "format": "uri",
//                 "type": "string"
//             }
//         },
//           "required" : ["uri"],
//                        "type" : "object"
// };

// struct RootsListChangedNotification {
//     "description" : "A notification from the client to the server, informing it that the "
//                     "list of roots has changed.\nThis notification should be sent whenever "
//                     "the client adds, removes, or modifies any root.\nThe server should "
//                     "then request an updated list of roots using the ListRootsRequest.",
//                     "properties"
//         : {
//             "method": {"const": "notifications/roots/list_changed", "type": "string"},
//             "params": {
//                 "additionalProperties": {},
//                 "properties": {
//                     "_meta": {
//                         "additionalProperties": {},
//                         "description":
//                             "This parameter name is reserved by MCP to allow clients and "
//                             "servers to attach additional metadata to their notifications.",
//                         "type": "object"
//                     }
//                 },
//                 "type": "object"
//             }
//         },
//           "required" : ["method"],
//                        "type" : "object"
// };

// struct ListRootsRequest {
//     "description"
//         : "Sent from the server to request a list of root URIs from the client. Roots "
//           "allow\nservers "
//           "to ask for specific directories or files to operate on. A common example\nfor roots is
//           " "providing a set of repositories or directories a server should operate\non.\n\nThis
//           " "request is typically used when the server needs to understand the file "
//           "system\nstructure "
//           "or access specific locations that the client has permission to read from.",
//           "properties" : {
//               "method": {"const": "roots/list", "type": "string"},
//               "params": {
//                   "additionalProperties": {},
//                   "properties": {
//                       "_meta": {
//                           "properties": {
//                               "progressToken": {
//                                   "$ref": "#/definitions/ProgressToken",
//                                   "description":
//                                       "If specified, the caller is requesting out-of-band
//                                       progress " "notifications for this request (as represented
//                                       by " "notifications/progress). The value of this parameter
//                                       is an " "opaque " "token that will be attached to any
//                                       subsequent " "notifications. The " "receiver is not
//                                       obligated to provide these notifications."
//                               }
//                           },
//                           "type": "object"
//                       }
//                   },
//                   "type": "object"
//               }
//           },
//                          "required" : ["method"],
//                                       "type" : "object"
// };

// struct ListRootsResult {
//     "description" : "The client's response to a roots/list request from the server.\nThis result
//     "
//                     "contains an array of Root objects, each representing a root directory\nor "
//                     "file that the server can operate on.",
//                     "properties"
//         : {
//             "_meta": {
//                 "additionalProperties": {},
//                 "description": "This result property is reserved by the protocol to allow clients
//                 "
//                                "and servers to attach additional metadata to their responses.",
//                 "type": "object"
//             },
//             "roots": {"items": {"$ref": "#/definitions/Root"}, "type": "array"}
//         },
//           "required" : ["roots"],
//                        "type" : "object"
// };

/* Roots */
/**
 * Sent from the server to request a list of root URIs from the client. Roots allow
 * servers to ask for specific directories or files to operate on. A common example
 * for roots is providing a set of repositories or directories a server should operate
 * on.
 *
 * This request is typically used when the server needs to understand the file system
 * structure or access specific locations that the client has permission to read from.
 */
struct ListRootsRequest : public Request {
    ListRootsRequest() {
        method = MTHD_ROOTS_LIST;
    }
};

/**
 * The client's response to a roots/list request from the server.
 * This result contains an array of Root objects, each representing a root directory
 * or file that the server can operate on.
 */
struct ListRootsResult : public Result {
    roots : Root[];
};

/**
 * Represents a root directory or file that the server can operate on.
 */
struct Root {
    /**
     * The URI identifying the root. This *must* start with file:// for now.
     * This restriction may be relaxed in future versions of the protocol to allow
     * other URI schemes.
     *
     * @format uri
     */
    string uri;
    /**
     * An optional name for the root. This can be used to provide a human-readable
     * identifier for the root, which may be useful for display purposes or for
     * referencing the root in other parts of the application.
     */
    optional<string> name;
};

/**
 * A notification from the client to the server, informing it that the list of roots has changed.
 * This notification should be sent whenever the client adds, removes, or modifies any root.
 * The server should then request an updated list of roots using the ListRootsRequest.
 */
struct RootsListChangedNotification : public Notification {
    RootsListChangedNotification() {
        method = MTHD_NOTIFICATIONS_ROOTS_LIST_CHANGED;
    }
};

MCP_NAMESPACE_END