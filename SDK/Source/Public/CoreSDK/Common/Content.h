#pragma once

#include <Poco/Net/MediaType.h>

#include <optional>
#include <string>
#include <variant>

#include "CoreSDK/Common/Annotations.h"
#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"
#include "URIProxy.h"

MCP_NAMESPACE_BEGIN

// TODO: @HalcyonOmega - Attempt to fix Poco::Data::BLOB instead of this
using BLOB = std::vector<char>;

struct Content
{
	std::string Type{ "ContentType" };						 // The type of content.
	std::optional<FAnnotations> Annotations{ std::nullopt }; // Optional annotations for the client.

	JSON_KEY(TYPEKEY, Type, "type")
	JSON_KEY(ANNOTATIONSKEY, Annotations, "annotations")

	DEFINE_TYPE_JSON(Content, TYPEKEY, ANNOTATIONSKEY)

	explicit Content(const std::string_view InType, const std::optional<FAnnotations>& InAnnotations = std::nullopt)
		: Type(InType),
		  Annotations(InAnnotations) {};
	Content() = default;
	~Content() = default;
};

// TextContent {
//   MSG_DESCRIPTION: "Text provided to or from an LLM.",
//                   MSG_PROPERTIES: {
//                     MSG_ANNOTATIONS: {
//                       "$ref": "#/definitions/FAnnotations",
//                       MSG_DESCRIPTION: "Optional annotations for the client."
//                     },
//                     MSG_TEXT: {
//                       MSG_DESCRIPTION: "The text content of the message.",
//                       MSG_TYPE: MSG_STRING
//                     },
//                     MSG_TYPE: {MSG_CONST: MSG_TEXT, MSG_TYPE: MSG_STRING}
//                   },
//                                  MSG_REQUIRED: [ MSG_TEXT, MSG_TYPE ],
//                                               MSG_TYPE: MSG_OBJECT
// };

// Text provided to or from an LLM.
struct TextContent : Content
{
	std::string Text{ "Message" }; // The text content of the message.

	JSON_KEY(TEXTKEY, Text, "text")

	DEFINE_TYPE_JSON_DERIVED(TextContent, Content, TEXTKEY)

	TextContent() : Content("text", std::nullopt) {}
	explicit TextContent(const std::string_view InText) : Content("text", std::nullopt), Text(InText) {}
};

// ImageContent {
//   MSG_DESCRIPTION: "An image provided to or from an LLM.",
//       MSG_PROPERTIES: {
//         MSG_ANNOTATIONS: {
//           "$ref": "#/definitions/FAnnotations",
//           MSG_DESCRIPTION: "Optional annotations for the client."
//         },
//         MSG_DATA: {
//           MSG_DESCRIPTION: "The base64-encoded image data.",
//           MSG_FORMAT: MSG_BYTE,
//           MSG_TYPE: MSG_STRING
//         },
//         MSG_MIME_TYPE: {
//           MSG_DESCRIPTION: "The MIME type of the image. Different providers may "
//                           "support different image types.",
//           MSG_TYPE: MSG_STRING
//         },
//         MSG_TYPE: {MSG_CONST: "image", MSG_TYPE: MSG_STRING}
//       },
//         MSG_REQUIRED: [ MSG_DATA, MSG_MIME_TYPE, MSG_TYPE ],
//                      MSG_TYPE: MSG_OBJECT
// };

// An image provided to or from an LLM.
struct ImageContent : Content
{
	// TODO: @HalcyonOmega @format byte (base64)
	std::string Data{ "" };				  // The base64-encoded image data.
	FMIMEType MIMEType{ "image", "png" }; // The MIME type of the image. Different
										  // providers may support different image types.

	JSON_KEY(DATAKEY, Data, "data")
	JSON_KEY(MIMETYPEKEY, MIMEType, "mimeType")

	DEFINE_TYPE_JSON_DERIVED(ImageContent, Content, DATAKEY, MIMETYPEKEY)

	ImageContent() : Content("image", std::nullopt) {}
	ImageContent(const std::string_view InData, const FMIMEType& InMediaType)
		: Content("image", std::nullopt),
		  Data(InData),
		  MIMEType(InMediaType)
	{}
};

