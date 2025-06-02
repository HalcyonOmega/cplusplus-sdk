#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

struct AudioContent {
  "description" : "Audio provided to or from an LLM.",
                  "properties"
      : {
        "annotations" : {
          "$ref" : "#/definitions/Annotations",
          "description" : "Optional annotations for the client."
        },
        "data" : {
          "description" : "The base64-encoded audio data.",
          "format" : "byte",
          "type" : "string"
        },
        "mimeType" : {
          "description" : "The MIME type of the audio. Different providers may "
                          "support different audio types.",
          "type" : "string"
        },
        "type" : {"const" : "audio", "type" : "string"}
      },
        "required" : [ "data", "mimeType", "type" ],
                     "type" : "object"
};

struct ImageContent {
  "description" : "An image provided to or from an LLM.",
                  "properties"
      : {
        "annotations" : {
          "$ref" : "#/definitions/Annotations",
          "description" : "Optional annotations for the client."
        },
        "data" : {
          "description" : "The base64-encoded image data.",
          "format" : "byte",
          "type" : "string"
        },
        "mimeType" : {
          "description" : "The MIME type of the image. Different providers may "
                          "support different image types.",
          "type" : "string"
        },
        "type" : {"const" : "image", "type" : "string"}
      },
        "required" : [ "data", "mimeType", "type" ],
                     "type" : "object"
};

struct TextContent {
  "description" : "Text provided to or from an LLM.",
                  "properties" : {
                    "annotations" : {
                      "$ref" : "#/definitions/Annotations",
                      "description" : "Optional annotations for the client."
                    },
                    "text" : {
                      "description" : "The text content of the message.",
                      "type" : "string"
                    },
                    "type" : {"const" : "text", "type" : "string"}
                  },
                                 "required" : [ "text", "type" ],
                                              "type" : "object"
};

MCP_NAMESPACE_END