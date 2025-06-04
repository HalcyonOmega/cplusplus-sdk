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
     * @param code The error code
     * @param message A short description of the error
     * @param data Optional additional error data
     */
    MCP_Error(int code, const string& message, const optional<JSON>& data = nullopt);

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
     * @param json The JSON object to parse
     * @return MCP_Error object
     */
    static MCP_Error FromJSON(const JSON& json);

  private:
    int Code;
    string Message;
    optional<JSON> Data;
};

MCP_NAMESPACE_END
