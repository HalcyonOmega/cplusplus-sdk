# API Reference Documentation

---

## Overview

Good API documentation makes your SDK easy to use and understand. The goal is to clearly document all public classes, functions, types, and behaviors so users can quickly find what they need.

## Documentation Approach

- **Markdown First:**
  - Use Markdown files for high-level overviews, guides, and reference docs.
  - Organize docs by topic or module (e.g., `Docs/Reference/`, `Docs/Examples/`).
  - Write clear, concise explanations and include code snippets where helpful.

- **In-Source Comments:**
  - Document public classes, functions, and types directly in the code with clear comments.
  - Use a consistent style (brief summary, parameter descriptions, return values, etc.).
  - Example:
    ```cpp
    // Sends a JSON-RPC request to the server.
    // @param method The method name.
    // @param params The request parameters as JSON.
    // @return The server's response as JSON.
    nlohmann::json sendRequest(const std::string& method, const nlohmann::json& params);
    ```

- **(Optional) Doc Generation Tools:**
  - For large projects, consider using a tool like Doxygen to generate HTML or Markdown docs from in-source comments.
  - You can configure Doxygen to output Markdown, which you can then include in your docs folder.
  - This is optional—Markdown docs alone are fine for smaller projects.

## What to Document

- All public classes, structs, enums, and typedefs
- All public functions and methods (with parameters and return values)
- Key data structures and their usage
- Error handling and exceptions
- Example usage for important APIs

## Organization

- Keep Markdown reference docs in a dedicated folder (e.g., `Docs/Reference/`).
- Use one file per major module or topic for easy navigation.
- Link between docs for related topics (e.g., "See also: Error Handling").

## Keeping Docs Up to Date

- Update docs whenever you change the public API.
- Review and improve docs as part of code reviews and releases.
- Encourage users (and future collaborators) to report unclear or missing documentation.

---

**TL;DR:**
- Use Markdown for high-level and reference docs
- Document public APIs with clear in-source comments
- (Optionally) generate docs with Doxygen
- Keep docs organized and up to date

-   **Tool:** (e.g., Doxygen)
-   **Generation Process:** 
-   **Hosting:** 