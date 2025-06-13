#pragma once

#include "Core.h"
#include "Core/Constants/MessageConstants.h"
#include "Core/Constants/MethodConstants.h"
#include "RequestSchemas.h"
#include "ResultSchemas.h"

MCP_NAMESPACE_BEGIN

struct AutocompleteReference {
    string Type;
};

// ResourceReference {
//   MSG_DESCRIPTION : "A reference to a resource or resource template definition.",
//                   MSG_PROPERTIES
//       : {
//         MSG_TYPE : {MSG_CONST : MSG_REF_RESOURCE, MSG_TYPE : MSG_STRING},
//         MSG_URI : {
//           MSG_DESCRIPTION : "The URI or URI template of the resource.",
//           MSG_FORMAT : MSG_URITEMPLATE,
//           MSG_TYPE : MSG_STRING
//         }
//       },
//         MSG_REQUIRED : [ MSG_TYPE, MSG_URI ],
//                      MSG_TYPE : MSG_OBJECT
// };

// A reference to a resource or resource template definition.
struct ResourceReference : public AutocompleteReference {
    // The URI or URI template of the resource.
    // @format uri-template
    string URI;

    ResourceReference() {
        Type = MSG_REF_RESOURCE;
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
//         MSG_TYPE : {MSG_CONST : MSG_REF_PROMPT, MSG_TYPE : MSG_STRING}
//       },
//         MSG_REQUIRED : [ MSG_NAME, MSG_TYPE ],
//                      MSG_TYPE : MSG_OBJECT
// };

// Identifies a prompt.
struct PromptReference : public AutocompleteReference {
    string Name; // The name of the prompt or prompt template

    PromptReference() {
        Type = MSG_REF_PROMPT;
    }
};

struct CompleteRequestParamsArgument {
    string Name;  // The name of the argument
    string Value; // The value of the argument to use for completion matching
};

struct CompleteRequestParams {
    variant<PromptReference, ResourceReference> Ref; // The reference to the argument
    CompleteRequestParamsArgument Argument;          // The argument's information
};

// CompleteRequest {
//   MSG_DESCRIPTION : "A request from the client to the server, to ask for "
//                   "completion options.",
//                   MSG_PROPERTIES
//       : {
//         MSG_METHOD : {MSG_CONST : MTHD_COMPLETION_COMPLETE, MSG_TYPE : MSG_STRING},
//         MSG_PARAMS : {
//           MSG_PROPERTIES : {
//             MSG_ARGUMENT : {
//               MSG_DESCRIPTION : "The argument's information",
//               MSG_PROPERTIES : {
//                 MSG_NAME : {
//                   MSG_DESCRIPTION : "The name of the argument",
//                   MSG_TYPE : MSG_STRING
//                 },
//                 MSG_VALUE : {
//                   MSG_DESCRIPTION : "The value of the argument to use for "
//                                   "completion matching.",
//                   MSG_TYPE : MSG_STRING
//                 }
//               },
//               MSG_REQUIRED : [ MSG_NAME, MSG_VALUE ],
//               MSG_TYPE : MSG_OBJECT
//             },
//             MSG_REF : {
//               "anyOf" : [
//                 {"$ref" : "#/definitions/PromptReference"},
//                 {"$ref" : "#/definitions/ResourceReference"}
//               ]
//             }
//           },
//           MSG_REQUIRED : [ MSG_ARGUMENT, MSG_REF ],
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED : [ MSG_METHOD, MSG_PARAMS ],
//                      MSG_TYPE : MSG_OBJECT
// };

// A request from the client to the server, to ask for completion options.
struct CompleteRequest : public RequestBase {
    CompleteRequestParams Params;

    CompleteRequest() : RequestBase(MTHD_COMPLETION_COMPLETE) {}
};

struct CompleteResultParams {
    vector<string> Values;  // An array of completion values. Must not exceed 100 items.
    optional<number> Total; // The total number of completion options available. This can exceed the
                            // number of values actually sent in the response.
    optional<bool>
        HasMore; // Indicates whether there are additional completion options beyond
                 // those provided in the current response, even if the exact total is unknown.
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
//         MSG_COMPLETION : {
//           MSG_PROPERTIES : {
//             MSG_HAS_MORE : {
//               MSG_DESCRIPTION :
//                   "Indicates whether there are additional completion options "
//                   "beyond those provided in the current response, even if the "
//                   "exact total is unknown.",
//               MSG_TYPE : MSG_BOOLEAN
//             },
//             MSG_TOTAL : {
//               MSG_DESCRIPTION :
//                   "The total number of completion options available. This can "
//                   "exceed the number of values actually sent in the response.",
//               MSG_TYPE : MSG_INTEGER
//             },
//             MSG_VALUES : {
//               MSG_DESCRIPTION :
//                   "An array of completion values. Must not exceed 100 items.",
//               MSG_ITEMS : {MSG_TYPE : MSG_STRING},
//               MSG_TYPE : MSG_ARRAY
//             }
//           },
//           MSG_REQUIRED : [MSG_VALUES],
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED : [MSG_COMPLETION],
//                      MSG_TYPE : MSG_OBJECT
// };

// The server's response to a completion/complete request
struct CompleteResult : public ResultMessage {
    CompleteResultParams Completion;
};

MCP_NAMESPACE_END