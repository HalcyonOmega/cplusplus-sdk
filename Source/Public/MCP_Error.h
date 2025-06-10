#pragma once

#include "Core/Includes/Core.h"

MCP_NAMESPACE_BEGIN

/**
 * Standard structure for error details within responses.
 * Follows JSON-RPC 2.0 error format.
 */
class MCP_Error {
  public:
    /**
     * Creates a new MCP_Error.
     * @param InCode The error code
     * @param InMessage A short description of the error
     * @param InData Optional additional error data
     */
    MCP_Error(int InCode, const string& InMessage, const optional<JSON>& InData = nullopt);

    /**
     * Gets the error code.
     * @return The error code
     */
    int GetCode() const;

    /**
     * Gets the error message.
     * @return The error message
     */
    const string& GetMessage() const;

    /**
     * Gets the optional error data.
     * @return The error data, if any
     */
    const optional<JSON>& GetData() const;

    /**
     * Converts the error to a JSON object.
     * @return JSON representation of the error
     */
    JSON ToJSON() const;

    /**
     * Creates an MCP_Error from a JSON object.
     * @param InJSON The JSON object to parse
     * @return MCP_Error object
     */
    static MCP_Error FromJSON(const JSON& InJSON);

  private:
    int m_Code;
    string m_Message;
    optional<JSON> m_Data;
};

MCP_NAMESPACE_END
