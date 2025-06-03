#pragma once

#include "Constants.h"
#include "Core.h"
#include "NotificationSchemas.h"
#include "RequestSchemas.h"
#include "ResultSchemas.h"
MCP_NAMESPACE_BEGIN

// struct Resource {
//   "description" : "A known resource that the server is capable of reading.",
//                   "properties"
//       : {
//         "annotations" : {
//           "$ref" : "#/definitions/Annotations",
//           "description" : "Optional annotations for the client."
//         },
//         "description" : {
//           "description" :
//               "A description of what this resource represents.\n\nThis can be
//               " "used " "by clients to improve the LLM's understanding of
//               available " "resources. It can be thought of like a \"hint\" to
//               the model.",
//           "type" : "string"
//         },
//         "mimeType" : {
//           "description" : "The MIME type of this resource, if known.",
//           "type" : "string"
//         },
//         "name" : {
//           "description" :
//               "A human-readable name for this resource.\n\nThis can be "
//               "used by clients to populate UI elements.",
//           "type" : "string"
//         },
//         "size" : {
//           "description" :
//               "The size of the raw resource content, in bytes (i.e., before "
//               "base64 "
//               "encoding or any tokenization), if known.\n\nThis can be used
//               by " "Hosts to display file sizes and estimate context window
//               usage.",
//           "type" : "integer"
//         },
//         "uri" : {
//           "description" : "The URI of this resource.",
//           "format" : "uri",
//           "type" : "string"
//         }
//       },
//         "required" : [ "name", "uri" ],
//                      "type" : "object"
// };

// struct ResourceContents {
//   "description" : "The contents of a specific resource or sub-resource.",
//                   "properties"
//       : {
//         "mimeType" : {
//           "description" : "The MIME type of this resource, if known.",
//           "type" : "string"
//         },
//         "uri" : {
//           "description" : "The URI of this resource.",
//           "format" : "uri",
//           "type" : "string"
//         }
//       },
//         "required" : ["uri"],
//                      "type" : "object"
// };

// struct ResourceListChangedNotification {
//   "description" : "An optional notification from the server to the client, "
//                   "informing it that the list of resources it can read "
//                   "from has changed. This may be issued by servers without "
//                   "any previous subscription from the client.",
//                   "properties"
//       : {
//         "method" : {
//           "const" : "notifications/resources/list_changed",
//           "type" : "string"
//         },
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

// struct ResourceTemplate {
//   "description"
//       : "A template description for resources available on the server.",
//         "properties"
//       : {
//         "annotations" : {
//           "$ref" : "#/definitions/Annotations",
//           "description" : "Optional annotations for the client."
//         },
//         "description" : {
//           "description" :
//               "A description of what this template is for.\n\nThis can be
//               used " "by clients to improve the LLM's understanding of
//               available " "resources. It can be thought of like a \"hint\" to
//               the model.",
//           "type" : "string"
//         },
//         "mimeType" : {
//           "description" :
//               "The MIME type for all resources that match this template. This
//               " "should only be included if all resources matching this
//               template " "have the same type.",
//           "type" : "string"
//         },
//         "name" : {
//           "description" : "A human-readable name for the type of resource
//           this "
//                           "template refers to.\n\nThis can be used by clients
//                           " "to populate UI elements.",
//           "type" : "string"
//         },
//         "uriTemplate" : {
//           "description" : "A URI template (according to RFC 6570) that can be
//           "
//                           "used to construct resource URIs.",
//           "format" : "uri-template",
//           "type" : "string"
//         }
//       },
//         "required" : [ "name", "uriTemplate" ],
//                      "type" : "object"
// };

// struct ResourceUpdatedNotification {
//   "description" : "A notification from the server to the client, "
//                   "informing it that a resource has changed and may need "
//                   "to be read again. This should only be sent if the "
//                   "client previously sent a resources/subscribe request.",
//                   "properties"
//       : {
//         "method" :
//             {"const" : "notifications/resources/updated", "type" : "string"},
//         "params" : {
//           "properties" : {
//             "uri" : {
//               "description" : "The URI of the resource that has been "
//                               "updated. This might be a sub-resource of the "
//                               "one that the client actually subscribed to.",
//               "format" : "uri",
//               "type" : "string"
//             }
//           },
//           "required" : ["uri"],
//           "type" : "object"
//         }
//       },
//         "required" : [ "method", "params" ],
//                      "type" : "object"
// };

