# Versioning Strategy

-   **Scheme:** Semantic Versioning (MAJOR.MINOR.PATCH)
    -   MCP itself has a `protocolVersion` (e.g., "2025-03-26") used in initialization for negotiation. Client/Server SDKs will have their own versions, but must correctly handle this protocol versioning (SDK_FullReference 2.1, 2.2).
-   **Definition of Breaking Change:** 
    -   Inability to negotiate a common `protocolVersion` as per MCP spec (SDK_FullReference 2.2).
    -   Changes to JSON-RPC message structures not compatible with the defined Schemas (SDK_FullReference 1.4). 