#pragma once

#include <optional>
#include <string>
#include <variant>

#include "CoreSDK/Common/Content.h"
#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Common/Roles.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

// Hints to use for model selection. Keys not declared here are currently left unspecified by
// the spec and are up to the client to interpret.
struct ModelHint {
    std::optional<std::string>
        Name; // A hint for a model name. The client SHOULD treat this as a substring
              // of a model name; for example: - `claude-3-5-sonnet` should match
              // `claude-3-5-sonnet-20241022` - `sonnet` should match
              // `claude-3-5-sonnet-20241022`, `claude-3-sonnet-20240229`, etc. - `claude`
              // should match any Claude model The client MAY also map the string to a different
              // provider's model name or a different model family, as long as it fills a
              // similar niche; for example: - `gemini-1.5-flash` could match
              // `claude-3-haiku-20240307`

    JKEY(NAMEKEY, Name, "name")

    DEFINE_TYPE_JSON(ModelHint, NAMEKEY)
};

// The server's preferences for model selection, requested of the client during sampling.
// Because LLMs can vary along multiple dimensions, choosing the "best" model is rarely
// straightforward. Different models excel in different areas—some are faster but less capable,
// others are more capable but more expensive, and so on. This interface allows servers to
// express their priorities across multiple dimensions to help clients make an appropriate
// selection for their use case. These preferences are always advisory. The client MAY ignore
// them. It is also up to the client to decide how to interpret these preferences and how to
// balance them against other considerations.
struct ModelPreferences {
    std::optional<std::vector<ModelHint>>
        Hints; // Optional hints to use for model selection. If multiple hints are specified,
               // the client MUST evaluate them in order (such that the first match is taken).
               // The client SHOULD prioritize these hints over the numeric priorities, but MAY
               // still use the priorities to select from ambiguous matches.
    // TODO: @HalcyonOmega Enforce min = 0, max = 1
    std::optional<double> CostPriority; // How much to prioritize cost when selecting a model. A
                                        // value of 0 means cost is not important, while a value
                                        // of 1 means cost is the most important factor.
    // TODO: @HalcyonOmega Enforce min = 0, max = 1
    std::optional<double>
        SpeedPriority; // How much to prioritize sampling speed (latency) when
                       // selecting a model. A value of 0 means speed is not important,
                       // while a value of 1 means speed is the most important factor.
    // TODO: @HalcyonOmega Enforce min = 0, max = 1
    std::optional<double>
        IntelligencePriority; // How much to prioritize intelligence and capabilities
                              // when selecting a model. A value of 0 means
                              // intelligence is not important, while a value of 1
                              // means intelligence is the most important factor.

    JKEY(HINTSKEY, Hints, "hints")
    JKEY(COSTPRIORITYKEY, CostPriority, "costPriority")
    JKEY(SPEEDPRIORITYKEY, SpeedPriority, "speedPriority")
    JKEY(INTELLIGENCEPRIORITYKEY, IntelligencePriority, "intelligencePriority")

    DEFINE_TYPE_JSON(ModelPreferences, HINTSKEY, COSTPRIORITYKEY, SPEEDPRIORITYKEY,
                     INTELLIGENCEPRIORITYKEY)
};

// Describes a message issued to or received from an LLM API.
struct SamplingMessage {
    MCP::Role Role;
    // TODO: @HalcyonOmega Should this exclude other content types?
    std::variant<TextContent, ImageContent, AudioContent> Content; // The content of the message.

    JKEY(ROLEKEY, Role, "role")
    JKEY(CONTENTKEY, Content, "content")

    DEFINE_TYPE_JSON(SamplingMessage, ROLEKEY, CONTENTKEY)
};

// Result of LLM sampling
// TODO: @HalcyonOmega Cross check with @MCPMessages under SamplingMessage type if possible
struct SamplingResult {
    std::variant<TextContent, ImageContent> Result;
    std::optional<std::string> Model;
    std::optional<std::string> StopReason;

    JKEY(RESULTKEY, Result, "result")
    JKEY(MODELKEY, Model, "model")
    JKEY(STOPREASONKEY, StopReason, "stopReason")

    DEFINE_TYPE_JSON(SamplingResult, RESULTKEY, MODELKEY, STOPREASONKEY)
};

template <typename T>
concept SamplingType = requires(T Type) {
    { Type.Role } -> std::same_as<MCP::Role>;
    { Type.Content } -> std::same_as<std::variant<TextContent, ImageContent, AudioContent>>;
};

MCP_NAMESPACE_END