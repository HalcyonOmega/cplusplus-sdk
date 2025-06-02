// *   `MCP_PromptArgument` *   `MCP_Prompt`: (Defines a prompt, contains `MCP_PromptArgument`s)
//         *   `MCP_PromptMessage`: (`Role`, `Content` of `MCP_ContentBase` type)
//                                  *   `MCP_PromptsGetResult`
//     : (`Description ?`, `Messages`: array of `MCP_PromptMessage`)
//     - Returned by the `Get` method.*   `PromptsClientStub` Methods
//     : *   `List(Cursor ?: std::optional<std::string>)`
//           ->Result
//     : `MCP_ListResult<MCP_Prompt>` *   `Get(Name : std::string, Arguments ?: json_object)`
//     ->Result : `MCP_PromptsGetResult` *   `notifications
//           / prompts / listChanged` (Notification) : (no parameters)