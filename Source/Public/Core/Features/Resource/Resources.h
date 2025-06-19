#pragma once

#include "Core/Types/Annotations.h"
#include "Core/Types/Content.h"
#include "MethodConstants.h"
#include "NotificationBase.h"
#include "RequestBase.h"
#include "ResourceBase.h"
#include "ResponseBase.h"

MCP_NAMESPACE_BEGIN

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

// Sent from the client to request a list of resources the server has.
struct ListResourcesRequest : public PaginatedRequest {
    ListResourcesRequest() : PaginatedRequest(MTHD_RESOURCES_LIST) {}
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

// The server's response to a resources/list request from the client.
struct ListResourcesResult : public PaginatedResult {
    vector<Resource> Resources;
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

// Sent from the client to request a list of resource templates the server has.
struct ListResourceTemplatesRequest : public PaginatedRequest {
    ListResourceTemplatesRequest() : PaginatedRequest(MTHD_RESOURCES_TEMPLATES_LIST) {}
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

// The server's response to a resources/templates/list request from the client.
struct ListResourceTemplatesResult : public PaginatedResult {
    vector<ResourceTemplate> ResourceTemplates;
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

// A notification from the server to the client, informing it that a resource has changed and may
// need to be read again. This should only be sent if the client previously sent a
// resources/subscribe request.
struct ResourceUpdatedNotification : public NotificationBase {
    struct ResourceUpdatedNotificationParams {
        URI URI; // The URI of the resource that has been updated. This might be a sub-resource of
                 // the one that the client actually subscribed to.
    };

    ResourceUpdatedNotificationParams Params;

    ResourceUpdatedNotification() : NotificationBase(MTHD_NOTIFICATIONS_RESOURCES_UPDATED) {}
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

// Sent from the client to request resources/updated notifications from the server whenever a
// particular resource changes.
struct SubscribeRequest : public RequestBase {
    struct SubscribeRequestParams {
        URI URI; // The URI of the resource to subscribe to. The URI can use any protocol; it is
                 // up to the server how to interpret it.
    };

    SubscribeRequestParams Params;

    SubscribeRequest() : RequestBase(MTHD_RESOURCES_SUBSCRIBE) {}
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

// Sent from the client to request cancellation of resources/updated notifications from the server.
// This should follow a previous resources/subscribe request.
struct UnsubscribeRequest : public RequestBase {
    struct UnsubscribeRequestParams {
        URI URI; // The URI of the resource to unsubscribe from.
    };

    UnsubscribeRequestParams Params;
    UnsubscribeRequest() : RequestBase(MTHD_RESOURCES_UNSUBSCRIBE) {}
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

// Sent from the client to the server, to read a specific resource URI.
struct ReadResourceRequest : public RequestBase {
    struct ReadResourceRequestParams {
        URI URI; // The URI of the resource to read. The URI can use any protocol; it is up to the
                 // server how to interpret it.
    };

    ReadResourceRequestParams Params;
    ReadResourceRequest() : RequestBase(MTHD_RESOURCES_READ) {}
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

// The server's response to a resources/read request from the client.
struct ReadResourceResult : public Result {
    vector<variant<TextResourceContents, BlobResourceContents>> Contents;
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

// An optional notification from the server to the client, informing it that the list of resources
// it can read from has changed. This may be issued by servers without any previous subscription
// from the client.
struct ResourceListChangedNotification : public NotificationBase {
    ResourceListChangedNotification()
        : NotificationBase(MTHD_NOTIFICATIONS_RESOURCES_LIST_CHANGED) {}
};

MCP_NAMESPACE_END