#pragma once

#include "Core/Types/Common.hpp"

namespace MCP::Types {

struct ExperimentalCapability {
  Passthrough Additional; // For passthrough properties
};

struct ToolsCapability {
    optional<bool> ListChanged;
    Passthrough Additional; // For passthrough properties
};

struct ResourcesCapability {
    optional<bool> Subscribe;
    optional<bool> ListChanged;
    Passthrough Additional; // For passthrough properties
};

struct PromptsCapability {
    optional<bool> ListChanged;
    Passthrough Additional; // For passthrough properties
};

struct LoggingCapability {
    Passthrough Additional; // For passthrough properties
};

struct CompletionCapability {
    Passthrough Additional; // For passthrough properties
};

struct SamplingCapability {
    Passthrough Additional; // For passthrough properties
};

// Capabilities a client may support. Known capabilities are defined here, in this schema, but this is not a closed set: any client can define its own, additional capabilities.
struct ClientCapabilities {
  optional<ExperimentalCapability> Experimental; // Experimental, non-standard capabilities that the client supports.
  optional<SamplingCapability> Sampling; // Present if the client supports sampling from an LLM.
  optional<RootsCapability> Roots; // Present if the client supports listing roots.
  optional<Passthrough> Additional; // Additional properties.
};

// Capabilities that a server may support. Known capabilities are defined here, in this schema, but this is not a closed set: any server can define its own, additional capabilities.
struct ServerCapabilities {
  optional<ExperimentalCapability> Experimental; // Experimental, non-standard capabilities that the server supports.
  optional<LoggingCapability> Logging; // Present if the server supports sending log messages to the client.
  optional<CompletionCapability> Completions; // Present if the server supports sending completions to the client.
  optional<PromptsCapability> Prompts; // Present if the server offers any prompt templates.
  optional<ResourcesCapability> Resources; // Present if the server offers any resources to read.
  optional<ToolsCapability> Tools; // Present if the server offers any tools to call.
  optional<Passthrough> Additional; // Additional properties.
};

} // namespace MCP::Types