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

// PaginatedResult {
//   MSG_PROPERTIES : {
//     MSG_META : {
//       MSG_ADDITIONAL_PROPERTIES : {},
//       MSG_DESCRIPTION : "This result property is reserved by the protocol "
//                       "to allow clients and servers to attach additional "
//                       "metadata to their responses.",
//       MSG_TYPE : MSG_OBJECT
//     },
//     MSG_NEXT_CURSOR : {
//       MSG_DESCRIPTION : "An opaque token representing the pagination "
//                       "position after the last returned result.\nIf "
//                       "present, there may be more results available.",
//       MSG_TYPE : MSG_STRING
//     }
//   },
//                  MSG_TYPE : MSG_OBJECT
// };

// Result {
//   MSG_ADDITIONAL_PROPERTIES : {},
//                            MSG_PROPERTIES
//       : {
//         MSG_META : {
//           MSG_ADDITIONAL_PROPERTIES : {},
//           MSG_DESCRIPTION : "This result property is reserved by the protocol
//           to "
//                           "allow clients and servers to attach additional "
//                           "metadata to their responses.",
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//         MSG_TYPE : MSG_OBJECT
// };

// EmptyResult {
//   "$ref" : "#/definitions/Result"
// };

MCP_NAMESPACE_END
