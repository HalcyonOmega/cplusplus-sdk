#pragma once

#include "Core.h"
#include "Core/Types/Content.h"
#include "Core/Types/Roles.h"

MCP_NAMESPACE_BEGIN

// SamplingMessage {
//   MSG_DESCRIPTION : "Describes a message issued to or received from an LLM API.",
//                   MSG_PROPERTIES : {
//                     MSG_CONTENT : {
//                       "anyOf" : [
//                         {"$ref" : "#/definitions/TextContent"},
//                         {"$ref" : "#/definitions/ImageContent"},
//                         {"$ref" : "#/definitions/AudioContent"}
//                       ]
//                     },
//                     MSG_ROLE : {"$ref" : "#/definitions/Role"}
//                   },
//                                  MSG_REQUIRED : [ MSG_CONTENT, MSG_ROLE ],
//                                               MSG_TYPE : MSG_OBJECT
// };

// Describes a message issued to or received from an LLM API.
struct SamplingMessage {
    Role Role;
    variant<TextContent, ImageContent, AudioContent> Content; // The content of the message.
};

// ModelHint {
//   MSG_DESCRIPTION
//       : "Hints to use for model selection.\n\nKeys not declared here are "
//         "currently left "
//         "unspecified by the spec and are up\nto the client to interpret.",
//         MSG_PROPERTIES : {
//           MSG_NAME : {
//             MSG_DESCRIPTION :
//                 "A hint for a model name.\n\nThe client SHOULD treat this as a "
//                 "substring of a model name; for example:\n - "
//                 "`claude-3-5-sonnet` should "
//                 "match `claude-3-5-sonnet-20241022`\n - `sonnet` should match "
//                 "`claude-3-5-sonnet-20241022`, `claude-3-sonnet-20240229`, "
//                 "etc.\n - "
//                 "`claude` should match any Claude model\n\nThe client MAY also "
//                 "map the "
//                 "string to a different provider's model name or a different "
//                 "model "
//                 "family, as long as it fills a similar niche; for example:\n - "
//                 "`gemini-1.5-flash` could match `claude-3-haiku-20240307`",
//             MSG_TYPE : MSG_STRING
//           }
//         },
//                        MSG_TYPE : MSG_OBJECT
// };

/**
 * Hints to use for model selection.
 *
 * Keys not declared here are currently left unspecified by the spec and are up
 * to the client to interpret.
 */
struct ModelHint {
    /**
     * A hint for a model name.
     *
     * The client SHOULD treat this as a substring of a model name; for example:
     *  - `claude-3-5-sonnet` should match `claude-3-5-sonnet-20241022`
     *  - `sonnet` should match `claude-3-5-sonnet-20241022`, `claude-3-sonnet-20240229`, etc.
     *  - `claude` should match any Claude model
     *
     * The client MAY also map the string to a different provider's model name or a different model
     * family, as long as it fills a similar niche; for example:
     *  - `gemini-1.5-flash` could match `claude-3-haiku-20240307`
     */
    optional<string> Name;
};

// ModelPreferences {
//   MSG_DESCRIPTION : "The server's preferences for model selection, requested of "
//                   "the client during "
//                   "sampling.\n\nBecause LLMs can vary along multiple "
//                   "dimensions, choosing the \"best\" "
//                   "model is\nrarely straightforward.  Different models excel "
//                   "in different areas—some "
//                   "are\nfaster but less capable, others are more capable but "
//                   "more expensive, and so\non. "
//                   "This interface allows servers to express their priorities "
//                   "across multiple\ndimensions "
//                   "to help clients make an appropriate selection for their use "
//                   "case.\n\nThese "
//                   "preferences are always advisory. The client MAY ignore "
//                   "them. It is also\nup to the "
//                   "client to decide how to interpret these preferences and how "
//                   "to\nbalance them against "
//                   "other considerations.",
//                   MSG_PROPERTIES
//       : {
//         "costPriority" : {
//           MSG_DESCRIPTION : "How much to prioritize cost when selecting a model. "
//                           "A value of 0 "
//                           "means cost\nis not important, while a value of 1 "
//                           "means cost is the "
//                           "most important\nfactor.",
//           MSG_MAXIMUM : 1,
//           MSG_MINIMUM : 0,
//           MSG_TYPE : MSG_NUMBER
//         },
//         "hints" : {
//           MSG_DESCRIPTION :
//               "Optional hints to use for model selection.\n\nIf multiple hints "
//               "are specified, the client MUST evaluate them in order\n(such "
//               "that "
//               "the first match is taken).\n\nThe client SHOULD prioritize "
//               "these "
//               "hints over the numeric priorities, but\nMAY still use the "
//               "priorities to select from ambiguous matches.",
//           MSG_ITEMS : {"$ref" : "#/definitions/ModelHint"},
//           MSG_TYPE : MSG_ARRAY
//         },
//         "intelligencePriority" : {
//           MSG_DESCRIPTION : "How much to prioritize intelligence and "
//                           "capabilities when selecting a\nmodel. "
//                           "A value of 0 means intelligence is not important, "
//                           "while a value of 1\nmeans "
//                           "intelligence is the most important factor.",
//           MSG_MAXIMUM : 1,
//           MSG_MINIMUM : 0,
//           MSG_TYPE : MSG_NUMBER
//         },
//         "speedPriority" : {
//           MSG_DESCRIPTION : "How much to prioritize sampling speed (latency) "
//                           "when selecting a "
//                           "model. A\nvalue of 0 means speed is not important, "
//                           "while a value "
//                           "of 1 means speed is\nthe most important factor.",
//           MSG_MAXIMUM : 1,
//           MSG_MINIMUM : 0,
//           MSG_TYPE : MSG_NUMBER
//         }
//       },
//         MSG_TYPE : MSG_OBJECT
// };

