# Dependency Management

---

## Overview

Our approach to dependency management is simple: keep dependencies to a minimum, use CMake for build configuration, and stick to best practices for modern C++ projects. This helps keep the SDK lightweight, portable, and easy to maintain.

## Guiding Principles

- **Minimize dependencies:** Only add a dependency if it's absolutely necessary and brings significant value. Prefer standard library features whenever possible.
- **Leverage CMake:** All build configuration and dependency management should be handled through CMake. This ensures cross-platform compatibility and makes it easy for users to integrate the SDK into their own projects.
- **Best practices:** Use modern CMake features (targets, `target_link_libraries`, `target_include_directories`, etc.), avoid global variables, and keep the build system clean and modular.

## Adding Dependencies

- **First, try to avoid it:** Can you solve the problem with the standard library or existing code? If so, do that.
- **If you must add a dependency:**
  - Prefer header-only or single-file libraries when possible.
  - Use CMake's `FetchContent` or `ExternalProject` for third-party dependencies, but only if absolutely necessary.
  - Document why the dependency is needed and any alternatives considered.
  - Make sure the dependency is compatible with the supported platforms and C++ standards.

## CMake Integration

- Use `target_link_libraries` to link dependencies to specific targets, not globally.
- Use `target_include_directories` with `PRIVATE`, `PUBLIC`, or `INTERFACE` as appropriate.
- Prefer `FetchContent` for pulling in dependencies at configure time, but only for essential libraries.
- Avoid hardcoding paths or relying on system-wide installs when possible.

## Exceptions

While minimizing dependencies is a strong guideline, sometimes a third-party library is the best (or only) solution. In those cases, document the rationale and make sure the integration is as clean and optional as possible.

## Future Considerations

If the project grows or user needs change, we can revisit this policy and consider package managers like vcpkg or Conan. For now, simplicity and minimalism win.

---

**TL;DR:**
- Keep dependencies minimal.
- Use CMake for everything.
- Only add third-party libs if there's a really good reason.
- Document any exceptions.

-   **External Libraries:** 
-   **Management Strategy:** (e.g., Conan, vcpkg, submodules) 