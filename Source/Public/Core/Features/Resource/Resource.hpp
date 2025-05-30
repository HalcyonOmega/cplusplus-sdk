#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// The contents of a specific resource or sub-resource.
struct ResourceContents {
    string URI;                 // The URI of this resource.
    optional<string> MIME_Type; // The MIME type of this resource, if known.
    Passthrough Additional;     // Additional properties.
};

struct TextResourceContents {
    ResourceContents;
    string Text; // The text of the item. This must only be set if the item can actually be
                 // represented as text (not binary data).
};

struct BLOB_ResourceContents {
    ResourceContents;
    string.base64() BLOB; // A base64-encoded string representing the binary data of the item.
};

// A known resource that the server is capable of reading.
struct Resource {
    string URI;  // The URI of this resource.
    string Name; // A human-readable name for this resource. This can be used by clients to populate
                 // UI elements.
    optional<string> Description; // A description of what this resource represents. This can be
                                  // used by clients to improve the LLM's understanding of available
                                  // resources. It can be thought of like a "hint" to the model.
    optional<string> MIME_Type;   // The MIME type of this resource, if known.
    Passthrough Additional;       // Additional properties.
};

// A template description for resources available on the server.
struct ResourceTemplate {
    string URI_Template; // A URI template (according to RFC 6570) that can be used to construct
                         // resource URIs.
    string Name; // A human-readable name for the type of resource this template refers to. This can
                 // be used by clients to populate UI elements.
    optional<string> Description; // A description of what this template is for. This can be used by
                                  // clients to improve the LLM's understanding of available
                                  // resources. It can be thought of like a "hint" to the model.
    optional<string>
        MIME_Type; // The MIME type for all resources that match this template. This should only be
                   // included if all resources matching this template have the same type.
    Passthrough Additional; // Additional properties.
};

// The contents of a resource, embedded into a prompt or tool call result.
struct EmbeddedResource {
    z.literal(MSG_KEY_RESOURCE) Type;
    variant<TextResourceContents, BLOB_ResourceContents> Resource;
    Passthrough Additional;
};

/* Autocomplete */
// A reference to a resource or resource template definition.
struct ResourceReference {
    z.literal("ref/resource") Type;

    string URI;             // The URI or URI template of the resource.
    Passthrough Additional; // Additional properties.
};

MCP_NAMESPACE_END