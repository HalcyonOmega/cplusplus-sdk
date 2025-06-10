// The following is from what was originally Root.h in the v2 SDK

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

// End of original Root.h

#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// Represents a root directory or file that the server can operate on.
struct Root {
    // The URI identifying the root. This *must* start with file:// for now.
    string.startsWith("file://") URI;

    // An optional name for the root.
    optional<string> Name;
    Passthrough Additional;
};

MCP_NAMESPACE_END