# JSON Library Comparison: Glaze, Nlohmann, and JSON Struct

This document provides a comparison of three C++ JSON libraries: Glaze JSON, Nlohmann JSON, and JSON Struct. The comparison focuses on aspects relevant to selecting a library for an SDK, with an emphasis on end-user developer experience.

## Feature Comparison Summary

| Feature                  | Glaze JSON                               | Nlohmann JSON                                  | JSON Struct (jorgen/json_struct)            |
| ------------------------ | ---------------------------------------- | ---------------------------------------------- | ------------------------------------------- |
| **C++ Standard**         | C++23                                    | C++11                                          | C++11                                       |
| **Dependencies**         | Header-only                              | Header-only                                    | Header-only                                 |
| **Installation**         | Header-only (single/multiple files)      | Header-only (single file or pkg manager)       | Header-only (single file)                   |
| **Automatic Reflection** | Yes (compile-time)                       | No (manual or macros)                          | Yes (macros: `JS_OBJ`/`JS_OBJECT`)          |
| **JSON RPC 2.0 Support** | Yes (explicit support, extensions)       | No (general purpose, adaptable)                | No (struct mapping focus, adaptable)      |
| **Key Renaming**         | Yes (`glz::opts`, custom mappers)        | Yes (manual, macros)                           | Yes (macros: `JS_MEMBER_WITH_NAME`, etc.)   |
| **Dev Experience**       | High (modern C++, reflection)            | High (intuitive, STL-like)                     | Medium (macro-based)                        |
| **Performance**          | Very High (fastest claim)                | Good (not primary goal)                        | Good (direct struct mapping)                |
| **Error Handling**       | `glz::error_code`                        | Exceptions / SAX callback                      | `JS::Error` enum                            |
| **Customization**        | High (parsers, writers, mappers)         | High (serializers, `adl_serializer`)           | Medium (TypeHandlers)                       |
| **Binary Formats**       | BEVE, (BSON, UBJSON planned/partial)     | BSON, CBOR, MsgPack, UBJSON, BJData            | No (JSON text focus)                        |
| **Notable Features**     | Perf, Reflection, Schema, JSON Pointer   | Syntax, STL-access, Pointer/Patch, Arbitrary Types | Struct mapping, Compile-time meta, Dyn. maps |
| **Licensing**            | MIT                                      | MIT                                            | MIT                                         |

## Detailed Library Breakdown

### Glaze JSON

*   **C++ Standard**: Requires C++23, leveraging modern C++ features for its high-performance reflection and serialization capabilities.
*   **Dependencies**: It is a header-only library, simplifying integration by not imposing external library dependencies.
*   **Installation**: Can be integrated by including its headers, available as either a single include or multiple files for more granular control. Also supports FetchContent, Conan, and other package managers.
*   **Automatic Reflection**: A core strength, providing powerful compile-time reflection that automatically maps C++ structs to JSON without requiring manual metadata or macros for aggregate initializable types. Optional explicit metadata can be provided for more control or for non-aggregate types.
*   **JSON RPC 2.0 Support**: Glaze explicitly mentions JSON-RPC 2.0 support and provides it as part of its extensions, making it suitable for applications requiring this protocol.
*   **Key Renaming**: Flexible key renaming is supported through `glz::opts` during serialization/deserialization or by defining custom mappers in the metadata.
*   **Developer Experience**: Generally high, especially for developers comfortable with modern C++ (C++23). The automatic reflection significantly reduces boilerplate.
*   **Performance**: Claims to be one of the fastest C++ JSON libraries, focusing on direct memory access and compile-time operations to achieve high throughput for reading and writing.
*   **Error Handling**: Uses a `glz::error_code` system, often returned within a `glz::expected` (similar to `std::expected`). Provides detailed error messages with `glz::format_error`. Compiles with `-fno-exceptions`.
*   **Customization**: Highly customizable. Users can define custom parsers, writers, and mappers. `glz::custom` allows registering custom functions for specific members.
*   **Binary Formats**: Primarily supports its own binary format (BEVE - Binary Efficient Versatile Encoding). Documentation indicates planned or partial support for BSON and UBJSON.
*   **Community & Docs**: Growing community with good quality documentation, including a dedicated documentation page and examples.
*   **Licensing**: MIT License.
*   **Notable Features**:
    *   Extremely high-performance parsing and serialization.
    *   Advanced compile-time reflection.
    *   JSON Schema generation.
    *   JSON Pointer support.
    *   Partial read/write capabilities.
    *   Support for comments (JSONC).

