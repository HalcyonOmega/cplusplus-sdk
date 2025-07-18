# Error Handling and Logging Strategy

-   **Error Reporting:** (e.g., Exceptions, Error Codes)
    -   JSON-RPC 2.0 `error` object structure: `{ code: number, message: string, data?: unknown }` (SDK_FullReference 1.2).
    -   Timeouts for sent requests: `notifications/cancelled` should be issued (SDK_FullReference 2.6).
    -   Tool execution errors reported in `CallToolResult` (`isError: true`), protocol errors as JSON-RPC errors (SDK_FullReference 5.3.2).
-   **Logging Framework:** 
    -   Server-to-client logging via `notifications/message` if `logging` capability declared by server (SDK_FullReference 7.4).
    -   Log levels follow syslog severity (SDK_FullReference 7.4).
    -   Client can set log level via `logging/setLevel` (SDK_FullReference 7.4).
    -   STDIO transport: Server MAY use `stderr` for UTF-8 log strings (SDK_FullReference 3.1). 