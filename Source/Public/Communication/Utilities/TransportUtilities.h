#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

/**
 * Validates a JSON-RPC message format
 * @param message The message to validate
 * @return true if the message is valid JSON-RPC format
 */
bool IsValidJSONRPC(const string& message);

/**
 * Validates UTF-8 encoding
 * @param message The message to validate
 * @return true if the message is valid UTF-8
 */
bool IsValidUTF8(const string& message);

/**
 * Validates protocol version
 * @param version The version string to validate
 * @return true if the version is supported
 */
bool IsValidProtocolVersion(const string& version);

/**
 * Validates transport state
 * @param isRunning Current running state
 * @param isConnected Current connection state
 * @return true if the state is valid for the requested operation
 */
bool IsValidState(bool isRunning, bool isConnected);

// TODO: @HalcyonOmega Begin Direct Translated Code
// SSE event writing
// TODO: @HalcyonOmega Should this be in the base class?
virtual void WriteSSEEvent(const string& InEvent, const string& InData) = 0;

// Helpers
void ParseSSEData(const string& InData) {
    size_t Pos = 0;
    string CurrentEvent;
    string CurrentData;

    while (Pos < InData.length()) {
        size_t LineEnd = InData.find('\n', Pos);
        if (LineEnd == string::npos) { break; }

        string Line = InData.substr(Pos, LineEnd - Pos);
        Pos = LineEnd + 1;

        if (Line.empty() || Line[0] == ':') { continue; }

        if (Line.substr(0, TSPT_EVENT_PREFIX_LEN) == TSPT_EVENT_PREFIX) {
            CurrentEvent = Line.substr(TSPT_EVENT_PREFIX_LEN);
        } else if (Line.substr(0, TSPT_EVENT_DATA_PREFIX_LEN) == TSPT_EVENT_DATA_PREFIX) {
            CurrentData = Line.substr(TSPT_EVENT_DATA_PREFIX_LEN);
        } else if (Line.empty() && !CurrentData.empty()) {
            // TODO: @HalcyonOmega Create a MessageBase from the SSE data
            MessageBase Message;
            CallOnMessage(Message);
            CurrentEvent.clear();
            CurrentData.clear();
        }
    }
}
// TODO: @HalcyonOmega End Direct Translated Code

MCP_NAMESPACE_END