### Nlohmann JSON

*   **C++ Standard**: Compatible with C++11 and later, making it broadly usable across many projects and compiler versions.
*   **Dependencies**: Header-only, requiring no external library dependencies.
*   **Installation**: Typically used by including the single `json.hpp` header file. Also widely available through various package managers (CMake `find_package`, Conan, vcpkg, etc.).
*   **Automatic Reflection**: Does not offer automatic reflection in the same way as Glaze. Serialization/deserialization for custom types requires defining `to_json` and `from_json` functions or using provided macros (`NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE`, etc.).
*   **JSON RPC 2.0 Support**: No explicit, built-in support for JSON RPC 2.0. However, its flexibility allows users to easily construct and parse JSON RPC messages.
*   **Key Renaming**: Achieved manually within custom `to_json`/`from_json` functions or through the functionality of specific macros that allow mapping struct members to different JSON keys.
*   **Developer Experience**: Very high due to its intuitive, Python-like syntax (e.g., `_json` literal, `j["key"]`) and STL-like access patterns for JSON objects and arrays.
*   **Performance**: Performance is generally good for most applications, though it's not the primary design goal compared to speed-focused libraries like Glaze. Focus is more on ease of use and features.
*   **Error Handling**: Primarily uses C++ exceptions for error reporting (e.g., `json::parse_error`). Exceptions can be disabled, and a SAX interface offers a `parse_error` callback.
*   **Customization**: Highly customizable through `adl_serializer` for arbitrary type conversions and the ability to define custom behavior for types.
*   **Binary Formats**: Extensive support for multiple binary formats including BSON, CBOR, MessagePack, UBJSON, and BJData.
*   **Community & Docs**: Very large and active community. Documentation is extensive, with a comprehensive README, a dedicated documentation website, and numerous examples.
*   **Licensing**: MIT License.
*   **Notable Features**:
    *   Intuitive and easy-to-use syntax.
    *   STL-like access and iterators.
    *   JSON Pointer and JSON Patch (RFC 6902) support.
    *   JSON Merge Patch (RFC 7386) support.
    *   Implicit conversions and extensive support for arbitrary type conversions.
    *   Support for comments and trailing commas via parser flags.

### JSON Struct (jorgen/json_struct)

*   **C++ Standard**: Requires C++11, making it compatible with a wide range of compilers and older codebases.
*   **Dependencies**: A single-header, header-only library with no external dependencies.
*   **Installation**: Integration is done by copying the `json_struct.h` file into the project's include path.
*   **Automatic Reflection**: Provides a form of automatic struct mapping using macros like `JS_OBJ` (intrusive) or `JS_OBJ_EXT` (non-intrusive). These macros generate the necessary metadata at compile time.
*   **JSON RPC 2.0 Support**: No explicit support. Focused on direct serialization/deserialization of C++ structs to/from JSON objects. Users would need to manually structure RPC messages.
*   **Key Renaming**: Supported via macros like `JS_MEMBER_WITH_NAME` (to specify a different JSON key for a member) and `JS_MEMBER_ALIASES` (to provide alternative keys).
*   **Developer Experience**: Medium. While it simplifies struct mapping, developers need to learn and correctly apply its specific set of macros. It's less "automatic" than pure reflection and less syntactically fluid than Nlohmann for generic JSON manipulation.
*   **Performance**: Good performance for its targeted use case of direct struct-to-JSON mapping, as it avoids much of the overhead of more dynamic libraries.
*   **Error Handling**: Uses an error enum (`JS::Error`) and a `JS::ParseContext` object that holds error information (e.g., `context.error` and `context.makeErrorString()`).
*   **Customization**: Offers customization for types that don't fit the standard object mapping via `JS::TypeHandler` template specializations, allowing users to define custom serialization/deserialization logic.
*   **Binary Formats**: Does not support binary JSON formats; its focus is on JSON text serialization and deserialization directly to and from C++ structs.
*   **Community & Docs**: Smaller community compared to Nlohmann or Glaze. The primary documentation is the README on GitHub, which is comprehensive and includes examples. Unit tests also serve as documentation.
*   **Licensing**: MIT License.
*   **Notable Features**:
    *   Simple and direct mapping of JSON to C++ structs/classes using macros.
    *   Minimal boilerplate for struct serialization/deserialization.
    *   Compile-time metadata generation via macros.
    *   `JS::Map` for handling JSON with a dynamic structure before parsing to a specific struct.
    *   Single header-only for easy integration.

