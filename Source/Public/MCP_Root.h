// *   `MCP_Root`
// *   `MCP_RootsListParams_S2C`: (likely `MCP_EmptyParams` from Server) -> Result from Client:
// `MCP_RootsListResult_S2C` (`Roots`: array of `MCP_Root`) %% TODO: Clarify if S2C requests can be
// parameterless using 'void'
// *   `notifications/roots/listChanged` (Notification from Client to Server): (no parameters)
// *   `MCP_SamplingMessage`: (`Role`, `Content`)
// *   `MCP_ModelHint`
// *   `MCP_ModelPreferences`
// *   `MCP_SamplingCreateMessageParams_S2C`: (from Server, complex: `Messages`,
// `ModelPreferences?`, etc.) -> Result from Client: `MCP_SamplingCreateMessageResult_S2C` (`Role`,
// `Content`, `Model`, `StopReason?`)