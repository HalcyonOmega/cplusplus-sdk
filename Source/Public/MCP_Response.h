// *   `MCP_ResponseBase`: Abstract base for responses (contains `ID`, `Error?`).
// *   `MCP_Response<ResultType>`: Template class for specific responses.
//     *   Inherits from `MCP_ResponseBase`.
//     *   Holds an `std::optional<ResultType> Result;`
//     *   A specialization `MCP_Response<void>` handles empty successful results (i.e., `result:
//     {}`), replacing the need for a separate `MCP_EmptyResult` type.
// *   `MCP_ErrorObject`: Standard structure for error details within responses.