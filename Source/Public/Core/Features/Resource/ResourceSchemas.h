#pragma once

#include "Constants.h"
#include "Core.h"
#include "NotificationSchemas.h"
#include "RequestSchemas.h"
#include "ResultSchemas.h"
#include "ToolSchemas.h"

MCP_NAMESPACE_BEGIN

// Resource {
//   MSG_DESCRIPTION : "A known resource that the server is capable of reading.",
//                   MSG_PROPERTIES
//       : {
//         MSG_ANNOTATIONS : {
//           "$ref" : "#/definitions/Annotations",
//           MSG_DESCRIPTION : "Optional annotations for the client."
//         },
//         MSG_DESCRIPTION : {
//           MSG_DESCRIPTION :
//               "A description of what this resource represents.\n\nThis can be
//               " "used " "by clients to improve the LLM's understanding of
//               available " "resources. It can be thought of like a \"hint\" to
//               the model.",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_MIME_TYPE : {
//           MSG_DESCRIPTION : "The MIME type of this resource, if known.",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_NAME : {
//           MSG_DESCRIPTION :
//               "A human-readable name for this resource.\n\nThis can be "
//               "used by clients to populate UI elements.",
//           MSG_TYPE : MSG_STRING
//         },
//         "size" : {
//           MSG_DESCRIPTION :
//               "The size of the raw resource content, in bytes (i.e., before "
//               "base64 "
//               "encoding or any tokenization), if known.\n\nThis can be used
//               by " "Hosts to display file sizes and estimate context window
//               usage.",
//           MSG_TYPE : "integer"
//         },
//         MSG_URI : {
//           MSG_DESCRIPTION : "The URI of this resource.",
//           MSG_FORMAT : MSG_URI,
//           MSG_TYPE : MSG_STRING
//         }
//       },
//         MSG_REQUIRED : [ MSG_NAME, MSG_URI ],
//                      MSG_TYPE : MSG_OBJECT
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

// ResourceTemplate {
//   MSG_DESCRIPTION
//       : "A template description for resources available on the server.",
//         MSG_PROPERTIES
//       : {
//         MSG_ANNOTATIONS : {
//           "$ref" : "#/definitions/Annotations",
//           MSG_DESCRIPTION : "Optional annotations for the client."
//         },
//         MSG_DESCRIPTION : {
//           MSG_DESCRIPTION :
//               "A description of what this template is for.\n\nThis can be
//               used " "by clients to improve the LLM's understanding of
//               available " "resources. It can be thought of like a \"hint\" to
//               the model.",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_MIME_TYPE : {
//           MSG_DESCRIPTION :
//               "The MIME type for all resources that match this template. This
//               " "should only be included if all resources matching this
//               template " "have the same type.",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_NAME : {
//           MSG_DESCRIPTION : "A human-readable name for the type of resource
//           this "
//                           "template refers to.\n\nThis can be used by clients
//                           " "to populate UI elements.",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_URI_TEMPLATE : {
//           MSG_DESCRIPTION : "A URI template (according to RFC 6570) that can be
//           "
//                           "used to construct resource URIs.",
//           MSG_FORMAT : MSG_URITEMPLATE,
//           MSG_TYPE : MSG_STRING
//         }
//       },
//         MSG_REQUIRED : [ MSG_NAME, MSG_URI_TEMPLATE ],
//                      MSG_TYPE : MSG_OBJECT
// };

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

// ListResourcesRequest {
//   MSG_DESCRIPTION
//       : "Sent from the client to request a list of resources the server
//       has.",
//         MSG_PROPERTIES : {
//           MSG_METHOD : {MSG_CONST : MTHD_RESOURCES_LIST, MSG_TYPE : MSG_STRING},
//           MSG_PARAMS : {
//             MSG_PROPERTIES : {
//               MSG_CURSOR : {
//                 MSG_DESCRIPTION :
//                     "An opaque token representing the current pagination "
//                     "position.\nIf provided, the server should return results
//                     " "starting after this cursor.",
//                 MSG_TYPE : MSG_STRING
//               }
//             },
//             MSG_TYPE : MSG_OBJECT
//           }
//         },
//                        MSG_REQUIRED : [MSG_METHOD],
//                                     MSG_TYPE : MSG_OBJECT
// };

