//     *   `ping` (Request): (no parameters) -> Result: `void`
//     *   `MCP_ProgressMeta`: (Part of other request params: `_Meta {
// ProgressToken }`)
//     *   `MCP_NotificationsProgressParams`: (`ProgressToken`, `Progress`, `Total?`, `Message?`)
//     *   `MCP_NotificationsCancelledParams`: (`RequestID`, `Reason?`)
//     *   `MCP_LoggingSetLevelParams`: (`Level`) -> Result: `void`
//     *   `MCP_NotificationsMessageParams_S2C`: (`Level`, `Logger?`, `Data`)
//     *   `MCP_PromptReference`
//     *   `MCP_ResourceReference`
//     *   `MCP_CompletionArgument`
//     *   `MCP_CompletionCompleteParams`: (`Ref`, `Argument`)
//     *   `MCP_CompletionValue`: (`Values`, `Total?`, `HasMore?`)
//     *   `MCP_CompletionCompleteResult`: (`Completion`: `MCP_CompletionValue`)