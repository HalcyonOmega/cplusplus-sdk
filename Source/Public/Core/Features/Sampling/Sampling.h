#pragma once

#include "Core/Types/Roles.h"
#include "RequestBase.h"
#include "ResponseBase.h"
#include "SamplingBase.h"
#include "Utilities/JSON/JSONLayer.hpp"

MCP_NAMESPACE_BEGIN

static constexpr const char* INCLUDE_CONTEXT_NONE = "none";
static constexpr const char* INCLUDE_CONTEXT_THIS_SERVER = "thisServer";
static constexpr const char* INCLUDE_CONTEXT_ALL_SERVERS = "allServers";

enum class IncludeContext { None, ThisServer, AllServers };

DEFINE_ENUM_JSON(IncludeContext, {{IncludeContext::None, INCLUDE_CONTEXT_NONE},
                                  {IncludeContext::ThisServer, INCLUDE_CONTEXT_THIS_SERVER},
                                  {IncludeContext::AllServers, INCLUDE_CONTEXT_ALL_SERVERS}});

// CreateMessageRequest {
//   MSG_DESCRIPTION
//       : "A request from the server to sample an LLM via the client. The client "
//         "has full discretion over which model to select. The client should "
//         "also "
//         "inform the user before beginning sampling, to allow them to inspect "
//         "the "
//         "request (human in the loop) and decide whether to approve it.",
//         MSG_PROPERTIES
//       : {
//         MSG_METHOD : {MSG_CONST : MTHD_SAMPLING_CREATE_MESSAGE, MSG_TYPE : MSG_STRING},
//         MSG_PARAMS : {
//           MSG_PROPERTIES : {
//             "includeContext" : {
//               MSG_DESCRIPTION :
//                   "A request to include context from one or more MCP "
//                   "servers (including the caller), to be attached to "
//                   "the prompt. The client MAY ignore this request.",
//               MSG_ENUM : [ "allServers", "none", "thisServer" ],
//               MSG_TYPE : MSG_STRING
//             },
//             "maxTokens" : {
//               MSG_DESCRIPTION :
//                   "The maximum number of tokens to sample, as "
//                   "requested by the server. The client MAY choose to "
//                   "sample fewer tokens than requested.",
//               MSG_TYPE : MSG_INTEGER
//             },
//             "messages" : {
//               MSG_ITEMS : {"$ref" : "#/definitions/SamplingMessage"},
//               MSG_TYPE : MSG_ARRAY
//             },
//             "metadata" : {
//               MSG_ADDITIONAL_PROPERTIES : true,
//               MSG_DESCRIPTION :
//                   "Optional metadata to pass through to the LLM provider. The "
//                   "format of this metadata is provider-specific.",
//               MSG_PROPERTIES : {},
//               MSG_TYPE : MSG_OBJECT
//             },
//             "modelPreferences" : {
//               "$ref" : "#/definitions/ModelPreferences",
//               MSG_DESCRIPTION :
//                   "The server's preferences for which model to select. "
//                   "The client MAY ignore these preferences."
//             },
//             "stopSequences" : {MSG_ITEMS : {MSG_TYPE : MSG_STRING}, MSG_TYPE : MSG_ARRAY},
//             "systemPrompt" : {
//               MSG_DESCRIPTION : "An optional system prompt the server wants to "
//                               "use for sampling. "
//                               "The client MAY modify or omit this prompt.",
//               MSG_TYPE : MSG_STRING
//             },
//             "temperature" : {MSG_TYPE : MSG_NUMBER}
//           },
//           MSG_REQUIRED : [ "maxTokens", "messages" ],
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED : [ MSG_METHOD, MSG_PARAMS ],
//                      MSG_TYPE : MSG_OBJECT
// };

