#pragma once

#include "Constants.h"
#include "Core.h"

MCP_NAMESPACE_BEGIN

struct Content {
    string Type;
    Passthrough Additional; // Additional properties.
};

// Text provided to or from an LLM.
struct TextContent : public Content {
    string Text; // The text content of the message.

    TextContent() {
        Type = MSG_TEXT;
    }
};

// An image provided to or from an LLM.
struct ImageContent : public Content {
    string
        Data; // TODO: Enforce base64 encoding. Zod: { type: MSG_STRING, contentEncoding: "base64"
              // }. The base64-encoded image data.
    string MIME_Type; // The MIME type of the image. Different providers may support different image
                      // types.

    ImageContent() {
        Type = CONST_IMAGE;
    }
};

// An Audio provided to or from an LLM.
struct AudioContent : public Content {
    string
        Data; // TODO: Enforce base64 encoding. Zod: { type: MSG_STRING, contentEncoding: "base64"
              // }.The base64-encoded audio data.
    string MIME_Type; // The MIME type of the audio. Different providers may support different audio
                      // types.

    AudioContent() {
        Type = CONST_AUDIO;
    }
};

MCP_NAMESPACE_END