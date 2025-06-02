#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

struct Result {
    /**
     * This result property is reserved by the protocol to allow clients and
     * servers to attach additional metadata to their responses.
     */
    optional<JSON> _meta;
    JSON additionalProperties;
};

/* Pagination */
struct PaginatedResult : public Result {
    /**
     * An opaque token representing the pagination position after the last
     * returned result. If present, there may be more results available.
     */
    optional<Cursor> nextCursor;
};

/* Empty result */
/**
 * A response that indicates success but carries no data.
 */
using EmptyResult = Result;

// struct PaginatedResult {
//   "properties" : {
//     "_meta" : {
//       "additionalProperties" : {},
//       "description" : "This result property is reserved by the protocol "
//                       "to allow clients and servers to attach additional "
//                       "metadata to their responses.",
//       "type" : "object"
//     },
//     "nextCursor" : {
//       "description" : "An opaque token representing the pagination "
//                       "position after the last returned result.\nIf "
//                       "present, there may be more results available.",
//       "type" : "string"
//     }
//   },
//                  "type" : "object"
// };

// struct Result {
//   "additionalProperties" : {},
//                            "properties"
//       : {
//         "_meta" : {
//           "additionalProperties" : {},
//           "description" : "This result property is reserved by the protocol
//           to "
//                           "allow clients and servers to attach additional "
//                           "metadata to their responses.",
//           "type" : "object"
//         }
//       },
//         "type" : "object"
// };

// struct EmptyResult {
//   "$ref" : "#/definitions/Result"
// };

MCP_NAMESPACE_END
