#pragma once

#include "CommonSchemas.h"
#include "Constants.h"
#include "ContentSchemas.h"
#include "Core.h"
#include "Schemas/Client/ClientSchemas.h"

MCP_NAMESPACE_BEGIN

// SamplingMessage {
//   "description" : "Describes a message issued to or received from an LLM API.",
//                   "properties" : {
//                     "content" : {
//                       "anyOf" : [
//                         {"$ref" : "#/definitions/TextContent"},
//                         {"$ref" : "#/definitions/ImageContent"},
//                         {"$ref" : "#/definitions/AudioContent"}
//                       ]
//                     },
//                     "role" : {"$ref" : "#/definitions/Role"}
//                   },
//                                  "required" : [ "content", "role" ],
//                                               "type" : "object"
// };

// ModelHint {
//   "description"
//       : "Hints to use for model selection.\n\nKeys not declared here are "
//         "currently left "
//         "unspecified by the spec and are up\nto the client to interpret.",
//         "properties" : {
//           "name" : {
//             "description" :
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
//             "type" : "string"
//           }
//         },
//                        "type" : "object"
// };

// ModelPreferences {
//   "description" : "The server's preferences for model selection, requested of "
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
//                   "properties"
//       : {
//         "costPriority" : {
//           "description" : "How much to prioritize cost when selecting a model. "
//                           "A value of 0 "
//                           "means cost\nis not important, while a value of 1 "
//                           "means cost is the "
//                           "most important\nfactor.",
//           "maximum" : 1,
//           "minimum" : 0,
//           "type" : "number"
//         },
//         "hints" : {
//           "description" :
//               "Optional hints to use for model selection.\n\nIf multiple hints "
//               "are specified, the client MUST evaluate them in order\n(such "
//               "that "
//               "the first match is taken).\n\nThe client SHOULD prioritize "
//               "these "
//               "hints over the numeric priorities, but\nMAY still use the "
//               "priorities to select from ambiguous matches.",
//           "items" : {"$ref" : "#/definitions/ModelHint"},
//           "type" : "array"
//         },
//         "intelligencePriority" : {
//           "description" : "How much to prioritize intelligence and "
//                           "capabilities when selecting a\nmodel. "
//                           "A value of 0 means intelligence is not important, "
//                           "while a value of 1\nmeans "
//                           "intelligence is the most important factor.",
//           "maximum" : 1,
//           "minimum" : 0,
//           "type" : "number"
//         },
//         "speedPriority" : {
//           "description" : "How much to prioritize sampling speed (latency) "
//                           "when selecting a "
//                           "model. A\nvalue of 0 means speed is not important, "
//                           "while a value "
//                           "of 1 means speed is\nthe most important factor.",
//           "maximum" : 1,
//           "minimum" : 0,
//           "type" : "number"
//         }
//       },
//         "type" : "object"
// };

// CreateMessageRequest {
//   "description"
//       : "A request from the server to sample an LLM via the client. The client "
//         "has full discretion over which model to select. The client should "
//         "also "
//         "inform the user before beginning sampling, to allow them to inspect "
//         "the "
//         "request (human in the loop) and decide whether to approve it.",
//         "properties"
//       : {
//         "method" : {"const" : "sampling/createMessage", "type" : "string"},
//         "params" : {
//           "properties" : {
//             "includeContext" : {
//               "description" :
//                   "A request to include context from one or more MCP "
//                   "servers (including the caller), to be attached to "
//                   "the prompt. The client MAY ignore this request.",
//               "enum" : [ "allServers", "none", "thisServer" ],
//               "type" : "string"
//             },
//             "maxTokens" : {
//               "description" :
//                   "The maximum number of tokens to sample, as "
//                   "requested by the server. The client MAY choose to "
//                   "sample fewer tokens than requested.",
//               "type" : "integer"
//             },
//             "messages" : {
//               "items" : {"$ref" : "#/definitions/SamplingMessage"},
//               "type" : "array"
//             },
//             "metadata" : {
//               "additionalProperties" : true,
//               "description" :
//                   "Optional metadata to pass through to the LLM provider. The "
//                   "format of this metadata is provider-specific.",
//               "properties" : {},
//               "type" : "object"
//             },
//             "modelPreferences" : {
//               "$ref" : "#/definitions/ModelPreferences",
//               "description" :
//                   "The server's preferences for which model to select. "
//                   "The client MAY ignore these preferences."
//             },
//             "stopSequences" : {"items" : {"type" : "string"}, "type" : "array"},
//             "systemPrompt" : {
//               "description" : "An optional system prompt the server wants to "
//                               "use for sampling. "
//                               "The client MAY modify or omit this prompt.",
//               "type" : "string"
//             },
//             "temperature" : {"type" : "number"}
//           },
//           "required" : [ "maxTokens", "messages" ],
//           "type" : "object"
//         }
//       },
//         "required" : [ "method", "params" ],
//                      "type" : "object"
// };

