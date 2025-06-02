// *   `MCP_JSONSchemaObject` *   `MCP_ToolAnnotations` *   `MCP_Tool`
//     : (Defines a tool, contains `MCP_JSONSchemaObject`, `MCP_ToolAnnotations`)
//     *   `ToolsClientStub` Methods : *   `List(Cursor ?: std::optional<std::string>)`
//                                         ->Result
//     : `MCP_ListResult<MCP_Tool>` *   `Call(Name : std::string, Arguments ?: object)` ->Result
//     : `struct {
//     Content : std::vector<MCP_ContentBase>;
//     IsError ?: bool;
// }` (This structure is returned directly) *   `notifications / tools
//     / listChanged` (Notification) : (no parameters)