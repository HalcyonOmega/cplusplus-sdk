#pragma once

#include <string>
#include <variant>

#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"
#include "URIProxy.h"

MCP_NAMESPACE_BEGIN

struct AutocompleteReference
{
	std::string Type;

	JSON_KEY(TYPEKEY, Type, "type")

	DEFINE_TYPE_JSON(AutocompleteReference, TYPEKEY)

	AutocompleteReference() = default;
	explicit AutocompleteReference(const std::string_view InType) : Type(InType) {}
};

// ResourceReference {
//   MSG_DESCRIPTION: "A reference to a resource or resource template definition.",
//    MSG_PROPERTIES: {
//         MSG_TYPE: {MSG_CONST: MSG_REF_RESOURCE, MSG_TYPE: MSG_STRING},
//         MSG_URI: {
//           MSG_DESCRIPTION: "The URI or URI template of the resource.",
//           MSG_FORMAT: MSG_URITEMPLATE,
//           MSG_TYPE: MSG_STRING
//         }
//       },
//         MSG_REQUIRED: [ MSG_TYPE, MSG_URI ],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * A reference to a resource or resource template definition.
 */
struct ResourceReference : AutocompleteReference
{
	std::variant<MCP::URI, MCP::URITemplate> URI; // The URI or URI template of the resource.

	ResourceReference() : AutocompleteReference("ref/resource") {}

	JSON_KEY(URIKEY, URI, "uri")

	DEFINE_TYPE_JSON_DERIVED(ResourceReference, AutocompleteReference, URIKEY)
};

// PromptReference {
//   MSG_DESCRIPTION: "Identifies a prompt.",
//    MSG_PROPERTIES: {
//         MSG_NAME: {
//           MSG_DESCRIPTION: "The name of the prompt or prompt template",
//           MSG_TYPE: MSG_STRING
//         },
//         MSG_TYPE: {MSG_CONST: MSG_REF_PROMPT, MSG_TYPE: MSG_STRING}
//       },
//         MSG_REQUIRED: [ MSG_NAME, MSG_TYPE ],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * Identifies a prompt.
 */
struct PromptReference : AutocompleteReference
{
	std::string Name; // The name of the prompt or prompt template

	PromptReference() : AutocompleteReference("ref/prompt") {}

	JSON_KEY(NAMEKEY, Name, "name")

	DEFINE_TYPE_JSON_DERIVED(PromptReference, AutocompleteReference, NAMEKEY)
};

MCP_NAMESPACE_END