/**
 * The server's preferences for model selection, requested of the client during sampling.
 *
 * Because LLMs can vary along multiple dimensions, choosing the "best" model is
 * rarely straightforward.  Different models excel in different areas—some are
 * faster but less capable, others are more capable but more expensive, and so
 * on. This interface allows servers to express their priorities across multiple
 * dimensions to help clients make an appropriate selection for their use case.
 *
 * These preferences are always advisory. The client MAY ignore them. It is also
 * up to the client to decide how to interpret these preferences and how to
 * balance them against other considerations.
 */
struct ModelPreferences {
    optional<vector<ModelHint>>
        Hints; // Optional hints to use for model selection. If multiple hints are specified, the
               // client MUST evaluate them in order (such that the first match is taken). The
               // client SHOULD prioritize these hints over the numeric priorities, but MAY still
               // use the priorities to select from ambiguous matches.
    // TODO: @HalcyonOmega Enforce min = 0, max = 1
    optional<double> CostPriority; // How much to prioritize cost when selecting a model. A
                                   // value of 0 means cost is not important, while a value of 1
                                   // means cost is the most important factor.
    // TODO: @HalcyonOmega Enforce min = 0, max = 1
    optional<double> SpeedPriority; // How much to prioritize sampling speed (latency) when
                                    // selecting a model. A value of 0 means speed is not important,
                                    // while a value of 1 means speed is the most important factor.
    // TODO: @HalcyonOmega Enforce min = 0, max = 1
    optional<double> IntelligencePriority; // How much to prioritize intelligence and capabilities
                                           // when selecting a model. A value of 0 means
                                           // intelligence is not important, while a value of 1
                                           // means intelligence is the most important factor.
};

enum class IncludeContext { none, thisServer, allServers };

struct CreateMessageRequestParams {
    vector<SamplingMessage> Messages;
    /**
     * The server's preferences for which model to select. The client MAY ignore these
     * preferences.
     */
    optional<ModelPreferences> ModelPreferences;
    /**
     * An optional system prompt the server wants to use for sampling. The client MAY modify or
     * omit this prompt.
     */
    optional<string> SystemPrompt;
    /**
     * A request to include context from one or more MCP servers (including the caller), to be
     * attached to the prompt. The client MAY ignore this request.
     */
    optional<IncludeContext> IncludeContext;
    /**
     * @TJS-type number
     */
    optional<number> Temperature;
    /**
     * The maximum number of tokens to sample, as requested by the server. The client MAY choose
     * to sample fewer tokens than requested.
     */
    number MaxTokens;
    optional<vector<string>> StopSequences;
    /**
     * Optional metadata to pass through to the LLM provider. The format of this metadata is
     * provider-specific.
     */
    optional<JSON> Metadata;
};

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

/**
 * A request from the server to sample an LLM via the client. The client has full discretion over
 * which model to select. The client should also inform the user before beginning sampling, to allow
 * them to inspect the request (human in the loop) and decide whether to approve it.
 */
struct CreateMessageRequest : public RequestMessage {
    CreateMessageRequestParams Params;

    CreateMessageRequest() {
        method = MTHD_SAMPLING_CREATE_MESSAGE;
    }
};

enum class StopReason { endTurn, stopSequence, maxTokens };

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

// TODO: Typescript extended from Result and SamplingMessage - How to convert properly?
/**
 * The client's response to a sampling/create_message request from the server. The client should
 * inform the user before returning the sampled message, to allow them to inspect the response
 * (human in the loop) and decide whether to allow the server to see it.
 */
struct CreateMessageResult : public ResultMessage, SamplingMessage {
    string Model; // The name of the model that generated the message.
    optional<variant<StopReason, string>> StopReason; // The reason why sampling stopped, if known.
};

MCP_NAMESPACE_END