/**
 * Sent from the client to request a list of resources the server has.
 */
struct ListResourcesRequest : public PaginatedRequest {
    ListResourcesRequest() {
        method = MTHD_RESOURCES_LIST;
    }
};

// ListResourcesResult {
//   MSG_DESCRIPTION
//       : "The server's response to a resources/list request from the client.",
//         MSG_PROPERTIES
//       : {
//         MSG_META : {
//           MSG_ADDITIONAL_PROPERTIES : {},
//           MSG_DESCRIPTION : "This result property is reserved by the protocol
//           to "
//                           "allow clients and servers to attach additional "
//                           "metadata to their responses.",
//           MSG_TYPE : MSG_OBJECT
//         },
//         MSG_NEXT_CURSOR : {
//           MSG_DESCRIPTION : "An opaque token representing the pagination "
//                           "position after the last returned result.\nIf "
//                           "present, there may be more results available.",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_RESOURCES :
//             {MSG_ITEMS : {"$ref" : "#/definitions/Resource"}, MSG_TYPE : MSG_ARRAY}
//       },
//         MSG_REQUIRED : [MSG_RESOURCES],
//                      MSG_TYPE : MSG_OBJECT
// };

/**
 * The server's response to a resources/list request from the client.
 */
struct ListResourcesResult : public PaginatedResult {
    vector<Resource> resources;
};

// ListResourceTemplatesRequest {
//   MSG_DESCRIPTION : "Sent from the client to request a list of resource "
//                   "templates the server has.",
//                   MSG_PROPERTIES
//       : {
//         MSG_METHOD : {MSG_CONST : MTHD_RESOURCES_TEMPLATES_LIST, MSG_TYPE : MSG_STRING},
//         MSG_PARAMS : {
//           MSG_PROPERTIES : {
//             MSG_CURSOR : {
//               MSG_DESCRIPTION :
//                   "An opaque token representing the current pagination "
//                   "position.\nIf provided, the server should return "
//                   "results starting after this cursor.",
//               MSG_TYPE : MSG_STRING
//             }
//           },
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED : [MSG_METHOD],
//                      MSG_TYPE : MSG_OBJECT
// };

/**
 * Sent from the client to request a list of resource templates the server has.
 */
struct ListResourceTemplatesRequest : public PaginatedRequest {
    ListResourceTemplatesRequest() {
        method = MTHD_RESOURCES_TEMPLATES_LIST;
    }
};

// ListResourceTemplatesResult {
//   MSG_DESCRIPTION : "The server's response to a resources/templates/list "
//                   "request from the client.",
//                   MSG_PROPERTIES
//       : {
//         MSG_META : {
//           MSG_ADDITIONAL_PROPERTIES : {},
//           MSG_DESCRIPTION : "This result property is reserved by the protocol
//           to "
//                           "allow clients and servers to attach additional "
//                           "metadata to their responses.",
//           MSG_TYPE : MSG_OBJECT
//         },
//         MSG_NEXT_CURSOR : {
//           MSG_DESCRIPTION : "An opaque token representing the pagination "
//                           "position after the last returned result.\nIf "
//                           "present, there may be more results available.",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_RESOURCE_TEMPLATES : {
//           MSG_ITEMS : {"$ref" : "#/definitions/ResourceTemplate"},
//           MSG_TYPE : MSG_ARRAY
//         }
//       },
//         MSG_REQUIRED : [MSG_RESOURCE_TEMPLATES],
//                      MSG_TYPE : MSG_OBJECT
// };

/**
 * The server's response to a resources/templates/list request from the client.
 */
struct ListResourceTemplatesResult : public PaginatedResult {
    vector<ResourceTemplate> resourceTemplates;
};

