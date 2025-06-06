# JSON Library Comparison: Glaze, Nlohmann, and JSON Struct

This document provides a comparison of three C++ JSON libraries: Glaze JSON, Nlohmann JSON, and JSON Struct. The comparison focuses on aspects relevant to selecting a library for an SDK, with an emphasis on end-user developer experience.

## Feature Comparison

| Feature                      | Glaze JSON                                     | Nlohmann JSON                                     | JSON Struct (jorgen/json_struct)              |
| ---------------------------- | ---------------------------------------------- | ------------------------------------------------- | --------------------------------------------- |
| **C++ Standard**             | C++20 (primarily for reflection)               | C++11 (vanilla)                                   | C++11                                         |
| **Dependencies**             | Header-only; no external dependencies listed   | Header-only; no external dependencies listed      | Header-only; no external dependencies listed  |
| **Installation**             | Header-only (single include or multiple files) | Header-only (single `json.hpp` or via package managers) | Header-only (copy `json_struct.h`)             |
| **Automatic Reflection**     | Yes (core feature, compile-time)               | No (requires manual `to_json`/`from_json` or macros) | Yes (via `JS_OBJ` / `JS_OBJECT` macros)       |
| **JSON RPC 2.0 Support**     | Not explicitly mentioned; performance focus    | Not explicitly mentioned; general purpose JSON    | Not explicitly mentioned; struct mapping focus |
| **Key Renaming**             | Yes (via `glz::opts` or custom mappers)        | Yes (manual in `to_json`/`from_json` or specific macros) | Yes (via `JS_MEMBER_WITH_NAME`, `JS_MEMBER_ALIASES` macros) |
| **Ease of Use (Dev Exp)**    | High for reflection; modern C++ features       | High (intuitive syntax, STL-like access)          | Medium (macro-based, requires learning specific macros) |
| **Performance**              | Very High (claims to be fastest)               | Good (not a primary design goal, but reasonable)  | Good (focus on direct struct mapping)         |
| **Error Handling**           | `glz::error_code` based, detailed error reporting | Exceptions (can be disabled), `parse_error` SAX callback | `JS::Error` enum, `ParseContext::error` member |
| **Customization**            | High (custom parsers, writers, mappers)        | High (custom serializers, `adl_serializer`)       | Medium (TypeHandlers for custom types)        |
| **Binary Formats**           | Yes (BEVE, BSON, UBJSON planned/partial)       | Yes (BSON, CBOR, MessagePack, UBJSON, BJData)     | No (focus is on JSON text to/from C++ structs) |
| **Community & Docs**         | Growing, good documentation                    | Very Large, extensive documentation, many examples | Smaller, good README, examples, and tests     |
| **Licensing**                | MIT                                            | MIT                                               | MIT                                           |
| **Notable Features**         | - High-performance parsing/serialization<br/>- Schema validation<br/>- Partial parsing<br/>- JSON Pointer | - Intuitive syntax (`_json` literal)<br/>- STL-like access<br/>- JSON Pointer & Patch<br/>- Extensive arbitrary type conversion | - Direct struct mapping with minimal boilerplate<br/>- Compile-time metadata via macros<br/>- Support for maps to handle dynamic JSON structures |

## Enhancements for a "Best of All Worlds" Library

Creating an ideal JSON library for SDK development involves balancing performance, ease of use, flexibility, and a minimal footprint for the end-user. Drawing from the strengths of Glaze, Nlohmann, and JSON Struct, a "best of all worlds" library could incorporate the following enhancements, ensuring theoretical compatibility and a superior developer experience:

### Unified Reflection and Serialization Model
The library should aim for a reflection mechanism that is as performant and compile-time oriented as Glaze's, but with the option for more explicit, macro-based control similar to JSON Struct for cases where full C++20 reflection is not available or desired. This allows developers to choose between convention-over-configuration (via reflection) and explicit mapping (via macros or Nlohmann-style `to_json`/`from_json` functions). This caters to different C++ standards and project preferences without sacrificing performance for the common case.

### Seamless Integration and Developer Ergonomics
The library should maintain a header-only option for trivial integration, akin to all three libraries. However, it should also provide robust CMake support and integration with popular package managers, similar to Nlohmann JSON, for larger projects. The syntax should strive for Nlohmann's intuitiveness, allowing for natural C++ data structure interaction (e.g., `j["key"] = value;`) while also providing powerful, type-safe mapping to structs as seen in Glaze and JSON Struct. Error reporting should be comprehensive, offering both exception-based (like Nlohmann) and error-code based (like Glaze) mechanisms, configurable by the user.

### Advanced Feature Set with Modularity
Support for JSON Pointer, JSON Patch, and various binary formats (BSON, CBOR, MessagePack) as seen in Nlohmann JSON is crucial for a versatile SDK. Performance should be a key design goal, aiming for Glaze-like speeds for common operations. However, features should be somewhat modular, allowing users to compile out support for binary formats or advanced features they don't need, to minimize binary size and compile times, a concern for SDKs. Automatic key renaming should be flexible, supporting both compile-time options (like Glaze `opts`) and runtime/macro-based approaches (like JSON Struct). Native support or clear patterns for JSON RPC 2.0 message structures would be a significant boon for SDKs focused on communication.

### Compile-Time Configuration and Safety
The library should leverage C++'s type system and compile-time capabilities to catch as many errors as possible before runtime. This includes options for schema validation (inspired by Glaze) that can be enforced at compile time or parse time. Options for customizing memory allocation and handling of non-standard JSON (like comments or trailing commas, as Nlohmann allows with flags) should be provided, but with safe defaults.

### Focus on End-User SDK Developer Experience
The documentation must be exceptionally clear, with numerous practical examples targeted at SDK developers. This includes guides on integrating the library into an SDK, best practices for exposing JSON-based APIs, and managing data contracts between the SDK and its consumers. Performance benchmarks should be transparent and reproducible. The learning curve should be gentle for common use cases, with advanced features being opt-in and well-documented.

By combining the high-performance reflection of Glaze, the user-friendly syntax and feature richness of Nlohmann JSON, and the straightforward struct-mapping approach of JSON Struct, while ensuring configurability and a focus on the SDK developer, a truly compelling JSON library could be created. Theoretical compatibility can be maintained by ensuring core functionality relies on C++11/14, with C++20 features like advanced reflection being an optional enhancement.
