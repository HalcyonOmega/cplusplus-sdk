// The following is from what was originally Prompt.h in the v2 SDK

// *   `PromptArgument` *   `Prompt`: (Defines a prompt, contains `PromptArgument`s)
//         *   `PromptMessage`: (`Role`, `Content` of `ContentBase` type)
//                                  *   `PromptsGetResult`
//     : (`Description ?`, `Messages`: array of `PromptMessage`)
//     - Returned by the `Get` method.*   `PromptsClientStub` Methods
//     : *   `List(Cursor ?: std::optional<std::string>)`
//           ->Result
//     : `ListResult<Prompt>` *   `Get(Name : std::string, Arguments ?: json_object)`
//     ->Result : `PromptsGetResult` *   `notifications
//           / prompts / listChanged` (Notification) : (no parameters)

// End of original Prompt.h

#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// Describes an argument that a prompt can accept.
struct PromptArgument {
    string Name;                  // The name of the argument.
    optional<string> Description; // A human-readable description of the argument.
    optional<bool> Required;      // Whether this argument must be provided.
    Passthrough Additional;       // Additional properties.
};

// A prompt or prompt template that the server offers.
struct Prompt {
    string Name;                  // The name of the prompt or prompt template.
    optional<string> Description; // An optional description of what this prompt provides.
    optional<vector<PromptArgument>>
        Arguments;          // A list of arguments to use for templating the prompt.
    Passthrough Additional; // Additional properties.
};

// Describes a message returned as part of a prompt.
struct PromptMessage {
    z.enum(["user", "assistant"]) Role;
    variant<TextContent, ImageContent, AudioContent, EmbeddedResource, > Content;
    Passthrough Additional;
};

// Autocomplete - Identifies a prompt.
struct PromptReference {
    z.literal("ref/prompt") Type;

    string Name;            // The name of the prompt or prompt template
    Passthrough Additional; // Additional properties.
};

MCP_NAMESPACE_END