#pragma once

#include "Core.h"
#include "Core/Messages/MessageBase.h"
#include "Utilities/JSON/JSONLayer.hpp"

MCP_NAMESPACE_BEGIN

// Forward Declarations
// TODO: @HalcyonOmega Fix External Ref: ProgressToken
struct ProgressToken;

// RequestID {
//   MSG_DESCRIPTION : "A uniquely identifying ID for a request in JSON-RPC.",
//                   MSG_TYPE : [ MSG_STRING, MSG_INTEGER ]
// };

// A uniquely identifying ID for a request in JSON-RPC.
struct RequestID {
  private:
    // TODO: Is LongLong the right type or should it be double?
    variant<string, int, long long> m_RequestID;

  public:
    // Constructors
    RequestID(string StringID) : m_RequestID(std::move(StringID)) {}
    RequestID(int IntID) : m_RequestID(IntID) {}
    RequestID(long long LongLongID) : m_RequestID(LongLongID) {}

    // Direct Getters
    [[nodiscard]] string_view ToString() const;
};

// TODO: @HalcyonOmega RequestParams not implemented in RequestBase
struct RequestParams {
    struct RequestParamsMeta {
        optional<ProgressToken>
            ProgressToken; // If specified, the caller is requesting out-of-band
                           // progress notifications for this request (as represented by
                           // notifications/progress). The value of this parameter is an opaque
                           // token that will be attached to any subsequent notifications. The
                           // receiver is not obligated to provide these notifications.
    };
    optional<RequestParamsMeta> Meta;
    DEFINE_TYPE_JSON(RequestParams, JKEY(Meta, MSG_META))
};

// RequestBase {
//   MSG_DESCRIPTION : "A request that expects a response.",
//   MSG_PROPERTIES  : {
//         MSG_ID : {"$ref" : "#/definitions/RequestID"},
//         MSG_JSON_RPC : {MSG_CONST : MSG_JSON_RPC_VERSION, MSG_TYPE : MSG_STRING},
//         MSG_METHOD : {MSG_TYPE : MSG_STRING},
//         MSG_PARAMS : {
//           MSG_ADDITIONAL_PROPERTIES : {},
//           MSG_PROPERTIES : {
//             MSG_META : {
//               MSG_PROPERTIES : {
//                 MSG_PROGRESS_TOKEN : {
//                   "$ref" : "#/definitions/ProgressToken",
//                   MSG_DESCRIPTION :
//                       "If specified, the caller is requesting out-of-band "
//                       "progress notifications for this request (as represented "
//                       "by notifications/progress). The value of this parameter "
//                       "is an opaque token that will be attached to any "
//                       "subsequent notifications. The receiver is not obligated "
//                       "to provide these notifications."
//                 }
//               },
//               MSG_TYPE : MSG_OBJECT
//             }
//           },
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED : [ MSG_ID, MSG_JSON_RPC, MSG_METHOD ],
//                      MSG_TYPE : MSG_OBJECT
// };

// A request that expects a response. Supports JSON-RPC 2.0.
class RequestBase : public MessageBase {
  private:
    RequestID m_ID;
    string m_Method;
    optional<unique_ptr<MessageParams>> m_Params = nullopt;

  public:
    // Constructors
    RequestBase(string Method, optional<unique_ptr<MessageParams>> Params = nullopt)
        : m_ID(0), m_Method(std::move(Method)), m_Params(std::move(Params)) {}

    // Direct Getters
    [[nodiscard]] RequestID GetRequestID() const;
    [[nodiscard]] string_view GetMethod() const;
    [[nodiscard]] optional<const MessageParams*> GetParams() const;

    // MessageBase Overrides
    [[nodiscard]] JSON ToJSON() const override;
    [[nodiscard]] unique_ptr<MessageBase> FromJSON(const JSON& InJSON) override;
    [[nodiscard]] string Serialize() const override;
    [[nodiscard]] unique_ptr<MessageBase> Deserialize(string InString) override;
};

bool IsRequestBase(const JSON& value) {
    return value.is_object() && value.value(MSG_JSON_RPC, MSG_EMPTY) == MSG_JSON_RPC_VERSION
           && value.contains(MSG_ID) && value.contains(MSG_METHOD) && !value.contains(MSG_ERROR)
           && !value.contains(MSG_RESULT);
}

// PaginatedRequest {
//   MSG_PROPERTIES : {
//     MSG_METHOD : {MSG_TYPE : MSG_STRING},
//     MSG_PARAMS : {
//       MSG_PROPERTIES : {
//         MSG_CURSOR : {
//           MSG_DESCRIPTION :
//               "An opaque token representing the current pagination position. If provided, the
//               server should return results starting after this cursor.",
//           MSG_TYPE : MSG_STRING
//         }
//       },
//       MSG_TYPE : MSG_OBJECT
//     }
//   },
//                  MSG_REQUIRED : [MSG_METHOD],
//                               MSG_TYPE : MSG_OBJECT
// };

struct PaginatedRequest : public RequestBase {
    // Parameters for Paginated Requests
    struct PaginatedRequestParams : public RequestParams {
        optional<Cursor>
            Cursor; // An opaque token representing the current pagination position. If
                    // provided, the server should return results starting after this cursor.
    };
    optional<PaginatedRequestParams> Params;
};

MCP_NAMESPACE_END