#pragma once

#include "ClientSchemas.h"
#include "CommonSchemas.h"
#include "Constants.h"
#include "Core.h"

MCP_NAMESPACE_BEGIN

// struct AudioContent {
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

// struct ImageContent {
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

// struct TextContent {
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