// struct TextResourceContents {
//   "properties" : {
//     "mimeType" : {
//       "description" : "The MIME type of this resource, if known.",
//       "type" : "string"
//     },
//     "text" : {
//       "description" : "The text of the item. This must only be set if the
//       item "
//                       "can actually be represented as text (not binary
//                       data).",
//       "type" : "string"
//     },
//     "uri" : {
//       "description" : "The URI of this resource.",
//       "format" : "uri",
//       "type" : "string"
//     }
//   },
//                  "required" : [ "text", "uri" ],
//                               "type" : "object"
// };

// struct BlobResourceContents {
//   "properties" : {
//     "blob" : {
//       "description" :
//           "A base64-encoded string representing the binary data of the
//           item.",
//       "format" : "byte",
//       "type" : "string"
//     },
//     "mimeType" : {
//       "description" : "The MIME type of this resource, if known.",
//       "type" : "string"
//     },
//     "uri" : {
//       "description" : "The URI of this resource.",
//       "format" : "uri",
//       "type" : "string"
//     }
//   },
//                  "required" : [ "blob", "uri" ],
//                               "type" : "object"
// };

// struct EmbeddedResource {
//   "description"
//       : "The contents of a resource, embedded into a prompt or tool call "
//         "result.\n\nIt is up to the client how best to render embedded "
//         "resources "
//         "for the benefit\nof the LLM and/or the user.",
//         "properties" : {
//           "annotations" : {
//             "$ref" : "#/definitions/Annotations",
//             "description" : "Optional annotations for the client."
//           },
//           "resource" : {
//             "anyOf" : [
//               {"$ref" : "#/definitions/TextResourceContents"},
//               {"$ref" : "#/definitions/BlobResourceContents"}
//             ]
//           },
//           "type" : {"const" : "resource", "type" : "string"}
//         },
//                        "required" : [ "resource", "type" ],
//                                     "type" : "object"
// };

// struct ListResourceTemplatesRequest {
//   "description" : "Sent from the client to request a list of resource "
//                   "templates the server has.",
//                   "properties"
//       : {
//         "method" : {"const" : "resources/templates/list", "type" : "string"},
//         "params" : {
//           "properties" : {
//             "cursor" : {
//               "description" :
//                   "An opaque token representing the current pagination "
//                   "position.\nIf provided, the server should return "
//                   "results starting after this cursor.",
//               "type" : "string"
//             }
//           },
//           "type" : "object"
//         }
//       },
//         "required" : ["method"],
//                      "type" : "object"
// };

// struct ListResourceTemplatesResult {
//   "description" : "The server's response to a resources/templates/list "
//                   "request from the client.",
//                   "properties"
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
//         "resourceTemplates" : {
//           "items" : {"$ref" : "#/definitions/ResourceTemplate"},
//           "type" : "array"
//         }
//       },
//         "required" : ["resourceTemplates"],
//                      "type" : "object"
// };

// struct ListResourcesRequest {
//   "description"
//       : "Sent from the client to request a list of resources the server
//       has.",
//         "properties" : {
//           "method" : {"const" : "resources/list", "type" : "string"},
//           "params" : {
//             "properties" : {
//               "cursor" : {
//                 "description" :
//                     "An opaque token representing the current pagination "
//                     "position.\nIf provided, the server should return results
//                     " "starting after this cursor.",
//                 "type" : "string"
//               }
//             },
//             "type" : "object"
//           }
//         },
//                        "required" : ["method"],
//                                     "type" : "object"
// };

// struct ListResourcesResult {
//   "description"
//       : "The server's response to a resources/list request from the client.",
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
//         "resources" :
//             {"items" : {"$ref" : "#/definitions/Resource"}, "type" : "array"}
//       },
//         "required" : ["resources"],
//                      "type" : "object"
// };

