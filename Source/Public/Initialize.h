// *   `InitializeParams`: (`ProtocolVersion`, `Capabilities`, `ClientInfo`)
// *   `InitializeResult`: (`ProtocolVersion`, `Capabilities`, `ServerInfo`, `Instructions?`)
// *   `InitializedParams`: (Parameters for the `initialized` notification. If the spec defines
// no parameters for this notification, `void` would be used directly as `ParamsType` for
// `Notification<ParamsType>` and this struct would not be needed.)