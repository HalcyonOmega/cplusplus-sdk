#pragma once

#include "Core/Types/Common.hpp"

namespace MCP::Types {

// Hints to use for model selection.
struct ModelHint {
  optional<string> Name; // A hint for a model name.
  Passthrough Additional; // Additional properties.
};

// The server's preferences for model selection, requested of the client during sampling.
struct ModelPreferences {
  optional<vector<ModelHint>> Hints; // Optional hints to use for model selection.
  optional<double.min(0).max(1)> CostPriority; // How much to prioritize cost when selecting a model.
  optional<double.min(0).max(1)> SpeedPriority; // How much to prioritize sampling speed (latency) when selecting a model.
  optional<double.min(0).max(1)> IntelligencePriority; // How much to prioritize intelligence and capabilities when selecting a model.
  Passthrough Additional; // Additional properties.
};

// Describes a message issued to or received from an LLM API.
struct SamplingMessage {
  z.enum(["user", "assistant"]) Role;
  variant<TextContent, ImageContent, AudioContent> Content; // The content of the message.
  Passthrough Additional; // Additional properties.
};

} // namespace MCP::Types