// struct ReadResourceRequest {
//   "description"
//       : "Sent from the client to the server, to read a specific resource
//       URI.",
//         "properties" : {
//           "method" : {"const" : "resources/read", "type" : "string"},
//           "params" : {
//             "properties" : {
//               "uri" : {
//                 "description" : "The URI of the resource to read. The URI can
//                 "
//                                 "use any protocol; "
//                                 "it is up to the server how to interpret
//                                 it.",
//                 "format" : "uri",
//                 "type" : "string"
//               }
//             },
//             "required" : ["uri"],
//             "type" : "object"
//           }
//         },
//                        "required" : [ "method", "params" ],
//                                     "type" : "object"
// };

// struct ReadResourceResult {
//   "description"
//       : "The server's response to a resources/read request from the client.",
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
//         "contents" : {
//           "items" : {
//             "anyOf" : [
//               {"$ref" : "#/definitions/TextResourceContents"},
//               {"$ref" : "#/definitions/BlobResourceContents"}
//             ]
//           },
//           "type" : "array"
//         }
//       },
//         "required" : ["contents"],
//                      "type" : "object"
// };

// struct SubscribeRequest {
//   "description"
//       : "Sent from the client to request resources/updated notifications from
//       "
//         "the server whenever a particular resource changes.",
//         "properties" : {
//           "method" : {"const" : "resources/subscribe", "type" : "string"},
//           "params" : {
//             "properties" : {
//               "uri" : {
//                 "description" :
//                     "The URI of the resource to subscribe to. The URI can use
//                     " "any " "protocol; it is up to the server how to
//                     interpret it.",
//                 "format" : "uri",
//                 "type" : "string"
//               }
//             },
//             "required" : ["uri"],
//             "type" : "object"
//           }
//         },
//                        "required" : [ "method", "params" ],
//                                     "type" : "object"
// };

// struct UnsubscribeRequest {
//   "description" : "Sent from the client to request cancellation of "
//                   "resources/updated notifications from the server. This "
//                   "should follow a previous resources/subscribe request.",
//                   "properties"
//       : {
//         "method" : {"const" : "resources/unsubscribe", "type" : "string"},
//         "params" : {
//           "properties" : {
//             "uri" : {
//               "description" : "The URI of the resource to unsubscribe from.",
//               "format" : "uri",
//               "type" : "string"
//             }
//           },
//           "required" : ["uri"],
//           "type" : "object"
//         }
//       },
//         "required" : [ "method", "params" ],
//                      "type" : "object"
// };

/**
 * A known resource that the server is capable of reading.
 */
struct Resource {
    /**
     * The URI of this resource.
     *
     * @format uri
     */
    string uri;

    /**
     * A human-readable name for this resource.
     *
     * This can be used by clients to populate UI elements.
     */
    string name;

    /**
     * A description of what this resource represents.
     *
     * This can be used by clients to improve the LLM's understanding of available
     * resources. It can be thought of like a "hint" to the model.
     */
    optional<string> description;

    /**
     * The MIME type of this resource, if known.
     */
    optional<string> mimeType;

    /**
     * Optional annotations for the client.
     */
    optional<Annotations> annotations;

    /**
     * The size of the raw resource content, in bytes (i.e., before base64
     * encoding or any tokenization), if known.
     *
     * This can be used by Hosts to display file sizes and estimate context window
     * usage.
     */
    optional<number> size;
};

/**
 * A template description for resources available on the server.
 */
struct ResourceTemplate {
    /**
     * A URI template (according to RFC 6570) that can be used to construct
     * resource URIs.
     *
     * @format uri-template
     */
    string uriTemplate;

    /**
     * A human-readable name for the type of resource this template refers to.
     *
     * This can be used by clients to populate UI elements.
     */
    string name;

    /**
     * A description of what this template is for.
     *
     * This can be used by clients to improve the LLM's understanding of available
     * resources. It can be thought of like a "hint" to the model.
     */
    optional<string> description;

