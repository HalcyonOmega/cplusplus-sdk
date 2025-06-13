#pragma once

#include "Core.h"
#include "Core/Messages/MessageBase.h"
#include "Core/Messages/Requests/RequestBase.h"

MCP_NAMESPACE_BEGIN

// ResponseBase {
//   MSG_DESCRIPTION : "A successful (non-error) response to a request.",
//                   MSG_PROPERTIES : {
//                     MSG_ID : {"$ref" : "#/definitions/RequestID"},
//                     MSG_JSON_RPC : {MSG_CONST : MSG_JSON_RPC_VERSION, MSG_TYPE : MSG_STRING},
//                     MSG_RESULT : {"$ref" : "#/definitions/Result"}
//                   },
//                                  MSG_REQUIRED : [ MSG_ID, MSG_JSON_RPC, MSG_RESULT ],
//                                               MSG_TYPE : MSG_OBJECT
// };

// A successful (non-error) response to a request. Supports JSON-RPC 2.0.
class ResponseBase : public MessageBase {
  private:
    RequestID m_ID;
    unique_ptr<MessageParams> m_Result;

  public:
    // Constructors
    ResponseBase(RequestID RequestID, unique_ptr<MessageParams> Result)
        : m_ID(std::move(RequestID)), m_Result(std::move(Result)) {}

    // Direct Getters
    [[nodiscard]] RequestID GetRequestID() const;
    [[nodiscard]] const MessageParams* GetResult() const;

    // MessageBase Overrides
    [[nodiscard]] JSON ToJSON() const override;
    [[nodiscard]] unique_ptr<MessageBase> FromJSON(const JSON& InJSON) override;
    [[nodiscard]] string Serialize() const override;
    [[nodiscard]] unique_ptr<MessageBase> Deserialize(string InString) override;
};

bool IsResponseBase(const JSON& value) {
    return value.is_object() && value.value(MSG_JSON_RPC, MSG_EMPTY) == MSG_JSON_RPC_VERSION
           && value.contains(MSG_ID) && value.contains(MSG_RESULT) && !value.contains(MSG_ERROR);
}

// Result {
//   MSG_ADDITIONAL_PROPERTIES : {},
//                            MSG_PROPERTIES
//       : {
//         MSG_META : {
//           MSG_ADDITIONAL_PROPERTIES : {},
//           MSG_DESCRIPTION : "This result property is reserved by the protocol to allow clients
//           and servers to attach additional metadata to their responses.", MSG_TYPE : MSG_OBJECT
//         }
//       },
//         MSG_TYPE : MSG_OBJECT
// };

struct Result {
    optional<JSON> Meta; // This result property is reserved by the protocol to allow clients and
                         // servers to attach additional metadata to their responses.
    AdditionalProperties AdditionalProperties;
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

struct PaginatedResult : public ResponseBase {
    optional<Cursor>
        NextCursor; // An opaque token representing the pagination position after the
                    // last returned result. If present, there may be more results available.
};

// EmptyResult {
//   "$ref" : "#/definitions/Result"
// };

using EmptyResult = ResponseBase; // A response that indicates success but carries no data.

MCP_NAMESPACE_END