#pragma once

#include "Core.h"
#include "Core/Constants/MessageConstants.h"
#include "Core/Messages/MessageBase.h"

MCP_NAMESPACE_BEGIN

// NotificationBase {
//   MSG_DESCRIPTION : "A notification which does not expect a response.",
//                   MSG_PROPERTIES
//       : {
//         MSG_JSON_RPC : {MSG_CONST : MSG_JSON_RPC_VERSION, MSG_TYPE : MSG_STRING},
//         MSG_METHOD : {MSG_TYPE : MSG_STRING},
//         MSG_PARAMS : {
//           MSG_ADDITIONAL_PROPERTIES : {},
//           MSG_PROPERTIES : {
//             MSG_META : {
//               MSG_ADDITIONAL_PROPERTIES : {},
//               MSG_DESCRIPTION : "This parameter name is reserved by MCP to allow "
//                               "clients and servers to attach additional "
//                               "metadata to their notifications.",
//               MSG_TYPE : MSG_OBJECT
//             }
//           },
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED : [ MSG_JSON_RPC, MSG_METHOD ],
//                      MSG_TYPE : MSG_OBJECT
// };

// A notification which does not expect a response. Supports JSON-RPC 2.0.
class NotificationBase : public MessageBase {
  private:
    string m_Method;
    optional<unique_ptr<MessageParams>> m_Params = nullopt;

  public:
    // Constructors
    NotificationBase(string Method, optional<unique_ptr<MessageParams>> Params = nullopt)
        : m_Method(std::move(Method)), m_Params(std::move(Params)) {}

    // Direct Getters
    [[nodiscard]] string_view GetMethod() const;
    [[nodiscard]] optional<const MessageParams*> GetParams() const;

    // MessageBase Overrides
    [[nodiscard]] JSON ToJSON() const override;
    [[nodiscard]] unique_ptr<MessageBase> FromJSON(const JSON& InJSON) override;
    [[nodiscard]] string Serialize() const override;
    [[nodiscard]] unique_ptr<MessageBase> Deserialize(string InString) override;
};

bool IsNotificationBase(const JSON& value) {
    return value.is_object() && value.value(MSG_JSON_RPC, MSG_EMPTY) == MSG_JSON_RPC_VERSION
           && value.contains(MSG_METHOD) && !value.contains(MSG_ID);
}

// TODO: @HalcyonOmega Cleanup below

struct NotificationParamsMeta {
    AdditionalProperties AdditionalProperties;
};

struct NotificationParams {
    optional<NotificationParamsMeta>
        Meta; // This parameter name is reserved by MCP to allow
              // clients and servers to attach additional metadata to their notifications.
    AdditionalProperties AdditionalProperties;
};

// Notification {
//   MSG_PROPERTIES : {
//     MSG_METHOD : {MSG_TYPE : MSG_STRING},
//     MSG_PARAMS : {
//       MSG_ADDITIONAL_PROPERTIES : {},
//       MSG_PROPERTIES : {
//         MSG_META : {
//           MSG_ADDITIONAL_PROPERTIES : {},
//           MSG_DESCRIPTION :
//               "This parameter name is reserved by MCP to allow clients and "
//               "servers to attach additional metadata to their
//               notifications.",
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//       MSG_TYPE : MSG_OBJECT
//     }
//   },
//                  MSG_REQUIRED : [MSG_METHOD],
//                               MSG_TYPE : MSG_OBJECT
// };

struct Notification {
    string Method;
    optional<NotificationParams> Params;
};

MCP_NAMESPACE_END