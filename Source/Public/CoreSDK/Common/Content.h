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
	std::string Type;						// The type of content.
	std::optional<Annotations> Annotations; // Optional annotations for the client.

	JKEY(TYPEKEY, Type, "type")
	JKEY(ANNOTATIONSKEY, Annotations, "annotations")

	DEFINE_TYPE_JSON(Content, TYPEKEY, ANNOTATIONSKEY)
};

// TextContent {
//   MSG_DESCRIPTION : "Text provided to or from an LLM.",
//                   MSG_PROPERTIES : {
//                     MSG_ANNOTATIONS : {
//                       "$ref" : "#/definitions/Annotations",
//                       MSG_DESCRIPTION : "Optional annotations for the client."
//                     },
//                     MSG_TEXT : {
//                       MSG_DESCRIPTION : "The text content of the message.",
//                       MSG_TYPE : MSG_STRING
//                     },
//                     MSG_TYPE : {MSG_CONST : MSG_TEXT, MSG_TYPE : MSG_STRING}
//                   },
//                                  MSG_REQUIRED : [ MSG_TEXT, MSG_TYPE ],
//                                               MSG_TYPE : MSG_OBJECT
// };

// Text provided to or from an LLM.
struct TextContent : Content
{
	std::string Text; // The text content of the message.

	JKEY(TEXTKEY, Text, "text")

	DEFINE_TYPE_JSON_DERIVED(TextContent, Content, TEXTKEY)

	TextContent()
	{
		Type = "text";
		Annotations = std::nullopt;
	}
};