    /**
     * The MIME type for all resources that match this template. This should only
     * be included if all resources matching this template have the same type.
     */
    optional<string> mimeType;

    /**
     * Optional annotations for the client.
     */
    optional<Annotations> annotations;
};

/**
 * The contents of a specific resource or sub-resource.
 */
struct ResourceContents {
    /**
     * The URI of this resource.
     *
     * @format uri
     */
    string uri;

    /**
     * The MIME type of this resource, if known.
     */
    optional<string> mimeType;
};

struct TextResourceContents : public ResourceContents {
    /**
     * The text of the item. This must only be set if the item can actually be
     * represented as text (not binary data).
     */
    string text;
};

struct BlobResourceContents : public ResourceContents {
    /**
     * A base64-encoded string representing the binary data of the item.
     *
     * @format byte
     */
    string blob;
};

/**
 * Sent from the client to request a list of resources the server has.
 */
struct ListResourcesRequest : public PaginatedRequest {
    ListResourcesRequest() {
        method = MTHD_RESOURCES_LIST;
    }
};

/**
 * The server's response to a resources/list request from the client.
 */
struct ListResourcesResult : public PaginatedResult {
    vector<Resource> resources;
};

/**
 * Sent from the client to request a list of resource templates the server has.
 */
struct ListResourceTemplatesRequest : public PaginatedRequest {
    ListResourceTemplatesRequest() {
        method = MTHD_RESOURCES_TEMPLATES_LIST;
    }
};

/**
 * The server's response to a resources/templates/list request from the client.
 */
struct ListResourceTemplatesResult : public PaginatedResult {
    vector<ResourceTemplate> resourceTemplates;
};

/**
 * A notification from the server to the client, informing it that a resource
 * has changed and may need to be read again. This should only be sent if the
 * client previously sent a resources/subscribe request.
 */
struct ResourceUpdatedNotification : public Notification {
    ResourceUpdatedNotification() {
        method = MTHD_NOTIFICATIONS_RESOURCES_UPDATED;
    }
    params : {
        /**
         * The URI of the resource that has been updated. This might be a sub-resource
         * of the one that the client actually subscribed to.
         *
         * @format uri
         */
        string uri;
    };
};

/**
 * Sent from the client to request resources/updated notifications from the
 * server whenever a particular resource changes.
 */
struct SubscribeRequest : public Request {
    SubscribeRequest() {
        method = MTHD_RESOURCES_SUBSCRIBE;
    }
    params : {
        /**
         * The URI of the resource to subscribe to. The URI can use any protocol; it
         * is up to the server how to interpret it.
         *
         * @format uri
         */
        string uri;
    };
};

/**
 * Sent from the client to request cancellation of resources/updated
 * notifications from the server. This should follow a previous
 * resources/subscribe request.
 */
struct UnsubscribeRequest : public Request {
    UnsubscribeRequest() {
        method = MTHD_RESOURCES_UNSUBSCRIBE;
    }
    params : {
        /**
         * The URI of the resource to unsubscribe from.
         *
         * @format uri
         */
        string uri;
    };
};

/**
 * Sent from the client to the server, to read a specific resource URI.
 */
struct ReadResourceRequest : public Request {
    ReadResourceRequest() {
        method = MTHD_RESOURCES_READ;
    }
    params : {
        /**
         * The URI of the resource to read. The URI can use any protocol; it is up to
         * the server how to interpret it.
         *
         * @format uri
         */
        string uri;
    };
};

/**
 * The server's response to a resources/read request from the client.
 */
struct ReadResourceResult : public Result {
    contents : (TextResourceContents | BlobResourceContents)[];
};

/**
 * An optional notification from the server to the client, informing it that the
 * list of resources it can read from has changed. This may be issued by servers
 * without any previous subscription from the client.
 */
struct ResourceListChangedNotification : public Notification {
    ResourceListChangedNotification() {
        method = MTHD_NOTIFICATIONS_RESOURCES_LIST_CHANGED;
    }
};

MCP_NAMESPACE_END