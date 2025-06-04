// *   `JSONSchemaObject` *   `ToolAnnotations` *   `Tool`
//     : (Defines a tool, contains `JSONSchemaObject`, `ToolAnnotations`)
//     *   `ToolsClientStub` Methods : *   `List(Cursor ?: std::optional<std::string>)`
//                                         ->Result
//     : `ListResult<Tool>` *   `Call(Name : std::string, Arguments ?: object)` ->Result
//     : `struct {
//     Content : std::vector<ContentBase>;
//     IsError ?: bool;
// }` (This structure is returned directly) *   `notifications / tools
//     / listChanged` (Notification) : (no parameters)