## Enhancements for a "Best of All Worlds" Library

Creating an ideal JSON library for SDK development involves balancing performance, ease of use, flexibility, and a minimal footprint for the end-user. Drawing from the strengths of Glaze, Nlohmann, and JSON Struct, a "best of all worlds" library could incorporate the following enhancements, ensuring theoretical compatibility and a superior developer experience:

### Unified Reflection and Serialization Model
The library should aim for a reflection mechanism that is as performant and compile-time oriented as Glaze's, but with the option for more explicit, macro-based control similar to JSON Struct for cases where full C++23 reflection is not available or desired. This allows developers to choose between convention-over-configuration (via reflection) and explicit mapping (via macros or Nlohmann-style `to_json`/`from_json` functions). This caters to different C++ standards and project preferences without sacrificing performance for the common case.

### Seamless Integration and Developer Ergonomics
The library should maintain a header-only option for trivial integration, akin to all three libraries. However, it should also provide robust CMake support and integration with popular package managers, similar to Nlohmann JSON, for larger projects. The syntax should strive for Nlohmann's intuitiveness, allowing for natural C++ data structure interaction (e.g., `j["key"] = value;`) while also providing powerful, type-safe mapping to structs as seen in Glaze and JSON Struct. Error reporting should be comprehensive, offering both exception-based (like Nlohmann) and error-code based (like Glaze) mechanisms, configurable by the user.

### Advanced Feature Set with Modularity
Support for JSON Pointer, JSON Patch, and various binary formats (BSON, CBOR, MessagePack) as seen in Nlohmann JSON is crucial for a versatile SDK. Performance should be a key design goal, aiming for Glaze-like speeds for common operations. However, features should be somewhat modular, allowing users to compile out support for binary formats or advanced features they don't need, to minimize binary size and compile times, a concern for SDKs. Automatic key renaming should be flexible, supporting both compile-time options (like Glaze `opts`) and runtime/macro-based approaches (like JSON Struct). Native support or clear patterns for JSON RPC 2.0 message structures would be a significant boon for SDKs focused on communication.

### Compile-Time Configuration and Safety
The library should leverage C++'s type system and compile-time capabilities to catch as many errors as possible before runtime. This includes options for schema validation (inspired by Glaze) that can be enforced at compile time or parse time. Options for customizing memory allocation and handling of non-standard JSON (like comments or trailing commas, as Nlohmann allows with flags) should be provided, but with safe defaults.

### Focus on End-User SDK Developer Experience
The documentation must be exceptionally clear, with numerous practical examples targeted at SDK developers. This includes guides on integrating the library into an SDK, best practices for exposing JSON-based APIs, and managing data contracts between the SDK and its consumers. Performance benchmarks should be transparent and reproducible. The learning curve should be gentle for common use cases, with advanced features being opt-in and well-documented.

By combining the high-performance reflection of Glaze, the user-friendly syntax and feature richness of Nlohmann JSON, and the straightforward struct-mapping approach of JSON Struct, while ensuring configurability and a focus on the SDK developer, a truly compelling JSON library could be created. Theoretical compatibility can be maintained by ensuring core functionality relies on C++11/14, with C++23 features like advanced reflection being an optional enhancement.
