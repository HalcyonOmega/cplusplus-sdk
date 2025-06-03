# Build System

---

## Overview

This SDK uses CMake as its build system. CMake is the de facto standard for modern C++ projects, offering great cross-platform support and flexibility. The goal is to keep the build process simple, reliable, and easy to integrate into other projects.

## Recommended CMake Version

- **CMake 3.15+** is recommended for best compatibility and access to modern features (like `FetchContent`, target-based commands, etc.).
- Older versions may work, but aren't officially supported.

## Out-of-Source Builds

- Always build out-of-source (i.e., build artifacts go in a separate directory, not mixed with source files).
- Example:
  ```sh
  mkdir build
  cd build
  cmake ..
  cmake --build .
  ```
- This keeps your source tree clean and makes it easy to manage multiple build configurations.

## Project Structure

- Use CMake targets (`add_library`, `add_executable`) for all components.
- Prefer `target_link_libraries`, `target_include_directories`, and other target-based commands over global settings.
- Organize code into logical subdirectories and use `add_subdirectory()` in your `CMakeLists.txt`.
- Avoid hardcoding paths or compiler flags; use CMake variables and options.

## Cross-Platform Support

- CMake makes it easy to build on Windows, Linux, and macOS with minimal changes.
- Use CMake's built-in variables (like `CMAKE_SYSTEM_NAME`, `CMAKE_CXX_COMPILER_ID`) to handle platform/compiler-specific tweaks if needed.
- Avoid platform-specific code in the build system unless absolutely necessary.

## Integrating with Other Projects

- Export CMake targets so users can `find_package()` and link against the SDK easily.
- Provide a `Config.cmake` file for consumers if possible.
- Keep dependencies minimal and manage them via CMake (see Dependency Management doc).

## Continuous Integration (CI)

- Set up CI to build and test the SDK on all supported platforms and compilers.
- Recommended: GitHub Actions, GitLab CI, or similar services.
- Run builds with different configurations (Debug/Release, different compilers, etc.) to catch issues early.

## Example: Minimal Build

```sh
mkdir build
cd build
cmake ..
cmake --build .
```

---

**TL;DR:**
- Use CMake 3.15+
- Always build out-of-source
- Use target-based CMake commands
- Keep it modular and cross-platform
- Set up CI to catch issues early

-   **Primary Build Tool:** CMake (Refer to `CMakeLists.txt`)
-   **Build Types:** (Debug, Release, etc.)
-   **Package Manager Integration Options:** 