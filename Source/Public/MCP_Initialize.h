// *   `MCP_InitializeParams`: (`ProtocolVersion`, `Capabilities`, `ClientInfo`)
// *   `MCP_InitializeResult`: (`ProtocolVersion`, `Capabilities`, `ServerInfo`, `Instructions?`)
// *   `MCP_InitializedParams`: (Parameters for the `initialized` notification. If the spec defines
// no parameters for this notification, `void` would be used directly as `ParamsType` for
// `MCP_Notification<ParamsType>` and this struct would not be needed.)