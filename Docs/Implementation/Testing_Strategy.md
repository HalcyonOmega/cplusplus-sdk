# Testing Strategy

---

## Overview

A solid testing strategy is key to building a reliable SDK. The goal is to catch bugs early, make refactoring safe, and give users (and yourself) confidence that everything works as expected.

## Types of Tests

- **Unit Tests:**
  - Test individual functions, classes, or modules in isolation.
  - Fast to run and easy to debug.
- **Integration Tests:**
  - Test how different parts of the SDK work together (e.g., protocol handling, message passing).
  - Useful for catching issues that only appear when components interact.
- **System/End-to-End Tests (optional):**
  - Test the SDK as a whole, possibly with a mock or real MCP server/client.

## Recommended Frameworks

- **Catch2** and **GoogleTest** are both great choices for C++ testing.
  - Catch2 is header-only, easy to set up, and has a nice syntax.
  - GoogleTest is widely used, feature-rich, and integrates well with CI tools.
- Pick one and stick with it for consistency.

## Test Organization

- Put tests in a separate directory (e.g., `Testing/` or `tests/`).
- Mirror the SDK's directory structure for easy navigation.
- Name test files clearly (e.g., `test_protocol.cpp`, `test_utils.cpp`).
- Use CMake to add tests as separate targets and enable them with `ctest`.

## Running Tests

- Integrate tests into the build system (CMake's `enable_testing()` and `add_test()` commands).
- Run all tests automatically in CI for every push and pull request.
- Make it easy to run tests locally:
  ```sh
  cd build
  ctest --output-on-failure
  ```

## Code Coverage

- Aim for high coverage, but don't obsess over 100%—focus on critical code paths.
- Use tools like `gcov`, `lcov`, or `Codecov` (with CI integration) to track coverage.
- Add coverage badges to the README if you want to show off!

## Continuous Integration (CI)

- Run tests on all supported platforms and compilers in CI (e.g., GitHub Actions, GitLab CI).
- Fail the build if any test fails.
- Optionally, run tests with sanitizers (ASan, UBSan) to catch memory and undefined behavior bugs.

---

**TL;DR:**
- Write unit and integration tests
- Use Catch2 or GoogleTest
- Organize tests clearly
- Run tests in CI
- Track code coverage