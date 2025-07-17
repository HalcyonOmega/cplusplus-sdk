#pragma once

#include "CoreSDK/Common/Annotations.h"
#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"
#include "Poco/Net/MediaType.h"
#include "URIProxy.h"

MCP_NAMESPACE_BEGIN

// Resource {
//   MSG_DESCRIPTION: "A known resource that the server is capable of reading.",
//                   MSG_PROPERTIES: {
//         MSG_ANNOTATIONS: {
//           "$ref": "#/definitions/Annotations",
//           MSG_DESCRIPTION: "Optional annotations for the client."
//         },
//         MSG_DESCRIPTION: {
//           MSG_DESCRIPTION:
//               "A description of what this resource represents.\n\nThis can be
//               " "used " "by clients to improve the LLM's understanding of
//               available " "resources. It can be thought of like a \"hint\" to
//               the model.",
//           MSG_TYPE: MSG_STRING
//         },
//         MSG_MIME_TYPE: {
//           MSG_DESCRIPTION: "The MIME type of this resource, if known.",
//           MSG_TYPE: MSG_STRING
//         },
//         MSG_NAME: {
//           MSG_DESCRIPTION:
//               "A human-readable name for this resource.\n\nThis can be "
//               "used by clients to populate UI elements.",
//           MSG_TYPE: MSG_STRING
//         },
//         MSG_SIZE: {
//           MSG_DESCRIPTION:
//               "The size of the raw resource content, in bytes (i.e., before "
//               "base64 "
//               "encoding or any tokenization), if known.\n\nThis can be used
//               by " "Hosts to display file sizes and estimate context window
//               usage.",
//           MSG_TYPE: MSG_INTEGER
//         },
//         MSG_URI: {
//           MSG_DESCRIPTION: "The URI of this resource.",
//           MSG_FORMAT: MSG_URI,
//           MSG_TYPE: MSG_STRING
//         }
//       },
//         MSG_REQUIRED: [ MSG_NAME, MSG_URI ],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * A known resource that the server is capable of reading.
 */
struct Resource
{
	MCP::URI URI;								  // The URI of this resource.
	std::string Name;							  // A human-readable name for this resource. Clients can use this to
												  // populate UI elements.
	std::optional<std::string> Description;		  // A description of what this resource represents. Clients can
												  // use this to improve the LLM's understanding of available
												  // resources. It can be thought of as a "hint" to the model.
	std::optional<Poco::Net::MediaType> MIMEType; // The MIME type of this resource, if known.
	std::optional<Annotations> Annotations;		  // Optional annotations for the client.
	std::optional<int64_t> Size;				  // The size of the raw resource content, in bytes (i.e., before base64
												  // encoding or any tokenization), if known. Hosts can use this
												  // to display file sizes and estimate context window usage.

	JSON_KEY(URIKEY, URI, "uri")
	JSON_KEY(NAMEKEY, Name, "name")
	JSON_KEY(DESCRIPTIONKEY, Description, "description")
	JSON_KEY(MIMETYPEKEY, MIMEType, "mimeType")
	JSON_KEY(ANNOTATIONSKEY, Annotations, "annotations")
	JSON_KEY(SIZEKEY, Size, "size")

	DEFINE_TYPE_JSON(Resource, URIKEY, NAMEKEY, DESCRIPTIONKEY, MIMETYPEKEY, ANNOTATIONSKEY, SIZEKEY)

	bool operator<(const Resource& InOther) const
	{
		if (Name != InOther.Name)
		{
			return Name < InOther.Name;
		}
		return URI.toString() < InOther.URI.toString();
	}

	bool operator==(const Resource& InOther) const { return URI == InOther.URI && Name == InOther.Name; }
};