// A request from the server to sample an LLM via the client. The client has full discretion over
// which model to select. The client should also inform the user before beginning sampling, to allow
// them to inspect the request (human in the loop) and decide whether to approve it.
struct CreateMessageRequest : public RequestBase {
    struct CreateMessageRequestParams {
        vector<SamplingMessage> Messages;
        optional<ModelPreferences>
            ModelPreferences; // The server's preferences for which model to select. The client MAY
                              // ignore these preferences.
        optional<string> SystemPrompt; // An optional system prompt the server wants to use for
                                       // sampling. The client MAY modify or omit this prompt.
        optional<IncludeContext>
            IncludeContext; // A request to include context from one or more
                            // MCP servers (including the caller), to be attached to the prompt. The
                            // client MAY ignore this request.
        optional<double> Temperature;
        int MaxTokens; // The maximum number of tokens to sample, as requested by the server. The
                       // client MAY choose to sample fewer tokens than
                       // requested.optional<vector<string>> StopSequences;
        optional<JSON> Metadata; // Optional metadata to pass through to the LLM provider. The
                                 // format of this metadata is provider-specific.
    };

    CreateMessageRequestParams Params;

    CreateMessageRequest() : RequestBase(MTHD_SAMPLING_CREATE_MESSAGE) {}
};

static constexpr const char* STOP_REASON_END_TURN = "endTurn";
static constexpr const char* STOP_REASON_STOP_SEQUENCE = "stopSequence";
static constexpr const char* STOP_REASON_MAX_TOKENS = "maxTokens";

enum class StopReason { EndTurn, StopSequence, MaxTokens };

DEFINE_ENUM_JSON(StopReason, {{StopReason::EndTurn, STOP_REASON_END_TURN},
                              {StopReason::StopSequence, STOP_REASON_STOP_SEQUENCE},
                              {StopReason::MaxTokens, STOP_REASON_MAX_TOKENS}});

// CreateMessageResult {
//   MSG_DESCRIPTION
//       : "The client's response to a sampling/create_message request from the "
//         "server. The client should inform the user before returning the "
//         "sampled message, to allow them to inspect the response (human in "
//         "the loop) and decide whether to allow the server to see it.",
//         MSG_PROPERTIES
//       : {
//         MSG_META : {
//           MSG_ADDITIONAL_PROPERTIES : {},
//           MSG_DESCRIPTION : "This result property is reserved by the protocol to "
//                           "allow clients and servers to attach additional "
//                           "metadata to their responses.",
//           MSG_TYPE : MSG_OBJECT
//         },
//         MSG_CONTENT : {
//           "anyOf" : [
//             {"$ref" : "#/definitions/TextContent"},
//             {"$ref" : "#/definitions/ImageContent"},
//             {"$ref" : "#/definitions/AudioContent"}
//           ]
//         },
//         MSG_MODEL : {
//           MSG_DESCRIPTION : "The name of the model that generated the message.",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_ROLE : {"$ref" : "#/definitions/Role"},
//         "stopReason" : {
//           MSG_DESCRIPTION : "The reason why sampling stopped, if known.",
//           MSG_TYPE : MSG_STRING
//         }
//       },
//         MSG_REQUIRED : [ MSG_CONTENT, MSG_MODEL, MSG_ROLE ],
//                      MSG_TYPE : MSG_OBJECT
// };

// The client's response to a sampling/create_message request from the server. The client should
// inform the user before returning the sampled message, to allow them to inspect the response
// (human in the loop) and decide whether to allow the server to see it.
// TODO: Typescript extended from Result and SamplingMessage - How to convert properly?
struct CreateMessageResult : public ResponseBase, SamplingMessage {
    string Model; // The name of the model that generated the message.
    optional<variant<StopReason, string>> StopReason; // The reason why sampling stopped, if known.
    enum Role Role;                                   // The role of the message.
    // TODO: @HalcyonOmega - Was z.discriminatedUnion(MSG_TYPE, [TextContent, ImageContent,
    // AudioContent]) Content;
    variant<TextContent, ImageContent, AudioContent> Content; // The content of the message.
};

MCP_NAMESPACE_END