// CreateMessageResult {
//   "description"
//       : "The client's response to a sampling/create_message request from the "
//         "server. The client should inform the user before returning the "
//         "sampled message, to allow them to inspect the response (human in "
//         "the loop) and decide whether to allow the server to see it.",
//         "properties"
//       : {
//         "_meta" : {
//           "additionalProperties" : {},
//           "description" : "This result property is reserved by the protocol to "
//                           "allow clients and servers to attach additional "
//                           "metadata to their responses.",
//           "type" : "object"
//         },
//         "content" : {
//           "anyOf" : [
//             {"$ref" : "#/definitions/TextContent"},
//             {"$ref" : "#/definitions/ImageContent"},
//             {"$ref" : "#/definitions/AudioContent"}
//           ]
//         },
//         "model" : {
//           "description" : "The name of the model that generated the message.",
//           "type" : "string"
//         },
//         "role" : {"$ref" : "#/definitions/Role"},
//         "stopReason" : {
//           "description" : "The reason why sampling stopped, if known.",
//           "type" : "string"
//         }
//       },
//         "required" : [ "content", "model", "role" ],
//                      "type" : "object"
// };

/**
 * Describes a message issued to or received from an LLM API.
 */
struct SamplingMessage {
    Role role;
    variant<TextContent, ImageContent, AudioContent> content;
};

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
    optional<string> name;
};

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
    /**
     * Optional hints to use for model selection.
     *
     * If multiple hints are specified, the client MUST evaluate them in order
     * (such that the first match is taken).
     *
     * The client SHOULD prioritize these hints over the numeric priorities, but
     * MAY still use the priorities to select from ambiguous matches.
     */
    optional<vector<ModelHint>> hints;

    /**
     * How much to prioritize cost when selecting a model. A value of 0 means cost
     * is not important, while a value of 1 means cost is the most important
     * factor.
     *
     * @TJS-type number
     * @minimum 0
     * @maximum 1
     */
    optional<number> costPriority;

    /**
     * How much to prioritize sampling speed (latency) when selecting a model. A
     * value of 0 means speed is not important, while a value of 1 means speed is
     * the most important factor.
     *
     * @TJS-type number
     * @minimum 0
     * @maximum 1
     */
    optional<number> speedPriority;

    /**
     * How much to prioritize intelligence and capabilities when selecting a
     * model. A value of 0 means intelligence is not important, while a value of 1
     * means intelligence is the most important factor.
     *
     * @TJS-type number
     * @minimum 0
     * @maximum 1
     */
    optional<number> intelligencePriority;
};

/**
 * A request from the server to sample an LLM via the client. The client has full discretion over
 * which model to select. The client should also inform the user before beginning sampling, to allow
 * them to inspect the request (human in the loop) and decide whether to approve it.
 */
struct CreateMessageRequest : public Request {
    params : {
        vector<SamplingMessage> messages;
        /**
         * The server's preferences for which model to select. The client MAY ignore these
         * preferences.
         */
        optional<ModelPreferences> modelPreferences;
        /**
         * An optional system prompt the server wants to use for sampling. The client MAY modify or
         * omit this prompt.
         */
        optional<string> systemPrompt;
        /**
         * A request to include context from one or more MCP servers (including the caller), to be
         * attached to the prompt. The client MAY ignore this request.
         */
        includeContext ?: "none" | "thisServer" | "allServers";
        /**
         * @TJS-type number
         */
        optional<number> temperature;
        /**
         * The maximum number of tokens to sample, as requested by the server. The client MAY choose
         * to sample fewer tokens than requested.
         */
        number maxTokens;
        optional<vector<string>> stopSequences;
        /**
         * Optional metadata to pass through to the LLM provider. The format of this metadata is
         * provider-specific.
         */
        optional<JSON> metadata;
    };

    CreateMessageRequest() {
        method = MTHD_SAMPLING_CREATE_MESSAGE;
    }
};

// TODO: Typescript extended from Result and SamplingMessage - How to convert properly?
/**
 * The client's response to a sampling/create_message request from the server. The client should
 * inform the user before returning the sampled message, to allow them to inspect the response
 * (human in the loop) and decide whether to allow the server to see it.
 */
struct CreateMessageResult : public Result, SamplingMessage {
    /**
     * The name of the model that generated the message.
     */
    string model;
    /**
     * The reason why sampling stopped, if known.
     */
    stopReason ?: "endTurn" | "stopSequence" | "maxTokens" | string;
};

MCP_NAMESPACE_END