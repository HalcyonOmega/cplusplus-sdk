#pragma once

#include <Poco/Net/MediaType.h>

#include <optional>
#include <string>
#include <variant>

#include "CoreSDK/Common/Annotations.h"
#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Common/Roles.h"
#include "JSONProxy.h"
#include "URIProxy.h"

MCP_NAMESPACE_BEGIN

// TODO: @HalcyonOmega - Attempt to fix Poco::Data::BLOB instead of this
using BLOB = std::vector<char>;

struct Content {
    std::string Type;                       // The type of content.
    std::optional<Annotations> Annotations; // Optional annotations for the client.

    JKEY(TYPEKEY, Type, "type")
    JKEY(ANNOTATIONSKEY, Annotations, "annotations")

    DEFINE_TYPE_JSON(Content, TYPEKEY, ANNOTATIONSKEY)
};

// Text provided to or from an LLM.
struct TextContent : Content {
    std::string Text; // The text content of the message.

    JKEY(TEXTKEY, Text, "text")

    DEFINE_TYPE_JSON_DERIVED(TextContent, Content, TEXTKEY)

    TextContent() {
        Type = "text";
        Annotations = std::nullopt;
    }
};

// An image provided to or from an LLM.
struct ImageContent : Content {
    // TODO: @HalcyonOmega @format byte (base64)
    std::string Data;                              // The base64-encoded image data.
    Poco::Net::MediaType MIMEType{"image", "png"}; // The MIME type of the image. Different
                                                   // providers may support different image types.

    JKEY(DATAKEY, Data, "data")
    JKEY(MIMETYPEKEY, MIMEType, "mimeType")

    DEFINE_TYPE_JSON_DERIVED(ImageContent, Content, DATAKEY, MIMETYPEKEY)

    ImageContent() {
        Type = "image";
        Annotations = std::nullopt;
    }
};

// An audio provided to or from an LLM.
struct AudioContent : Content {
    // TODO: @HalcyonOmega @format byte (base64)
    std::string Data;                               // The base64-encoded audio data.
    Poco::Net::MediaType MIMEType{"audio", "mpeg"}; // The MIME type of the audio. Different
                                                    // providers may support different audio
                                                    // types.

    JKEY(DATAKEY, Data, "data")
    JKEY(MIMETYPEKEY, MIMEType, "mimeType")

    DEFINE_TYPE_JSON_DERIVED(AudioContent, Content, DATAKEY, MIMETYPEKEY)

    AudioContent() {
        Type = "audio";
        Annotations = std::nullopt;
    }
};

// The contents of a specific resource or sub-resource.
struct ResourceContents {
    MCP::URI URI;                                 // The URI of this resource.
    std::optional<Poco::Net::MediaType> MIMEType; // The MIME type of this resource, if known.

    JKEY(URIKEY, URI, "uri")
    JKEY(MIMETYPEKEY, MIMEType, "mimeType")

    DEFINE_TYPE_JSON(ResourceContents, URIKEY, MIMETYPEKEY)
};

struct TextResourceContents : ResourceContents {
    std::string Text; // The text of the item. This must only be set if the item can actually be
                      // represented as text (not binary data).

    JKEY(TEXTKEY, Text, "text")

    DEFINE_TYPE_JSON_DERIVED(TextResourceContents, ResourceContents, TEXTKEY)

    TextResourceContents(const std::string_view& InText, const MCP::URI& InURI) {
        URI = InURI;
        MIMEType = Poco::Net::MediaType{"text", "plain"};
        MIMEType->setParameter("charset", "utf-8");
        Text = InText;
    }
};

struct BlobResourceContents : ResourceContents {
    // TODO: @HalcyonOmega @format byte (base64) blob
    MCP::BLOB Blob; // A base64-encoded string representing the binary data of the item.

    JKEY(BLOBKEY, Blob, "blob")

    DEFINE_TYPE_JSON_DERIVED(BlobResourceContents, ResourceContents, BLOBKEY)

    BlobResourceContents(const MCP::BLOB& InBlob, const MCP::URI& InURI) {
        URI = InURI;
        MIMEType = Poco::Net::MediaType{"application", "octet-stream"};
        Blob = InBlob;
    }
};

// The contents of a resource, embedded into a prompt or tool call result.
struct EmbeddedResource : Content {
    std::variant<TextResourceContents, BlobResourceContents> Resource;

    JKEY(RESOURCEKEY, Resource, "resource")

    DEFINE_TYPE_JSON_DERIVED(EmbeddedResource, Content, RESOURCEKEY)

    template <typename T>
        requires(std::is_same_v<std::decay_t<T>, TextResourceContents>
                 || std::is_same_v<std::decay_t<T>, BlobResourceContents>)
    EmbeddedResource(T&& InResource) {
        Type = "resource";
        Annotations = std::nullopt;
        Resource = std::forward<T>(InResource);
    }
};

MCP_NAMESPACE_END