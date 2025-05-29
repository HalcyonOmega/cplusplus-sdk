#pragma once

#include "Core/Types/Common.hpp"

namespace MCP::Types {

struct Content {
  string Type;
  Passthrough Additional; // Additional properties.
};

// Text provided to or from an LLM.
struct TextContent : public Content {
  string Text; // The text content of the message.

  TextContent() {
    Type = "text";
  }
};

// An image provided to or from an LLM.
struct ImageContent : public Content {
  string Data; // TODO: Enforce base64 encoding. Zod: { type: "string", contentEncoding: "base64" }. The base64-encoded image data.
  string MIME_Type; // The MIME type of the image. Different providers may support different image types.

  ImageContent() {
    Type = "image";
  }
};

// An Audio provided to or from an LLM.
struct AudioContent : public Content {
  string Data; // TODO: Enforce base64 encoding. Zod: { type: "string", contentEncoding: "base64" }.The base64-encoded audio data.
  string MIME_Type; // The MIME type of the audio. Different providers may support different audio types.

  AudioContent() {
    Type = "audio";
  }
};

} // namespace MCP::Types