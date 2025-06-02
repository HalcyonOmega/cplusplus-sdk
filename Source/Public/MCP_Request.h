// *   `MCP_RequestBase`: Abstract base for requests (contains `ID`, `Method`).
// *   `MCP_Request<ParamsType>`: Template class for specific requests.
//     *   Inherits from `MCP_RequestBase`.
//     *   Holds a `ParamsType Params;
// ` member.* The `Method` string is associated with the `ParamsType`.