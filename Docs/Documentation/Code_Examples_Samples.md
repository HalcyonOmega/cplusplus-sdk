# Code Examples & Samples

---

## Overview

Great code examples make it much easier for users to get started and understand how to use your SDK. The goal is to provide clear, minimal, and complete examples that cover common use cases and key features.

## Best Practices for Examples

- **Keep It Simple:**
  - Write minimal examples that focus on one concept at a time.
  - Avoid unnecessary complexity or dependencies.

- **Make It Complete:**
  - Each example should be copy-pasteable and runnable as-is.
  - Include all necessary includes, setup, and teardown code.

- **Use Markdown for Docs:**
  - Document examples in Markdown files for easy reading and sharing.
  - Use fenced code blocks with language tags (e.g., ```cpp) for syntax highlighting.
  - Place example docs in a dedicated folder (e.g., `Docs/Examples/`).

- **Provide Standalone Example Files:**
  - Include full example source files in an `Examples/` directory.
  - Mirror the structure of the SDK for easy navigation.
  - Reference these files from the Markdown docs.

- **Show Real Use Cases:**
  - Cover common tasks (e.g., initializing the SDK, sending a request, handling responses).
  - Include both basic and advanced examples as the SDK matures.

- **Style and Consistency:**
  - Use consistent formatting and naming conventions.
  - Comment code where it helps clarify intent.

- **Keep Examples Up to Date:**
  - Update examples whenever the API changes.
  - Test examples regularly (ideally as part of CI).

- **Make Examples Discoverable:**
  - Link to examples from the main README and API docs.
  - Use descriptive file and section names.

## Example Markdown Snippet

```cpp
#include <mcp/sdk.h>

int main() {
    mcp::Client client;
    client.initialize();
    // ... use the client ...
    return 0;
}
```

---

**TL;DR:**
- Write simple, complete, runnable examples
- Use Markdown and standalone files
- Cover common use cases
- Keep examples up to date and easy to find

# Code Examples & Sample Applications

-   **Location:** (e.g., `Docs/Examples/`, `Examples-Unported/`)
-   **Types of Examples:** 
-   **Sample Application(s) Overview:** 