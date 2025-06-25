#pragma once

#include "CoreSDK/Common/Annotations.h"
#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"
#include "Poco/Net/MediaType.h"
#include "URIProxy.h"

MCP_NAMESPACE_BEGIN

// A known resource that the server is capable of reading.
struct Resource {
    MCP::URI URI;     // The URI of this resource.
    std::string Name; // A human-readable name for this resource. This can be used by clients to
                      // populate UI elements.
    std::optional<std::string>
        Description; // A description of what this resource represents. This can be
                     // used by clients to improve the LLM's understanding of available
                     // resources. It can be thought of like a "hint" to the model.
    std::optional<Poco::Net::MediaType> MIMEType; // The MIME type of this resource, if known.
    std::optional<Annotations> Annotations;       // Optional annotations for the client.
    std::optional<int64_t>
        Size; // The size of the raw resource content, in bytes (i.e., before base64
              // encoding or any tokenization), if known. This can be used by Hosts
              // to display file sizes and estimate context window usage.

    JKEY(URIKEY, URI, "uri")
    JKEY(NAMEKEY, Name, "name")
    JKEY(DESCRIPTIONKEY, Description, "description")
    JKEY(MIMETYPEKEY, MIMEType, "mimeType")
    JKEY(ANNOTATIONSKEY, Annotations, "annotations")
    JKEY(SIZEKEY, Size, "size")

    DEFINE_TYPE_JSON(Resource, URIKEY, NAMEKEY, DESCRIPTIONKEY, MIMETYPEKEY, ANNOTATIONSKEY,
                     SIZEKEY)
};

// A template description for resources available on the server.
struct ResourceTemplate {
    MCP::URITemplate URITemplate; // A URI template (according to RFC 6570) that can be used to
                                  // construct resource URIs.
    std::string Name; // A human-readable name for the type of resource this template refers to.
                      // This can be used by clients to populate UI elements.
    std::optional<std::string> Description; // A description of what this template is for. This
                                            // can be used by clients to improve the LLM's
                                            // understanding of available resources. It can be
                                            // thought of like a "hint" to the model.
    std::optional<Poco::Net::MediaType>
        MIMEType; // The MIME type for all resources that match this template. This should only
                  // be included if all resources matching this template have the same type.
    std::optional<Annotations> Annotations; // Optional annotations for the client.

    JKEY(URITEMPLATEKEY, URITemplate, "uriTemplate")
    JKEY(NAMEKEY, Name, "name")
    JKEY(DESCRIPTIONKEY, Description, "description")
    JKEY(MIMETYPEKEY, MIMEType, "mimeType")
    JKEY(ANNOTATIONSKEY, Annotations, "annotations")

    DEFINE_TYPE_JSON(ResourceTemplate, URITEMPLATEKEY, NAMEKEY, DESCRIPTIONKEY, MIMETYPEKEY,
                     ANNOTATIONSKEY)
};

template <typename T>
concept IsResource = requires(T Type) {
    { Type.URI } -> std::same_as<MCP::URI>;
    { Type.Name } -> std::convertible_to<std::string>;
    { Type.Description } -> std::same_as<std::optional<std::string>>;
    { Type.MIMEType } -> std::same_as<std::optional<Poco::Net::MediaType>>;
    { Type.Annotations } -> std::same_as<std::optional<Annotations>>;
    { Type.Size } -> std::same_as<std::optional<int64_t>>;
};

template <typename T>
concept IsResourceTemplate = requires(T Type) {
    { Type.URITemplate } -> std::same_as<MCP::URITemplate>;
    { Type.Name } -> std::convertible_to<std::string>;
    { Type.Description } -> std::same_as<std::optional<std::string>>;
    { Type.MIMEType } -> std::same_as<std::optional<Poco::Net::MediaType>>;
    { Type.Annotations } -> std::same_as<std::optional<Annotations>>;
};

MCP_NAMESPACE_END