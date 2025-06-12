#pragma once

#include "Core.h"
#include "Core/Messages/Messages.h"

MCP_NAMESPACE_BEGIN

// RequestMessage {
//   MSG_DESCRIPTION : "A request that expects a response.",
//                   MSG_PROPERTIES
//       : {
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
class RequestMessage : public MessageBase {
  private:
    MessageID m_ID;
    string m_Method;
    optional<unique_ptr<MessageParams>> m_Params = nullopt;

  public:
    // Constructors
    RequestMessage(string Method, optional<unique_ptr<MessageParams>> Params = nullopt)
        : m_ID(0), m_Method(std::move(Method)), m_Params(std::move(Params)) {}

    // Direct Getters
    [[nodiscard]] MessageID GetMessageID() const;
    [[nodiscard]] string_view GetMethod() const;
    [[nodiscard]] optional<const MessageParams*> GetParams() const;

    // MessageBase Overrides
    [[nodiscard]] JSON ToJSON() const override;
    [[nodiscard]] unique_ptr<MessageBase> FromJSON(const JSON& InJSON) override;
    [[nodiscard]] string Serialize() const override;
    [[nodiscard]] unique_ptr<MessageBase> Deserialize(string InString) override;
};

bool IsRequestMessage(const JSON& value) {
    return value.is_object() && value.value(MSG_JSON_RPC, MSG_NULL) == MSG_JSON_RPC_VERSION
           && value.contains(MSG_ID) && value.contains(MSG_METHOD) && !value.contains(MSG_ERROR)
           && !value.contains(MSG_RESULT);
}

MCP_NAMESPACE_END