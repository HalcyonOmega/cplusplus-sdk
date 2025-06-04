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

#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

MCP_NAMESPACE_END