// *   `Root`
// *   `RootsListParams_S2C`: (likely `EmptyParams` from Server) -> Result from Client:
// `RootsListResult_S2C` (`Roots`: array of `Root`) %% TODO: Clarify if S2C requests can be
// parameterless using 'void'
// *   `notifications/roots/listChanged` (Notification from Client to Server): (no parameters)
// *   `SamplingMessage`: (`Role`, `Content`)
// *   `ModelHint`
// *   `ModelPreferences`
// *   `SamplingCreateMessageParams_S2C`: (from Server, complex: `Messages`,
// `ModelPreferences?`, etc.) -> Result from Client: `SamplingCreateMessageResult_S2C` (`Role`,
// `Content`, `Model`, `StopReason?`)