// ImageContent {
//   MSG_DESCRIPTION : "An image provided to or from an LLM.",
//                   MSG_PROPERTIES
//       : {
//         MSG_ANNOTATIONS : {
//           "$ref" : "#/definitions/Annotations",
//           MSG_DESCRIPTION : "Optional annotations for the client."
//         },
//         MSG_DATA : {
//           MSG_DESCRIPTION : "The base64-encoded image data.",
//           MSG_FORMAT : MSG_BYTE,
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_MIME_TYPE : {
//           MSG_DESCRIPTION : "The MIME type of the image. Different providers may "
//                           "support different image types.",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_TYPE : {MSG_CONST : "image", MSG_TYPE : MSG_STRING}
//       },
//         MSG_REQUIRED : [ MSG_DATA, MSG_MIME_TYPE, MSG_TYPE ],
//                      MSG_TYPE : MSG_OBJECT
// };

// An image provided to or from an LLM.
struct ImageContent : Content
{
	// TODO: @HalcyonOmega @format byte (base64)
	std::string Data;								 // The base64-encoded image data.
	Poco::Net::MediaType MIMEType{ "image", "png" }; // The MIME type of the image. Different
													 // providers may support different image types.

	JKEY(DATAKEY, Data, "data")
	JKEY(MIMETYPEKEY, MIMEType, "mimeType")

	DEFINE_TYPE_JSON_DERIVED(ImageContent, Content, DATAKEY, MIMETYPEKEY)

	ImageContent()
	{
		Type = "image";
		Annotations = std::nullopt;
	}
};

// AudioContent {
//   MSG_DESCRIPTION : "Audio provided to or from an LLM.",
//                   MSG_PROPERTIES
//       : {
//         MSG_ANNOTATIONS : {
//           "$ref" : "#/definitions/Annotations",
//           MSG_DESCRIPTION : "Optional annotations for the client."
//         },
//         MSG_DATA : {
//           MSG_DESCRIPTION : "The base64-encoded audio data.",
//           MSG_FORMAT : MSG_BYTE,
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_MIME_TYPE : {
//           MSG_DESCRIPTION : "The MIME type of the audio. Different providers may "
//                           "support different audio types.",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_TYPE : {MSG_CONST : "audio", MSG_TYPE : MSG_STRING}
//       },
//         MSG_REQUIRED : [ MSG_DATA, MSG_MIME_TYPE, MSG_TYPE ],
//                      MSG_TYPE : MSG_OBJECT
// };

// Audio provided to or from an LLM.
struct AudioContent : Content
{
	// TODO: @HalcyonOmega @format byte (base64)
	std::string Data;								  // The base64-encoded audio data.
	Poco::Net::MediaType MIMEType{ "audio", "mpeg" }; // The MIME type of the audio. Different
													  // providers may support different audio
													  // types.

	JKEY(DATAKEY, Data, "data")
	JKEY(MIMETYPEKEY, MIMEType, "mimeType")

	DEFINE_TYPE_JSON_DERIVED(AudioContent, Content, DATAKEY, MIMETYPEKEY)

	AudioContent()
	{
		Type = "audio";
		Annotations = std::nullopt;
	}
};

// ResourceContents {
//   MSG_DESCRIPTION : "The contents of a specific resource or sub-resource.",
//                   MSG_PROPERTIES
//       : {
//         MSG_MIME_TYPE : {
//           MSG_DESCRIPTION : "The MIME type of this resource, if known.",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_URI : {
//           MSG_DESCRIPTION : "The URI of this resource.",
//           MSG_FORMAT : MSG_URI,
//           MSG_TYPE : MSG_STRING
//         }
//       },
//         MSG_REQUIRED : [MSG_URI],
//                      MSG_TYPE : MSG_OBJECT
// };

// The contents of a specific resource or sub-resource.
struct ResourceContents
{
	MCP::URI URI;								  // The URI of this resource.
	std::optional<Poco::Net::MediaType> MIMEType; // The MIME type of this resource, if known.

	JKEY(URIKEY, URI, "uri")
	JKEY(MIMETYPEKEY, MIMEType, "mimeType")

	DEFINE_TYPE_JSON(ResourceContents, URIKEY, MIMETYPEKEY)
};

// TextResourceContents {
//   MSG_PROPERTIES : {
//     MSG_MIME_TYPE : {
//       MSG_DESCRIPTION : "The MIME type of this resource, if known.",
//       MSG_TYPE : MSG_STRING
//     },
//     MSG_TEXT : {
//       MSG_DESCRIPTION : "The text of the item. This must only be set if the
//       item "
//                       "can actually be represented as text (not binary
//                       data).",
//       MSG_TYPE : MSG_STRING
//     },
//     MSG_URI : {
//       MSG_DESCRIPTION : "The URI of this resource.",
//       MSG_FORMAT : MSG_URI,
//       MSG_TYPE : MSG_STRING
//     }
//   },
//                  MSG_REQUIRED : [ MSG_TEXT, MSG_URI ],
//                               MSG_TYPE : MSG_OBJECT
// };

// The contents of a text resource.
struct TextResourceContents : ResourceContents
{
	std::string Text; // The text of the item. This must only be set if the item can actually be
					  // represented as text (not binary data).

	JKEY(TEXTKEY, Text, "text")

	DEFINE_TYPE_JSON_DERIVED(TextResourceContents, ResourceContents, TEXTKEY)

	TextResourceContents(const std::string_view& InText, const MCP::URI& InURI)
	{
		URI = InURI;
		MIMEType = Poco::Net::MediaType{ "text", "plain" };
		MIMEType->setParameter("charset", "utf-8");
		Text = InText;
	}
};

// BlobResourceContents {
//   MSG_PROPERTIES : {
//     MSG_BLOB : {
//       MSG_DESCRIPTION :
//           "A base64-encoded string representing the binary data of the
//           item.",
//       MSG_FORMAT : MSG_BYTE,
//       MSG_TYPE : MSG_STRING
//     },
//     MSG_MIME_TYPE : {
//       MSG_DESCRIPTION : "The MIME type of this resource, if known.",
//       MSG_TYPE : MSG_STRING
//     },
//     MSG_URI : {
//       MSG_DESCRIPTION : "The URI of this resource.",
//       MSG_FORMAT : MSG_URI,
//       MSG_TYPE : MSG_STRING
//     }
//   },
//                  MSG_REQUIRED : [ MSG_BLOB, MSG_URI ],
//                               MSG_TYPE : MSG_OBJECT
// };

// The contents of a blob resource.
struct BlobResourceContents : ResourceContents
{
	// TODO: @HalcyonOmega @format byte (base64) blob
	MCP::BLOB Blob; // A base64-encoded string representing the binary data of the item.

	JKEY(BLOBKEY, Blob, "blob")

	DEFINE_TYPE_JSON_DERIVED(BlobResourceContents, ResourceContents, BLOBKEY)

	BlobResourceContents(const MCP::BLOB& InBlob, const MCP::URI& InURI)
	{
		URI = InURI;
		MIMEType = Poco::Net::MediaType{ "application", "octet-stream" };
		Blob = InBlob;
	}
};

// EmbeddedResource {
//   MSG_DESCRIPTION
//       : "The contents of a resource, embedded into a prompt or tool call "
//         "result.\n\nIt is up to the client how best to render embedded "
//         "resources "
//         "for the benefit\nof the LLM and/or the user.",
//         MSG_PROPERTIES : {
//           MSG_ANNOTATIONS : {
//             "$ref" : "#/definitions/Annotations",
//             MSG_DESCRIPTION : "Optional annotations for the client."
//           },
//           MSG_RESOURCE : {
//             "anyOf" : [
//               {"$ref" : "#/definitions/TextResourceContents"},
//               {"$ref" : "#/definitions/BlobResourceContents"}
//             ]
//           },
//           MSG_TYPE : {MSG_CONST : MSG_RESOURCE, MSG_TYPE : MSG_STRING}
//         },
//                        MSG_REQUIRED : [ MSG_RESOURCE, MSG_TYPE ],
//                                     MSG_TYPE : MSG_OBJECT
// };

// The contents of a resource, embedded into a prompt or tool call result.
struct EmbeddedResource : Content
{
	std::variant<TextResourceContents, BlobResourceContents> Resource;

	JKEY(RESOURCEKEY, Resource, "resource")

	DEFINE_TYPE_JSON_DERIVED(EmbeddedResource, Content, RESOURCEKEY)

	template <typename T>
		requires(std::is_same_v<std::decay_t<T>, TextResourceContents>
			|| std::is_same_v<std::decay_t<T>, BlobResourceContents>)
	EmbeddedResource(T&& InResource)
	{
		Type = "resource";
		Annotations = std::nullopt;
		Resource = std::forward<T>(InResource);
	}
};

MCP_NAMESPACE_END