struct ResourceUpdatedNotificationParams {
    /**
     * The URI of the resource that has been updated. This might be a sub-resource
     * of the one that the client actually subscribed to.
     *
     * @format uri
     */
    string uri;
};

// ResourceUpdatedNotification {
//   MSG_DESCRIPTION : "A notification from the server to the client, "
//                   "informing it that a resource has changed and may need "
//                   "to be read again. This should only be sent if the "
//                   "client previously sent a resources/subscribe request.",
//                   MSG_PROPERTIES
//       : {
//         MSG_METHOD :
//             {MSG_CONST : MTHD_NOTIFICATIONS_RESOURCES_UPDATED, MSG_TYPE : MSG_STRING},
//         MSG_PARAMS : {
//           MSG_PROPERTIES : {
//             MSG_URI : {
//               MSG_DESCRIPTION : "The URI of the resource that has been "
//                               "updated. This might be a sub-resource of the "
//                               "one that the client actually subscribed to.",
//               MSG_FORMAT : MSG_URI,
//               MSG_TYPE : MSG_STRING
//             }
//           },
//           MSG_REQUIRED : [MSG_URI],
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED : [ MSG_METHOD, MSG_PARAMS ],
//                      MSG_TYPE : MSG_OBJECT
// };

/**
 * A notification from the server to the client, informing it that a resource
 * has changed and may need to be read again. This should only be sent if the
 * client previously sent a resources/subscribe request.
 */
struct ResourceUpdatedNotification : public Notification {
    ResourceUpdatedNotification() {
        method = MTHD_NOTIFICATIONS_RESOURCES_UPDATED;
    }
    ResourceUpdatedNotificationParams params;
};

struct SubscribeRequestParams {
    /**
     * The URI of the resource to subscribe to. The URI can use any protocol; it
     * is up to the server how to interpret it.
     *
     * @format uri
     */
    string uri;
};

// SubscribeRequest {
//   MSG_DESCRIPTION
//       : "Sent from the client to request resources/updated notifications from
//       "
//         "the server whenever a particular resource changes.",
//         MSG_PROPERTIES : {
//           MSG_METHOD : {MSG_CONST : MTHD_RESOURCES_SUBSCRIBE, MSG_TYPE : MSG_STRING},
//           MSG_PARAMS : {
//             MSG_PROPERTIES : {
//               MSG_URI : {
//                 MSG_DESCRIPTION :
//                     "The URI of the resource to subscribe to. The URI can use
//                     " "any " "protocol; it is up to the server how to
//                     interpret it.",
//                 MSG_FORMAT : MSG_URI,
//                 MSG_TYPE : MSG_STRING
//               }
//             },
//             MSG_REQUIRED : [MSG_URI],
//             MSG_TYPE : MSG_OBJECT
//           }
//         },
//                        MSG_REQUIRED : [ MSG_METHOD, MSG_PARAMS ],
//                                     MSG_TYPE : MSG_OBJECT
// };

/**
 * Sent from the client to request resources/updated notifications from the
 * server whenever a particular resource changes.
 */
struct SubscribeRequest : public Request {
    SubscribeRequest() {
        method = MTHD_RESOURCES_SUBSCRIBE;
    }
    SubscribeRequestParams params;
};

struct UnsubscribeRequestParams {
    /**
     * The URI of the resource to unsubscribe from.
     *
     * @format uri
     */
    string uri;
};

// UnsubscribeRequest {
//   MSG_DESCRIPTION : "Sent from the client to request cancellation of "
//                   "resources/updated notifications from the server. This "
//                   "should follow a previous resources/subscribe request.",
//                   MSG_PROPERTIES
//       : {
//         MSG_METHOD : {MSG_CONST : MTHD_RESOURCES_UNSUBSCRIBE, MSG_TYPE : MSG_STRING},
//         MSG_PARAMS : {
//           MSG_PROPERTIES : {
//             MSG_URI : {
//               MSG_DESCRIPTION : "The URI of the resource to unsubscribe from.",
//               MSG_FORMAT : MSG_URI,
//               MSG_TYPE : MSG_STRING
//             }
//           },
//           MSG_REQUIRED : [MSG_URI],
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED : [ MSG_METHOD, MSG_PARAMS ],
//                      MSG_TYPE : MSG_OBJECT
// };

