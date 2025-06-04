# Platform & Compiler Support

---

## Supported Platforms & Compilers

The goal is to support as many platforms and compilers as is reasonable for a modern C++ SDK. At a minimum, we aim to support:

- **Operating Systems:**
  - Windows
  - Linux
  - macOS
- **Compilers:**
  - GCC (GNU Compiler Collection)
  - Clang
  - MSVC (Microsoft Visual C++)

If it works elsewhere, that's a bonus!

## Compatibility Best Practices

To maximize portability and minimize headaches, follow these guidelines:

- **Stick to the C++ Standard:**
  - Use features from the C++ standard library and language, not compiler-specific extensions.
  - Avoid vendor-specific attributes, pragmas, or built-ins unless absolutely necessary (and always provide a fallback or guard with `#ifdef`).
- **Avoid Platform-Specific APIs:**
  - Don't use OS-specific APIs (like Win32, POSIX, or Apple frameworks) in core SDK code. If you must, isolate them behind a well-defined interface.
- **Mind File Paths and Line Endings:**
  - Use portable path handling (e.g., `std::filesystem` in C++17+), and don't assume `/` or `\` as the path separator.
  - Be aware of line ending differences (LF vs CRLF) if reading/writing files.
- **Character Encodings:**
  - Prefer UTF-8 for text data. Avoid platform-specific encodings unless you have to support legacy systems.
- **Threading and Concurrency:**
  - Use the C++ standard threading library, not OS-specific threading APIs.
- **Compiler Warnings:**
  - Build with warnings enabled (`-Wall -Wextra` for GCC/Clang, `/W4` for MSVC) and treat warnings as errors where possible. Fix portability warnings early.
- **Endianness and Data Sizes:**
  - Don't assume endianness or data type sizes. Use fixed-width types (`std::uint32_t`, etc.) when needed.
- **Build System:**
  - Use CMake for cross-platform builds. Avoid hardcoding compiler flags or paths.

## What to Avoid (If You Want Maximum Compatibility)

- Compiler-specific language extensions (e.g., `__declspec`, `__attribute__`, `#pragma once` without fallback)
- OS-specific APIs in core logic
- Inline assembly
- Non-standard filesystem or networking APIs
- Hardcoded file paths, path separators, or line endings
- Reliance on undefined or implementation-defined behavior
- Assuming a particular endianness or data alignment
- Using features not available in the minimum supported C++ standard
- System-specific build scripts (stick to CMake)

## Future Considerations

If a feature is only available on some platforms or compilers, document it clearly and provide a fallback or alternative where possible. If we need to drop support for a platform or compiler, document the rationale.

---

**TL;DR:**
- Support Windows, Linux, macOS, GCC, Clang, and MSVC.
- Stick to the C++ standard and avoid platform/compiler-specific stuff.
- If in doubt, ask: "Will this work everywhere?" 