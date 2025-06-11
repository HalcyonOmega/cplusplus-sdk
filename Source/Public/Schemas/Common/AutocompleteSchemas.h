#pragma once

#include "CommonSchemas.h"
#include "Constants.h"
#include "Core.h"
#include "RequestSchemas.h"
#include "ResultSchemas.h"

MCP_NAMESPACE_BEGIN

struct AutocompleteReference {
    string type;
};

// ResourceReference {
//   MSG_DESCRIPTION : "A reference to a resource or resource template definition.",
//                   MSG_PROPERTIES
//       : {
//         MSG_TYPE : {MSG_CONST : "ref/resource", MSG_TYPE : MSG_STRING},
//         MSG_URI : {
//           MSG_DESCRIPTION : "The URI or URI template of the resource.",
//           MSG_FORMAT : "uri-template",
//           MSG_TYPE : MSG_STRING
//         }
//       },
//         MSG_REQUIRED : [ MSG_TYPE, MSG_URI ],
//                      MSG_TYPE : MSG_OBJECT
// };

/**
 * A reference to a resource or resource template definition.
 */
struct ResourceReference : public AutocompleteReference {
    /**
     * The URI or URI template of the resource.
     *
     * @format uri-template
     */
    string uri;

    ResourceReference() {
        type = MSG_REF_RESOURCE;
    }
};

// PromptReference {
//   MSG_DESCRIPTION : "Identifies a prompt.",
//                   MSG_PROPERTIES
//       : {
//         MSG_NAME : {
//           MSG_DESCRIPTION : "The name of the prompt or prompt template",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_TYPE : {MSG_CONST : "ref/prompt", MSG_TYPE : MSG_STRING}
//       },
//         MSG_REQUIRED : [ MSG_NAME, MSG_TYPE ],
//                      MSG_TYPE : MSG_OBJECT
// };

/**
 * Identifies a prompt.
 */
struct PromptReference : public AutocompleteReference {
    /**
     * The name of the prompt or prompt template
     */
    string name;

    PromptReference() {
        type = MSG_REF_PROMPT;
    }
};

struct CompleteRequestParamsArgument {
    /**
     * The name of the argument
     */
    string name;
    /**
     * The value of the argument to use for completion matching.
     */
    string value;
};

struct CompleteRequestParams {
    variant<PromptReference, ResourceReference> ref;
    /**
     * The argument's information
     */
    CompleteRequestParamsArgument argument;
};

// CompleteRequest {
//   MSG_DESCRIPTION : "A request from the client to the server, to ask for "
//                   "completion options.",
//                   MSG_PROPERTIES
//       : {
//         MSG_METHOD : {MSG_CONST : "completion/complete", MSG_TYPE : MSG_STRING},
//         MSG_PARAMS : {
//           MSG_PROPERTIES : {
//             "argument" : {
//               MSG_DESCRIPTION : "The argument's information",
//               MSG_PROPERTIES : {
//                 MSG_NAME : {
//                   MSG_DESCRIPTION : "The name of the argument",
//                   MSG_TYPE : MSG_STRING
//                 },
//                 "value" : {
//                   MSG_DESCRIPTION : "The value of the argument to use for "
//                                   "completion matching.",
//                   MSG_TYPE : MSG_STRING
//                 }
//               },
//               MSG_REQUIRED : [ MSG_NAME, "value" ],
//               MSG_TYPE : MSG_OBJECT
//             },
//             "ref" : {
//               "anyOf" : [
//                 {"$ref" : "#/definitions/PromptReference"},
//                 {"$ref" : "#/definitions/ResourceReference"}
//               ]
//             }
//           },
//           MSG_REQUIRED : [ "argument", "ref" ],
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED : [ MSG_METHOD, MSG_PARAMS ],
//                      MSG_TYPE : MSG_OBJECT
// };

/**
 * A request from the client to the server, to ask for completion options.
 */
struct CompleteRequest : public Request {
    CompleteRequestParams params;

    CompleteRequest() {
        method = MTHD_COMPLETION_COMPLETE;
    }
};

struct CompleteResultParams {
    /**
     * An array of completion values. Must not exceed 100 items.
     */
    vector<string> values;
    /**
     * The total number of completion options available. This can exceed the number of values
     * actually sent in the response.
     */
    optional<number> total;
    /**
     * Indicates whether there are additional completion options beyond those provided in the
     * current response, even if the exact total is unknown.
     */
    optional<bool> hasMore;
};

// CompleteResult {
//   MSG_DESCRIPTION : "The server's response to a completion/complete request",
//                   MSG_PROPERTIES
//       : {
//         MSG_META : {
//           MSG_ADDITIONAL_PROPERTIES : {},
//           MSG_DESCRIPTION : "This result property is reserved by the protocol to "
//                           "allow clients and servers to attach additional "
//                           "metadata to their responses.",
//           MSG_TYPE : MSG_OBJECT
//         },
//         "completion" : {
//           MSG_PROPERTIES : {
//             "hasMore" : {
//               MSG_DESCRIPTION :
//                   "Indicates whether there are additional completion options "
//                   "beyond those provided in the current response, even if the "
//                   "exact total is unknown.",
//               MSG_TYPE : "boolean"
//             },
//             "total" : {
//               MSG_DESCRIPTION :
//                   "The total number of completion options available. This can "
//                   "exceed the number of values actually sent in the response.",
//               MSG_TYPE : "integer"
//             },
//             "values" : {
//               MSG_DESCRIPTION :
//                   "An array of completion values. Must not exceed 100 items.",
//               MSG_ITEMS : {MSG_TYPE : MSG_STRING},
//               MSG_TYPE : MSG_ARRAY
//             }
//           },
//           MSG_REQUIRED : ["values"],
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED : ["completion"],
//                      MSG_TYPE : MSG_OBJECT
// };

/**
 * The server's response to a completion/complete request
 */
struct CompleteResult : public Result {
    CompleteResultParams completion;
};

MCP_NAMESPACE_END