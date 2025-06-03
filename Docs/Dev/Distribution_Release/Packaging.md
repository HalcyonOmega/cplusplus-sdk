# Packaging

---

## Overview

Packaging is all about making it easy for others to use your SDK. The goal is to provide a clean, flexible package that works across platforms and build systems, and is easy to integrate into other C++ projects.

## Library Types

- **Static and Shared Libraries:**
  - Provide both static (`.a`/`.lib`) and shared (`.so`/`.dll`/`.dylib`) builds if possible.
  - Let users choose via a CMake option (e.g., `BUILD_SHARED_LIBS`).

## CMake Packaging

- **Export CMake Targets:**
  - Use `install(TARGETS ...)` and `export()` to make your library available as a CMake target.
  - Provide a `Config.cmake` file so users can `find_package()` your SDK easily.
- **Install Headers and CMake Files:**
  - Install public headers to an `include/` directory.
  - Install CMake config files to the appropriate location (usually under `lib/cmake/<ProjectName>`).
- **Versioning:**
  - Use semantic versioning (e.g., `1.2.3`) and include the version in your CMake config.

## Package Managers (Optional)

- **vcpkg/Conan:**
  - Consider adding support for vcpkg or Conan if you want to reach a wider audience.
  - Provide a `conanfile.py` or vcpkg portfile as needed.
  - Not required for initial releases, but nice to have.

## Cross-Platform Considerations

- Test packaging on all supported platforms (Windows, Linux, macOS).
- Make sure install paths and exported targets work everywhere.
- Avoid hardcoded paths or platform-specific install logic.

## Documentation for Consumers

- Document how to install and use the SDK in other projects (with and without CMake).
- Provide example `CMakeLists.txt` snippets for consumers.
- Mention any required dependencies or environment variables.

---

**TL;DR:**
- Provide static and shared libs
- Export CMake targets and config files
- Install headers and docs
- (Optionally) support vcpkg/Conan
- Document usage for consumers 