// ResourceTemplate {
//   MSG_DESCRIPTION: "A template description for resources available on the server.",
//         MSG_PROPERTIES: {
//         MSG_ANNOTATIONS: {
//           "$ref": "#/definitions/Annotations",
//           MSG_DESCRIPTION: "Optional annotations for the client."
//         },
//         MSG_DESCRIPTION: {
//           MSG_DESCRIPTION:
//               "A description of what this template is for.\n\nThis can be
//               used " "by clients to improve the LLM's understanding of
//               available " "resources. It can be thought of like a \"hint\" to
//               the model.",
//           MSG_TYPE: MSG_STRING
//         },
//         MSG_MIME_TYPE: {
//           MSG_DESCRIPTION:
//               "The MIME type for all resources that match this template. This
//               " "should only be included if all resources matching this
//               template " "have the same type.",
//           MSG_TYPE: MSG_STRING
//         },
//         MSG_NAME: {
//           MSG_DESCRIPTION: "A human-readable name for the type of resource
//           this "
//                           "template refers to.\n\nThis can be used by clients
//                           " "to populate UI elements.",
//           MSG_TYPE: MSG_STRING
//         },
//         MSG_URI_TEMPLATE: {
//           MSG_DESCRIPTION: "A URI template (according to RFC 6570) that can be
//           "
//                           "used to construct resource URIs.",
//           MSG_FORMAT: MSG_URITEMPLATE,
//           MSG_TYPE: MSG_STRING
//         }
//       },
//         MSG_REQUIRED: [ MSG_NAME, MSG_URI_TEMPLATE ],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * A template description for resources available on the server.
 */
struct ResourceTemplate
{
	MCP::URITemplate URITemplate;			// A URI template (according to RFC 6570) that can be used to
											// construct resource URIs.
	std::string Name;						// A human-readable name for the type of resource this template refers to.
											// Clients can use this to populate UI elements.
	std::optional<std::string> Description; // A description of what this template is for. Clients
											// can use this to improve the LLM's
											// understanding of available resources. It can be
											// thought of as a "hint" to the model.
	std::optional<Poco::Net::MediaType>
		MIMEType; // The MIME type for all resources that match this template. This should only
				  // be included if all resources matching this template have the same type.
	std::optional<Annotations> Annotations; // Optional annotations for the client.

	JSON_KEY(URITEMPLATEKEY, URITemplate, "uriTemplate")
	JSON_KEY(NAMEKEY, Name, "name")
	JSON_KEY(DESCRIPTIONKEY, Description, "description")
	JSON_KEY(MIMETYPEKEY, MIMEType, "mimeType")
	JSON_KEY(ANNOTATIONSKEY, Annotations, "annotations")

	DEFINE_TYPE_JSON(ResourceTemplate, URITEMPLATEKEY, NAMEKEY, DESCRIPTIONKEY, MIMETYPEKEY, ANNOTATIONSKEY)

	bool operator<(const ResourceTemplate& InOther) const
	{
		if (Name != InOther.Name)
		{
			return Name < InOther.Name;
		}
		return URITemplate.ToString() < InOther.URITemplate.ToString();
	}

	bool operator==(const ResourceTemplate& InOther) const
	{
		return URITemplate.ToString() == InOther.URITemplate.ToString() && Name == InOther.Name;
	}
};

template <typename T>
concept ResourceType = requires(T Type) {
	{ Type.URI } -> std::same_as<MCP::URI>;
	{ Type.Name } -> std::convertible_to<std::string>;
	{ Type.Description } -> std::same_as<std::optional<std::string>>;
	{ Type.MIMEType } -> std::same_as<std::optional<Poco::Net::MediaType>>;
	{ Type.Annotations } -> std::same_as<std::optional<Annotations>>;
	{ Type.Size } -> std::same_as<std::optional<int64_t>>;
};

template <typename T>
concept ResourceTemplateType = requires(T Type) {
	{ Type.URITemplate } -> std::same_as<MCP::URITemplate>;
	{ Type.Name } -> std::convertible_to<std::string>;
	{ Type.Description } -> std::same_as<std::optional<std::string>>;
	{ Type.MIMEType } -> std::same_as<std::optional<Poco::Net::MediaType>>;
	{ Type.Annotations } -> std::same_as<std::optional<Annotations>>;
};

MCP_NAMESPACE_END