/**
 * Sent from the client to request cancellation of resources/updated
 * notifications from the server. This should follow a previous
 * resources/subscribe request.
 */
struct UnsubscribeRequest : public Request {
    UnsubscribeRequest() {
        method = MTHD_RESOURCES_UNSUBSCRIBE;
    }
    UnsubscribeRequestParams params;
};

struct ReadResourceRequestParams {
    /**
     * The URI of the resource to read. The URI can use any protocol; it is up to
     * the server how to interpret it.
     *
     * @format uri
     */
    string uri;
};

// ReadResourceRequest {
//   MSG_DESCRIPTION
//       : "Sent from the client to the server, to read a specific resource
//       URI.",
//         MSG_PROPERTIES : {
//           MSG_METHOD : {MSG_CONST : MTHD_RESOURCES_READ, MSG_TYPE : MSG_STRING},
//           MSG_PARAMS : {
//             MSG_PROPERTIES : {
//               MSG_URI : {
//                 MSG_DESCRIPTION : "The URI of the resource to read. The URI can
//                 "
//                                 "use any protocol; "
//                                 "it is up to the server how to interpret
//                                 it.",
//                 MSG_FORMAT : MSG_URI,
//                 MSG_TYPE : MSG_STRING
//               }
//             },
//             MSG_REQUIRED : [MSG_URI],
//             MSG_TYPE : MSG_OBJECT
//           }
//         },
//                        MSG_REQUIRED : [ MSG_METHOD, MSG_PARAMS ],
//                                     MSG_TYPE : MSG_OBJECT
// };

/**
 * Sent from the client to the server, to read a specific resource URI.
 */
struct ReadResourceRequest : public Request {
    ReadResourceRequest() {
        method = MTHD_RESOURCES_READ;
    }
    ReadResourceRequestParams params;
};

// ReadResourceResult {
//   MSG_DESCRIPTION
//       : "The server's response to a resources/read request from the client.",
//         MSG_PROPERTIES
//       : {
//         MSG_META : {
//           MSG_ADDITIONAL_PROPERTIES : {},
//           MSG_DESCRIPTION : "This result property is reserved by the protocol
//           to "
//                           "allow clients and servers to attach additional "
//                           "metadata to their responses.",
//           MSG_TYPE : MSG_OBJECT
//         },
//         MSG_CONTENTS : {
//           MSG_ITEMS : {
//             "anyOf" : [
//               {"$ref" : "#/definitions/TextResourceContents"},
//               {"$ref" : "#/definitions/BlobResourceContents"}
//             ]
//           },
//           MSG_TYPE : MSG_ARRAY
//         }
//       },
//         MSG_REQUIRED : [MSG_CONTENTS],
//                      MSG_TYPE : MSG_OBJECT
// };

/**
 * The server's response to a resources/read request from the client.
 */
struct ReadResourceResult : public Result {
    vector<variant<TextResourceContents, BlobResourceContents>> contents;
};

// ResourceListChangedNotification {
//   MSG_DESCRIPTION : "An optional notification from the server to the client, "
//                   "informing it that the list of resources it can read "
//                   "from has changed. This may be issued by servers without "
//                   "any previous subscription from the client.",
//                   MSG_PROPERTIES
//       : {
//         MSG_METHOD : {
//           MSG_CONST : MTHD_NOTIFICATIONS_RESOURCES_LIST_CHANGED,
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_PARAMS : {
//           MSG_ADDITIONAL_PROPERTIES : {},
//           MSG_PROPERTIES : {
//             MSG_META : {
//               MSG_ADDITIONAL_PROPERTIES : {},
//               MSG_DESCRIPTION : "This parameter name is reserved by MCP to
//               allow "
//                               "clients and servers to attach additional "
//                               "metadata to their notifications.",
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