// AudioContent {
//   MSG_DESCRIPTION: "Audio provided to or from an LLM.",
//     MSG_PROPERTIES: {
//         MSG_ANNOTATIONS: {
//           "$ref": "#/definitions/FAnnotations",
//           MSG_DESCRIPTION: "Optional annotations for the client."
//         },
//         MSG_DATA: {
//           MSG_DESCRIPTION: "The base64-encoded audio data.",
//           MSG_FORMAT: MSG_BYTE,
//           MSG_TYPE: MSG_STRING
//         },
//         MSG_MIME_TYPE: {
//           MSG_DESCRIPTION: "The MIME type of the audio. Different providers may "
//                           "support different audio types.",
//           MSG_TYPE: MSG_STRING
//         },
//         MSG_TYPE: {MSG_CONST: "audio", MSG_TYPE: MSG_STRING}
//       },
//         MSG_REQUIRED: [ MSG_DATA, MSG_MIME_TYPE, MSG_TYPE ],
//                      MSG_TYPE: MSG_OBJECT
// };

// Audio provided to or from an LLM.
struct AudioContent : Content
{
	// TODO: @HalcyonOmega @format byte (base64)
	std::string Data{ "" };				   // The base64-encoded audio data.
	FMIMEType MIMEType{ "audio", "mpeg" }; // The MIME type of the audio. Different
										   // providers may support different audio types.

	JSON_KEY(DATAKEY, Data, "data")
	JSON_KEY(MIMETYPEKEY, MIMEType, "mimeType")

	DEFINE_TYPE_JSON_DERIVED(AudioContent, Content, DATAKEY, MIMETYPEKEY)

	AudioContent() : Content("audio", std::nullopt) {}
	AudioContent(const std::string_view InData, const FMIMEType& InMediaType)
		: Content("audio", std::nullopt),
		  Data(InData),
		  MIMEType(InMediaType)
	{}
};

// ResourceContents {
//   MSG_DESCRIPTION: "The contents of a specific resource or sub-resource.",
//     MSG_PROPERTIES: {
//         MSG_MIME_TYPE: {
//           MSG_DESCRIPTION: "The MIME type of this resource, if known.",
//           MSG_TYPE: MSG_STRING
//         },
//         MSG_URI: {
//           MSG_DESCRIPTION: "The URI of this resource.",
//           MSG_FORMAT: MSG_URI,
//           MSG_TYPE: MSG_STRING
//         }
//       },
//         MSG_REQUIRED: [MSG_URI],
//                      MSG_TYPE: MSG_OBJECT
// };

// The contents of a specific resource or sub-resource.
struct ResourceContents
{
	MCP::URI URI{};									   // The URI of this resource.
	std::optional<FMIMEType> MIMEType{ std::nullopt }; // The MIME type of this resource, if known.

	JSON_KEY(URIKEY, URI, "uri")
	JSON_KEY(MIMETYPEKEY, MIMEType, "mimeType")

	DEFINE_TYPE_JSON(ResourceContents, URIKEY, MIMETYPEKEY)

	ResourceContents() = default;
	explicit ResourceContents(const MCP::URI& InURI, const std::optional<FMIMEType>& InMIMEType = std::nullopt)
		: URI(InURI),
		  MIMEType(InMIMEType)
	{}
};

// TextResourceContents {
//   MSG_PROPERTIES: {
//     MSG_MIME_TYPE: {
//       MSG_DESCRIPTION: "The MIME type of this resource, if known.",
//       MSG_TYPE: MSG_STRING
//     },
//     MSG_TEXT: {
//       MSG_DESCRIPTION: "The text of the item. This must only be set if the
//       item "
//                       "can actually be represented as text (not binary
//                       data).",
//       MSG_TYPE: MSG_STRING
//     },
//     MSG_URI: {
//       MSG_DESCRIPTION: "The URI of this resource.",
//       MSG_FORMAT: MSG_URI,
//       MSG_TYPE: MSG_STRING
//     }
//   },
//                  MSG_REQUIRED: [ MSG_TEXT, MSG_URI ],
//                               MSG_TYPE: MSG_OBJECT
// };

// The contents of a text resource.
struct TextResourceContents : ResourceContents
{
	std::string Text{ "" }; // The text of the item. This must only be set if the item can actually be
							// represented as text (not binary data).

	JSON_KEY(TEXTKEY, Text, "text")

	DEFINE_TYPE_JSON_DERIVED(TextResourceContents, ResourceContents, TEXTKEY)

	TextResourceContents(const std::string_view& InText,
		const MCP::URI& InURI,
		const std::optional<FMIMEType>& InMIMEType = std::nullopt)
		: ResourceContents(InURI, FMIMEType{ "text", "plain" }),
		  Text(InText)
	{
		MIMEType->setParameter("charset", "utf-8");
	}

