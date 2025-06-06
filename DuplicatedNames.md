# Duplicated Entity Names in Source/Public

This document lists C++ entity names (classes, structs, enums) that appear multiple times in the `Source/Public` directory, after normalizing the names (lowercase, no underscores). Line numbers are approximate.

---

1.  **Normalized Name:** `completable`
    *   **Original Name:** `Completable`
        *   Type: `template class`
        *   File: `Source/Public/Utilities/Autocomplete/Completable.h` (line ~11)
    *   **Original Name:** `Completable`
        *   Type: `template class`
        *   File: `Source/Public/Utilities/Autocomplete/Completable.hpp` (line ~12)

2.  **Normalized Name:** `content`
    *   **Original Name:** `Content`
        *   Type: `struct` (assumed, from `Source/Public/Content.h` if defined)
        *   File: `Source/Public/Content.h` (line ~N/A, potentially defined)
    *   **Original Name:** `Content`
        *   Type: `struct`
        *   File: `Source/Public/Core/Types__DT/Content.hpp` (line ~16)

3.  **Normalized Name:** `error`
    *   **Original Name:** `MCP_Error`
        *   Type: `class`
        *   File: `Source/Public/MCP_Error.h` (line ~10)
    *   **Original Name:** `Error`
        *   Type: `struct`
        *   File: `Source/Public/Core/Types__DT/Error.hpp` (line ~16)
    *   **Original Name:** `JSONRPCError` (Normalized: `jsonrpcerror`, distinct but related)
        *   Type: `struct`
        *   File: `Source/Public/Schemas/Common/JSON_RPC_Schemas.h` (line ~18)

4.  **Normalized Name:** `notification`
    *   **Original Name:** `Notification`
        *   Type: `template class`
        *   File: `Source/Public/Notification.h` (line ~21)
    *   **Original Name:** `Notification`
        *   Type: `struct`
        *   File: `Source/Public/Core/Types__DT/Notification.hpp` (line ~18)
    *   **Original Name:** `JSONRPCNotification` (Normalized: `jsonrpcnotification`, distinct but related)
        *   Type: `struct`
        *   File: `Source/Public/Schemas/Common/JSON_RPC_Schemas.h` (line ~107)

5.  **Normalized Name:** `prompt`
    *   **Original Name:** `Prompt`
        *   Type: `struct` (assumed, from `Source/Public/Prompt.h` if defined)
        *   File: `Source/Public/Prompt.h` (line ~N/A, potentially defined)
    *   **Original Name:** `Prompt`
        *   Type: `struct`
        *   File: `Source/Public/Core/Features__DT/Prompt/Prompt.hpp` (line ~16)
    *   **Original Name:** `Prompt`
        *   Type: `struct`
        *   File: `Source/Public/Schemas/Common/PromptSchemas.h` (line ~21)

6.  **Normalized Name:** `request`
    *   **Original Name:** `Request`
        *   Type: `template class`
        *   File: `Source/Public/Request.h` (line ~14)
    *   **Original Name:** `Request`
        *   Type: `struct`
        *   File: `Source/Public/Core/Types__DT/Request.hpp` (line ~27)
    *   **Original Name:** `JSONRPCRequest` (Normalized: `jsonrpcrequest`, distinct but related)
        *   Type: `struct`
        *   File: `Source/Public/Schemas/Common/JSON_RPC_Schemas.h` (line ~43)

7.  **Normalized Name:** `resource`
    *   **Original Name:** `Resource`
        *   Type: `struct` (assumed, from `Source/Public/Resource.h` if defined, though file is mostly comments)
        *   File: `Source/Public/Resource.h` (line ~N/A)
    *   **Original Name:** `Resource`
        *   Type: `struct`
        *   File: `Source/Public/Core/Features__DT/Resource/Resource.hpp` (line ~18)
    *   **Original Name:** `Resource`
        *   Type: `struct`
        *   File: `Source/Public/Schemas/Common/ResourceSchemas.h` (line ~30)

8.  **Normalized Name:** `response`
    *   **Original Name:** `Response`
        *   Type: `template class`
        *   File: `Source/Public/Response.h` (line ~15)
    *   **Original Name:** `Response` (Template specialization for `void`)
        *   Type: `template class`
        *   File: `Source/Public/Response.h` (line ~19)
    *   **Original Name:** `JSONRPCResponse` (Normalized: `jsonrpcresponse`, distinct but related)
        *   Type: `struct`
        *   File: `Source/Public/Schemas/Common/JSON_RPC_Schemas.h` (line ~73)


9.  **Normalized Name:** `root`
    *   **Original Name:** `Root`
        *   Type: `struct` (assumed, from `Source/Public/Root.h` if defined, though file is mostly comments)
        *   File: `Source/Public/Root.h` (line ~N/A)
    *   **Original Name:** `Root`
        *   Type: `struct`
        *   File: `Source/Public/Core/Features__DT/Root/Root.hpp` (line ~14)
    *   **Original Name:** `Root`
        *   Type: `struct`
        *   File: `Source/Public/Schemas/Common/RootsSchemas.h` (line ~19)

10. **Normalized Name:** `tool`
    *   **Original Name:** `Tool`
        *   Type: `struct` (assumed, from `Source/Public/Tool.h` if defined, though file is mostly comments)
        *   File: `Source/Public/Tool.h` (line ~N/A)
    *   **Original Name:** `Tool`
        *   Type: `struct`
        *   File: `Source/Public/Core/Features__DT/Tool/Tool.hpp` (line ~17)
    *   **Original Name:** `Tool`
        *   Type: `struct`
        *   File: `Source/Public/Schemas/Common/ToolSchemas.h` (line ~28)

11. **Normalized Name:** `transport`
    *   **Original Name:** `Transport`
        *   Type: `class`
        *   File: `Source/Public/Communication/Transport/Transport.h` (line ~14)
    *   **Original Name:** `Transport`
        *   Type: `class`
        *   File: `Source/Public/Communication/Transport__DT/Transport.h` (line ~21)

---
*Note: This list is based on a scan of type definitions. Files listed with "N/A" for line number and "assumed" for type (e.g., `Source/Public/Content.h`) are top-level includes that might define these types or are primarily comment blocks based on initial assessment; a deeper dive into their direct content or includes would confirm if they contribute a distinct definition.* 