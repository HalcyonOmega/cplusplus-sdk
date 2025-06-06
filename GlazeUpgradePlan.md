# Glaze JSON Enhancement Plan: Features from Nlohmann & JSON Struct

This document outlines potential features to enhance Glaze JSON, drawing inspiration from the convenience, quality-of-life, and customization options found in Nlohmann JSON and JSON Struct libraries. The goal is to broaden Glaze's appeal and usability without compromising its core strengths in performance and modern C++ design.

## Features Inspired by Nlohmann JSON

*   **STL-like Intuitive Syntax for Dynamic JSON**:
    *   Description: More ergonomic, STL-like access and manipulation for dynamic JSON objects (`glz::json_t` or equivalent), e.g., Python-like `j["key"]` access, `_json` string literal equivalent, direct iteration, `push_back`, `emplace` etc.
*   **Optional Exception-Based Error Handling**:
    *   Description: An option to use C++ exceptions for error reporting, alongside the existing `glz::error_code` / `glz::expected` system, for users preferring an exception-based workflow.
*   **Broader Binary Format Support**:
    *   Description: Native support for more standard binary formats like CBOR, MessagePack, and BJData, in addition to the existing BEVE and planned BSON/UBJSON.
*   **JSON Patch (RFC 6902) Support**:
    *   Description: Implementation of JSON Patch operations to apply a sequence of changes to a JSON document.
*   **JSON Merge Patch (RFC 7386) Support**:
    *   Description: Implementation of JSON Merge Patch for simpler patching of JSON documents, particularly for merging.
*   **Extended Implicit Type Conversions**:
    *   Description: More flexible (yet safe and clearly defined) implicit conversions between JSON values and common C++ types, with opt-out mechanisms where performance is critical.
*   **Parser Flags for Extended JSON Syntax**:
    *   Description: Additional parser flags to support non-standard JSON features like trailing commas in objects and arrays (Glaze already supports comments via JSONC).
*   **Dedicated Forward Declaration Header**:
    *   Description: A `glaze_fwd.hpp` (or similar) to allow forward declaration of Glaze types, reducing compilation dependencies and times in larger projects.
*   **Convenience `value_or` Access Method**:
    *   Description: A method like Nlohmann's `value("key", default_value)` or `j.value("/pointer/path"_json_pointer, default_value)` for safely retrieving a value or a default if the key/path is not found or the type is wrong.
*   **Direct `contains(key)` Method**:
    *   Description: A simple boolean method `contains("key")` (or `contains_pointer("/path")`) for objects to check for key/path existence.
*   **Explicit `update()` Functionality**:
    *   Description: A function to merge one JSON object into another (e.g., `target.update(source)`), similar to Python's `dict.update()`.
*   **Integrated File I/O Convenience Functions**:
    *   Description: High-level functions for directly parsing JSON from file paths/streams and serializing to file paths/streams, simplifying common I/O operations (e.g., `glz::read_json("file.json", obj); glz::write_json(obj, "output.json");`).
*   **ADL-based Serializer Customization (`adl_serializer` pattern)**:
    *   Description: Support for non-intrusive customization of serialization for arbitrary types using Argument-Dependent Lookup (ADL) by specializing a template (e.g., `adl_serializer`) in the type's namespace or the global namespace.

## Features Inspired by JSON Struct

*   **Optional C++11-Compatible Macro-Based Reflection/Mapping**:
    *   Description: A set of simple macros (e.g., `GLZ_OBJ`, `GLZ_MEMBER`, `GLZ_MEMBER_WITH_NAME`) as an alternative to C++23 reflection or full `glz::meta` struct definitions. This would enhance ease of use for simpler cases, non-aggregate types, or in projects not yet on C++23, by providing explicit, localized mapping definitions.
*   **Flexible Intermediate Representation for Dynamic Structures (`JS::Map`-like)**:
    *   Description: An enhanced mode for `glz::json_t` or a new type that allows parsing into a more flexible, map-like or `std::any`-like structure without upfront schema knowledge. This would be useful for inspecting/transforming JSON before strict parsing or when dealing with highly dynamic/heterogeneous structures. JSON Struct's `JS::Map` serves a similar purpose.
