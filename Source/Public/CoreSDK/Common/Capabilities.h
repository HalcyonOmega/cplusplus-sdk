#pragma once

#include <optional>

#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

struct RootsCapability
{
	std::optional<bool> ListChanged{
		std::nullopt
	}; // Whether the client supports notifications for changes to the roots list.
	JSONData AdditionalProperties{};

	JSON_KEY(LISTCHANGEDKEY, ListChanged, "listChanged")
	JSON_KEY(ADDITIONALPROPERTIESKEY, AdditionalProperties, "additionalProperties")

	DEFINE_TYPE_JSON(RootsCapability, LISTCHANGEDKEY, ADDITIONALPROPERTIESKEY)

	explicit RootsCapability(const std::optional<bool> InListChanged = std::nullopt)
		: ListChanged(InListChanged),
		  AdditionalProperties(JSONData{})
	{}
};

struct SamplingCapability
{
	JSONData AdditionalProperties{};

	JSON_KEY(ADDITIONALPROPERTIESKEY, AdditionalProperties, "additionalProperties")

	DEFINE_TYPE_JSON(SamplingCapability, ADDITIONALPROPERTIESKEY)
};

struct ExperimentalCapability
{
	JSONData AdditionalProperties{};

	JSON_KEY(ADDITIONALPROPERTIESKEY, AdditionalProperties, "additionalProperties")

	DEFINE_TYPE_JSON(ExperimentalCapability, ADDITIONALPROPERTIESKEY)
};

struct LoggingCapability
{
	JSONData AdditionalProperties{};

	JSON_KEY(ADDITIONALPROPERTIESKEY, AdditionalProperties, "additionalProperties")

	DEFINE_TYPE_JSON(LoggingCapability, ADDITIONALPROPERTIESKEY)
};

struct PromptsCapability
{
	std::optional<bool> ListChanged{ std::nullopt }; // Whether this server supports notifications for changes
													 // to the prompt list.
	JSONData AdditionalProperties{};

	JSON_KEY(LISTCHANGEDKEY, ListChanged, "listChanged")
	JSON_KEY(ADDITIONALPROPERTIESKEY, AdditionalProperties, "additionalProperties")

	DEFINE_TYPE_JSON(PromptsCapability, LISTCHANGEDKEY, ADDITIONALPROPERTIESKEY)
};

struct ResourcesCapability
{
	std::optional<bool> Subscribe{ std::nullopt };	 // Whether this server supports subscribing to resource updates.
	std::optional<bool> ListChanged{ std::nullopt }; // Whether this server supports notifications for changes
													 // to the resource list.
	JSONData AdditionalProperties{};

	JSON_KEY(SUBSCRIBEKEY, Subscribe, "subscribe")
	JSON_KEY(LISTCHANGEDKEY, ListChanged, "listChanged")
	JSON_KEY(ADDITIONALPROPERTIESKEY, AdditionalProperties, "additionalProperties")

	DEFINE_TYPE_JSON(ResourcesCapability, SUBSCRIBEKEY, LISTCHANGEDKEY, ADDITIONALPROPERTIESKEY)
};

struct CompletionCapability
{
	JSONData AdditionalProperties{};

	JSON_KEY(ADDITIONALPROPERTIESKEY, AdditionalProperties, "additionalProperties")

	DEFINE_TYPE_JSON(CompletionCapability, ADDITIONALPROPERTIESKEY)
};

struct ToolsCapability
{
	std::optional<bool> ListChanged{
		std::nullopt
	}; // Whether this server supports notifications for changes to the tool list.
	JSONData AdditionalProperties{};

	JSON_KEY(LISTCHANGEDKEY, ListChanged, "listChanged")
	JSON_KEY(ADDITIONALPROPERTIESKEY, AdditionalProperties, "additionalProperties")

	DEFINE_TYPE_JSON(ToolsCapability, LISTCHANGEDKEY, ADDITIONALPROPERTIESKEY)
};

struct ElicitationCapability
{
	JSONData AdditionalProperties{};

	JSON_KEY(ADDITIONALPROPERTIESKEY, AdditionalProperties, "additionalProperties")

	DEFINE_TYPE_JSON(ElicitationCapability, ADDITIONALPROPERTIESKEY)
};

// ClientCapabilities {
//     MSG_DESCRIPTION: "Capabilities a client may support. Known capabilities are defined here, in this schema, but
//     this is not a closed set: any client can define its own, additional capabilities.",
//           MSG_PROPERTIES: {
//             MSG_EXPERIMENTAL: {
//                 MSG_ADDITIONAL_PROPERTIES:
//                     {MSG_ADDITIONAL_PROPERTIES: true, MSG_PROPERTIES: {}, MSG_TYPE:
//                     MSG_OBJECT},
//                 MSG_DESCRIPTION: "Experimental, non-standard capabilities that
//                 the client supports.", MSG_TYPE: MSG_OBJECT
//             },
//             MSG_ROOTS: {
//                 MSG_DESCRIPTION: "Present if the client supports listing roots.",
//                 MSG_PROPERTIES: {
//                     MSG_LIST_CHANGED: {
//                         MSG_DESCRIPTION: "Whether the client supports
//                         notifications for changes to the roots list.",
//                         MSG_TYPE: MSG_BOOLEAN
//                     }
//                 },
//                 MSG_TYPE: MSG_OBJECT
//             },
//             MSG_SAMPLING: {
//                 MSG_ADDITIONAL_PROPERTIES: true,
//                 MSG_DESCRIPTION: "Present if the client supports sampling from
//                 an LLM.",
//                 MSG_PROPERTIES: {}, MSG_TYPE: MSG_OBJECT
//             }
//         },
//           MSG_TYPE : MSG_OBJECT
// };