	TextResourceContents() : ResourceContents(MCP::URI{}, FMIMEType{ "text", "plain" })
	{
		MIMEType->setParameter("charset", "utf-8");
	}
};

// BlobResourceContents {
//   MSG_PROPERTIES: {
//     MSG_BLOB: {
//       MSG_DESCRIPTION:
//           "A base64-encoded string representing the binary data of the
//           item.",
//       MSG_FORMAT: MSG_BYTE,
//       MSG_TYPE: MSG_STRING
//     },
//     MSG_MIME_TYPE: {
//       MSG_DESCRIPTION: "The MIME type of this resource, if known.",
//       MSG_TYPE: MSG_STRING
//     },
//     MSG_URI: {
//       MSG_DESCRIPTION: "The URI of this resource.",
//       MSG_FORMAT: MSG_URI,
//       MSG_TYPE: MSG_STRING
//     }
//   },
//                  MSG_REQUIRED: [ MSG_BLOB, MSG_URI ],
//                               MSG_TYPE: MSG_OBJECT
// };

// The contents of a blob resource.
struct BlobResourceContents : ResourceContents
{
	// TODO: @HalcyonOmega @format byte (base64) blob
	MCP::BLOB Blob{}; // A base64-encoded string representing the binary data of the item.

	JSON_KEY(BLOBKEY, Blob, "blob")

	DEFINE_TYPE_JSON_DERIVED(BlobResourceContents, ResourceContents, BLOBKEY)

	BlobResourceContents(const MCP::BLOB& InBlob, const MCP::URI& InURI)
		: ResourceContents(InURI, FMIMEType{ "application", "octet-stream" }),
		  Blob(InBlob)
	{}

	BlobResourceContents() : ResourceContents(MCP::URI{}, FMIMEType{ "application", "octet-stream" }) {}
};

// EmbeddedResource {
//   MSG_DESCRIPTION: "The contents of a resource, embedded into a prompt or tool call result.
//         It is up to the client how best to render embedded resources for the benefit of the LLM and/or the user.",
//         MSG_PROPERTIES: {
//           MSG_ANNOTATIONS: {
//             "$ref": "#/definitions/FAnnotations",
//             MSG_DESCRIPTION: "Optional annotations for the client."
//           },
//           MSG_RESOURCE: {
//             "anyOf": [
//               {"$ref": "#/definitions/TextResourceContents"},
//               {"$ref": "#/definitions/BlobResourceContents"}
//             ]
//           },
//           MSG_TYPE: {MSG_CONST: MSG_RESOURCE, MSG_TYPE: MSG_STRING}
//         },
//                        MSG_REQUIRED: [ MSG_RESOURCE, MSG_TYPE ],
//                                     MSG_TYPE: MSG_OBJECT
// };

// The contents of a resource, embedded into a prompt or tool call result.
struct EmbeddedResource : Content
{
	std::variant<TextResourceContents, BlobResourceContents> Resource{ TextResourceContents{ "", MCP::URI{} } };

	template <typename BasicJSONType> friend void to_json(BasicJSONType& JSON_J, const EmbeddedResource& JSON_T)
	{
		to_json(JSON_J, static_cast<const Content&>(JSON_T));
		JSON_J["resource"] = JSON_T.Resource;
	}

	template <typename BasicJSONType> friend void from_json(const BasicJSONType& JSON_J, EmbeddedResource& JSON_T)
	{
		from_json(JSON_J, static_cast<Content&>(JSON_T));
		if (const auto& Content = JSON_J.at("resource"); Content.contains("text"))
		{
			JSON_T.Resource = Content.template get<TextResourceContents>();
		}
		else if (Content.contains("blob"))
		{
			JSON_T.Resource = Content.template get<BlobResourceContents>();
		}
		else
		{
			JSON_T.Resource = TextResourceContents{ "", MCP::URI{} };
		}
	}

	template <typename T>
		requires(std::is_same_v<std::decay_t<T>, TextResourceContents>
					|| std::is_same_v<std::decay_t<T>, BlobResourceContents>)
	explicit EmbeddedResource(T&& InResource) : Content("resource", std::nullopt),
												Resource(std::forward<T>(InResource))
	{}
	EmbeddedResource() : Content("resource", std::nullopt) {}
};

MCP_NAMESPACE_END