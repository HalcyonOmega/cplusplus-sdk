#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

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

struct Result {
    /**
     * This result property is reserved by the protocol to allow clients and
     * servers to attach additional metadata to their responses.
     */
    optional<JSON> _meta;
    JSON additionalProperties;
};

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

struct PaginatedResult : public Result {
    /**
     * An opaque token representing the pagination position after the last
     * returned result. If present, there may be more results available.
     */
    optional<Cursor> nextCursor;
};

// EmptyResult {
//   "$ref" : "#/definitions/Result"
// };

using EmptyResult = Result; // A response that indicates success but carries no data.

MCP_NAMESPACE_END