/**
 * Capabilities a client may support. Known capabilities are defined here, in this schema, but this
 * is not a closed set: any client can define its own, additional capabilities.
 */
struct ClientCapabilities
{
	std::optional<ExperimentalCapability> Experimental{
		std::nullopt
	}; // Experimental, non-standard capabilities that the client supports.
	std::optional<SamplingCapability> Sampling{ std::nullopt }; // Present if the client supports sampling from an LLM.
	std::optional<RootsCapability> Roots{ std::nullopt };		// Present if the client supports listing roots.
	std::optional<ElicitationCapability> Elicitation{
		std::nullopt
	}; // Present if the client supports eliciting user input.

	JSON_KEY(ROOTSKEY, Roots, "roots")
	JSON_KEY(SAMPLINGKEY, Sampling, "sampling")
	JSON_KEY(EXPERIMENTALKEY, Elicitation, "elicitation")
	JSON_KEY(ELICITATIONKEY, Experimental, "experimental")

	DEFINE_TYPE_JSON(ClientCapabilities, ROOTSKEY, SAMPLINGKEY, EXPERIMENTALKEY)
};

// ServerCapabilities {
//     MSG_DESCRIPTION: "Capabilities that a server may support. Known
//     capabilities are defined here,
//     "
//                    "in this schema, but this is not a closed set: any server
//                    can define its own, " "additional capabilities.",
//     MSG_PROPERTIES: {
//         MSG_COMPLETIONS: {
//             MSG_ADDITIONAL_PROPERTIES: true,
//             MSG_DESCRIPTION: "Present if the server supports argument
//             autocompletion suggestions.", MSG_PROPERTIES: {}, MSG_TYPE: MSG_OBJECT
//         },
//         MSG_EXPERIMENTAL: {
//             MSG_ADDITIONAL_PROPERTIES:
//                 {MSG_ADDITIONAL_PROPERTIES: true, MSG_PROPERTIES: {}, MSG_TYPE:
//                 MSG_OBJECT},
//             MSG_DESCRIPTION: "Experimental, non-standard capabilities that the
//             server supports.", MSG_TYPE: MSG_OBJECT
//         },
//         MSG_LOGGING: {
//             MSG_ADDITIONAL_PROPERTIES: true,
//             MSG_DESCRIPTION: "Present if the server supports sending log
//             messages to the client.", MSG_PROPERTIES: {}, MSG_TYPE: MSG_OBJECT
//         },
//         MSG_PROMPTS: {
//             MSG_DESCRIPTION: "Present if the server offers any prompt
//             templates.", MSG_PROPERTIES: {
//                 MSG_LIST_CHANGED: {
//                     MSG_DESCRIPTION: "Whether this server supports
//                     notifications for changes to the
//                     "
//                                    "prompt list.",
//                     MSG_TYPE: MSG_BOOLEAN
//                 }
//             },
//             MSG_TYPE: MSG_OBJECT
//         },
//         MSG_RESOURCES: {
//             MSG_DESCRIPTION: "Present if the server offers any resources to
//             read.", MSG_PROPERTIES: {
//                 MSG_LIST_CHANGED: {
//                     MSG_DESCRIPTION: "Whether this server supports
//                     notifications for changes to the
//                     "
//                                    "resource list.",
//                     MSG_TYPE: MSG_BOOLEAN
//                 },
//                 MSG_SUBSCRIBE: {
//                     MSG_DESCRIPTION: "Whether this server supports subscribing
//                     to resource updates.", MSG_TYPE: MSG_BOOLEAN
//                 }
//             },
//             MSG_TYPE: MSG_OBJECT
//         },
//         MSG_TOOLS: {
//             MSG_DESCRIPTION: "Present if the server offers any tools to call.",
//             MSG_PROPERTIES: {
//                 MSG_LIST_CHANGED: {
//                     MSG_DESCRIPTION: "Whether this server supports
//                     notifications for changes to "
//                                    "the tool list.",
//                     MSG_TYPE: MSG_BOOLEAN
//                 }
//             },
//             MSG_TYPE: MSG_OBJECT
//         }
//     },
//     MSG_TYPE: MSG_OBJECT
// };

/**
 * Capabilities that a server may support. Known capabilities are defined here, in this schema, but
 * this is not a closed set: any server can define its own, additional capabilities.
 */
struct ServerCapabilities
{
	std::optional<ExperimentalCapability> Experimental{
		std::nullopt
	}; // Experimental, non-standard capabilities that the server supports.
	std::optional<LoggingCapability> Logging{
		std::nullopt
	}; // Present if the server supports sending log messages to the client.
	std::optional<CompletionCapability> Completions{
		std::nullopt
	}; // Present if the server supports sending completions to the client.
	std::optional<PromptsCapability> Prompts{ std::nullopt };	  // Present if the server offers any prompt templates.
	std::optional<ResourcesCapability> Resources{ std::nullopt }; // Present if the server offers any resources to read.
	std::optional<ToolsCapability> Tools{ std::nullopt };		  // Present if the server offers any tools to call.

	JSON_KEY(EXPERIMENTALKEY, Experimental, "experimental")
	JSON_KEY(LOGGINGKEY, Logging, "logging")
	JSON_KEY(PROMPTSKEY, Prompts, "prompts")
	JSON_KEY(RESOURCESKEY, Resources, "resources")
	JSON_KEY(TOOLSKEY, Tools, "tools")

	DEFINE_TYPE_JSON(ServerCapabilities, EXPERIMENTALKEY, LOGGINGKEY, PROMPTSKEY, RESOURCESKEY, TOOLSKEY)
};

MCP_NAMESPACE_END