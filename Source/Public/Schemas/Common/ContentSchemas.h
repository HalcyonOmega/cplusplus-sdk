#pragma once

#include "CommonSchemas.h"
#include "Constants.h"
#include "Core.h"
#include "Schemas/Client/ClientSchemas.h"

MCP_NAMESPACE_BEGIN

struct Content {
    /**
     * The type of content.
     */
    string type;

    /**
     * Optional annotations for the client.
     */
    optional<Annotations> annotations;
};

// TextContent {
//   "description" : "Text provided to or from an LLM.",
//                   "properties" : {
//                     "annotations" : {
//                       "$ref" : "#/definitions/Annotations",
//                       "description" : "Optional annotations for the client."
//                     },
//                     "text" : {
//                       "description" : "The text content of the message.",
//                       "type" : "string"
//                     },
//                     "type" : {"const" : "text", "type" : "string"}
//                   },
//                                  "required" : [ "text", "type" ],
//                                               "type" : "object"
// };

/**
 * Text provided to or from an LLM.
 */
struct TextContent : public Content {
    /**
     * The text content of the message.
     */
    string text;

    TextContent() {
        type = CONST_TEXT;
    }
};

// ImageContent {
//   "description" : "An image provided to or from an LLM.",
//                   "properties"
//       : {
//         "annotations" : {
//           "$ref" : "#/definitions/Annotations",
//           "description" : "Optional annotations for the client."
//         },
//         "data" : {
//           "description" : "The base64-encoded image data.",
//           "format" : "byte",
//           "type" : "string"
//         },
//         "mimeType" : {
//           "description" : "The MIME type of the image. Different providers may "
//                           "support different image types.",
//           "type" : "string"
//         },
//         "type" : {"const" : "image", "type" : "string"}
//       },
//         "required" : [ "data", "mimeType", "type" ],
//                      "type" : "object"
// };

/**
 * An image provided to or from an LLM.
 */
struct ImageContent : public Content {
    /**
     * The base64-encoded image data.
     *
     * @format byte
     */
    string data;

    /**
     * The MIME type of the image. Different providers may support different image types.
     */
    string mimeType;

    ImageContent() {
        type = CONST_IMAGE;
    }
};

// AudioContent {
//   "description" : "Audio provided to or from an LLM.",
//                   "properties"
//       : {
//         "annotations" : {
//           "$ref" : "#/definitions/Annotations",
//           "description" : "Optional annotations for the client."
//         },
//         "data" : {
//           "description" : "The base64-encoded audio data.",
//           "format" : "byte",
//           "type" : "string"
//         },
//         "mimeType" : {
//           "description" : "The MIME type of the audio. Different providers may "
//                           "support different audio types.",
//           "type" : "string"
//         },
//         "type" : {"const" : "audio", "type" : "string"}
//       },
//         "required" : [ "data", "mimeType", "type" ],
//                      "type" : "object"
// };

/**
 * Audio provided to or from an LLM.
 */
struct AudioContent : public Content {
    /**
     * The base64-encoded audio data.
     *
     * @format byte
     */
    string data;

    /**
     * The MIME type of the audio. Different providers may support different audio types.
     */
    string mimeType;

    AudioContent() {
        type = CONST_AUDIO;
    }
};

// ResourceContents {
//   "description" : "The contents of a specific resource or sub-resource.",
//                   "properties"
//       : {
//         "mimeType" : {
//           "description" : "The MIME type of this resource, if known.",
//           "type" : "string"
//         },
//         "uri" : {
//           "description" : "The URI of this resource.",
//           "format" : "uri",
//           "type" : "string"
//         }
//       },
//         "required" : ["uri"],
//                      "type" : "object"
// };

/**
 * The contents of a specific resource or sub-resource.
 */
struct ResourceContents {
    /**
     * The URI of this resource.
     *
     * @format uri
     */
    string uri;

    /**
     * The MIME type of this resource, if known.
     */
    optional<string> mimeType;
};

// TextResourceContents {
//   "properties" : {
//     "mimeType" : {
//       "description" : "The MIME type of this resource, if known.",
//       "type" : "string"
//     },
//     "text" : {
//       "description" : "The text of the item. This must only be set if the
//       item "
//                       "can actually be represented as text (not binary
//                       data).",
//       "type" : "string"
//     },
//     "uri" : {
//       "description" : "The URI of this resource.",
//       "format" : "uri",
//       "type" : "string"
//     }
//   },
//                  "required" : [ "text", "uri" ],
//                               "type" : "object"
// };

struct TextResourceContents : public ResourceContents {
    /**
     * The text of the item. This must only be set if the item can actually be
     * represented as text (not binary data).
     */
    string text;
};

// BlobResourceContents {
//   "properties" : {
//     "blob" : {
//       "description" :
//           "A base64-encoded string representing the binary data of the
//           item.",
//       "format" : "byte",
//       "type" : "string"
//     },
//     "mimeType" : {
//       "description" : "The MIME type of this resource, if known.",
//       "type" : "string"
//     },
//     "uri" : {
//       "description" : "The URI of this resource.",
//       "format" : "uri",
//       "type" : "string"
//     }
//   },
//                  "required" : [ "blob", "uri" ],
//                               "type" : "object"
// };

struct BlobResourceContents : public ResourceContents {
    /**
     * A base64-encoded string representing the binary data of the item.
     *
     * @format byte
     */
    string blob;
};

// EmbeddedResource {
//   "description"
//       : "The contents of a resource, embedded into a prompt or tool call "
//         "result.\n\nIt is up to the client how best to render embedded "
//         "resources "
//         "for the benefit\nof the LLM and/or the user.",
//         "properties" : {
//           "annotations" : {
//             "$ref" : "#/definitions/Annotations",
//             "description" : "Optional annotations for the client."
//           },
//           "resource" : {
//             "anyOf" : [
//               {"$ref" : "#/definitions/TextResourceContents"},
//               {"$ref" : "#/definitions/BlobResourceContents"}
//             ]
//           },
//           "type" : {"const" : "resource", "type" : "string"}
//         },
//                        "required" : [ "resource", "type" ],
//                                     "type" : "object"
// };

/**
 * The contents of a resource, embedded into a prompt or tool call result.
 *
 * It is up to the client how best to render embedded resources for the benefit
 * of the LLM and/or the user.
 */
struct EmbeddedResource : public Content {
    variant<TextResourceContents, BlobResourceContents> resource;

    EmbeddedResource() {
        type = CONST_RESOURCE;
    }
};

MCP_NAMESPACE_END