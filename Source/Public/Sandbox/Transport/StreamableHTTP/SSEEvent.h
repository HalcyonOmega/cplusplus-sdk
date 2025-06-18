#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// SSE Event structure for Server-Sent Events
struct SSEEvent {
    string ID;
    string Type;
    string Data;
    optional<int> Retry;

    SSEEvent() = default;
    SSEEvent(const string& InData) : Data(InData) {}
};

MCP_NAMESPACE_END