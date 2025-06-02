#pragma once

#include "CommonSchemas.h"
#include "Constants.h"
#include "Core.h"
MCP_NAMESPACE_BEGIN

// struct CompleteRequest {
//   "description" : "A request from the client to the server, to ask for "
//                   "completion options.",
//                   "properties"
//       : {
//         "method" : {"const" : "completion/complete", "type" : "string"},
//         "params" : {
//           "properties" : {
//             "argument" : {
//               "description" : "The argument's information",
//               "properties" : {
//                 "name" : {
//                   "description" : "The name of the argument",
//                   "type" : "string"
//                 },
//                 "value" : {
//                   "description" : "The value of the argument to use for "
//                                   "completion matching.",
//                   "type" : "string"
//                 }
//               },
//               "required" : [ "name", "value" ],
//               "type" : "object"
//             },
//             "ref" : {
//               "anyOf" : [
//                 {"$ref" : "#/definitions/PromptReference"},
//                 {"$ref" : "#/definitions/ResourceReference"}
//               ]
//             }
//           },
//           "required" : [ "argument", "ref" ],
//           "type" : "object"
//         }
//       },
//         "required" : [ "method", "params" ],
//                      "type" : "object"
// };

// struct CompleteResult {
//   "description" : "The server's response to a completion/complete request",
//                   "properties"
//       : {
//         "_meta" : {
//           "additionalProperties" : {},
//           "description" : "This result property is reserved by the protocol to "
//                           "allow clients and servers to attach additional "
//                           "metadata to their responses.",
//           "type" : "object"
//         },
//         "completion" : {
//           "properties" : {
//             "hasMore" : {
//               "description" :
//                   "Indicates whether there are additional completion options "
//                   "beyond those provided in the current response, even if the "
//                   "exact total is unknown.",
//               "type" : "boolean"
//             },
//             "total" : {
//               "description" :
//                   "The total number of completion options available. This can "
//                   "exceed the number of values actually sent in the response.",
//               "type" : "integer"
//             },
//             "values" : {
//               "description" :
//                   "An array of completion values. Must not exceed 100 items.",
//               "items" : {"type" : "string"},
//               "type" : "array"
//             }
//           },
//           "required" : ["values"],
//           "type" : "object"
//         }
//       },
//         "required" : ["completion"],
//                      "type" : "object"
// };

/* Autocomplete */
/**
 * A request from the client to the server, to ask for completion options.
 */
struct CompleteRequest : public Request {
    params : {
        variant<PromptReference, ResourceReference> ref;
    /**
     * The argument's information
     */
    argument: {
        /**
         * The name of the argument
         */
        string name;
        /**
         * The value of the argument to use for completion matching.
         */
        string value;
    };
    };

    CompleteRequest() {
        method = MTHD_COMPLETION_COMPLETE;
    }
};

/**
 * The server's response to a completion/complete request
 */
struct CompleteResult : public Result {
    completion : {
    /**
     * An array of completion values. Must not exceed 100 items.
     */
    values:
        string[];
        /**
         * The total number of completion options available. This can exceed the number of values
         * actually sent in the response.
         */
        total ?: number;
        /**
         * Indicates whether there are additional completion options beyond those provided in the
         * current response, even if the exact total is unknown.
         */
        optional<bool> hasMore;
    };
};

MCP_NAMESPACE_END