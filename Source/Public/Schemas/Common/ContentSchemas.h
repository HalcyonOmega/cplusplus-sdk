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

/**
 * Text provided to or from an LLM.
 */
struct TextContent : public Content {
    /**
     * The text content of the message.
     */
    string text;

    TextContent() {
        type = MSG_TEXT;
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

struct TextResourceContents : public ResourceContents {
    /**
     * The text of the item. This must only be set if the item can actually be
     * represented as text (not binary data).
     */
    string text;
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

struct BlobResourceContents : public ResourceContents {
    /**
     * A base64-encoded string representing the binary data of the item.
     *
     * @format byte
     */
    string blob;
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

/**
 * The contents of a resource, embedded into a prompt or tool call result.
 *
 * It is up to the client how best to render embedded resources for the benefit
 * of the LLM and/or the user.
 */
struct EmbeddedResource : public Content {
    variant<TextResourceContents, BlobResourceContents> resource;

    EmbeddedResource() {
        type = MSG_RESOURCE;
    }
};

MCP_NAMESPACE_END