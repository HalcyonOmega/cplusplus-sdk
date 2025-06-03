# Asynchronous Operations

---

## Overview

This SDK handles asynchronous operations using only the C++ standard library. That means we stick to features like `std::thread`, `std::future`, `std::async`, and (where available) C++20 coroutines. No third-party libraries (like Boost.Asio or libuv) are used at this stage. This keeps things simple and portable for most modern C++ environments.

## Supported C++ Standards

- **C++11 and up:** Basic async support via `std::thread`, `std::future`, and `std::async`.
- **C++20:** Adds coroutine support (`co_await`, `co_yield`, `co_return`), which can make async code much cleaner and more composable.
- **Older standards:** Not supported for async features. If you need to target pre-C++11, you'll need to implement your own threading or consider a third-party library (not in scope for now).

## API Design Recommendations

- **Prefer `std::future`/`std::promise` or coroutines** for async APIs. This makes it easy for users to compose and chain async operations.
- **Avoid exposing raw threads** in public APIs. Instead, provide higher-level abstractions (e.g., functions returning `std::future` or coroutine-based interfaces).
- **Use `std::async` for simple fire-and-forget or deferred execution** when you don't need fine-grained control over the thread.
- **If using coroutines:** Clearly document which APIs require C++20, and provide fallback sync/async options for users on older standards if possible.
- **Cancellation and timeouts:** The C++ standard library doesn't provide built-in cancellation for async tasks. If you need this, design your APIs to accept cancellation tokens or flags, but keep it simple for now.

## Portability and Compatibility

- **Most modern compilers (GCC, Clang, MSVC) support C++11 and up.**
- **Coroutines require C++20** and may not be available everywhere yet. Check your compiler's support if you want to use them.
- **No third-party dependencies** means easier builds and fewer headaches, but also fewer advanced features (like async I/O, event loops, etc.).

## Future Considerations

If we hit limitations with the standard library (e.g., need for async I/O, better cancellation, or cross-platform event loops), we can revisit third-party solutions like Boost.Asio or libuv. For now, let's keep it simple and standard.

## Example Usage

```cpp
#include <future>
#include <iostream>

std::future<int> async_add(int a, int b) {
    return std::async(std::launch::async, [a, b] { return a + b; });
}

int main() {
    auto fut = async_add(2, 3);
    std::cout << "Result: " << fut.get() << std::endl;
}
```

If using coroutines (C++20):

```cpp
#include <coroutine>
#include <iostream>
// ... coroutine example would go here ...
```

---

**TL;DR:**
- Use C++ standard library for async.
- Prefer futures/promises or coroutines for APIs.
- No third-party libs (yet).
- Revisit if/when we need more advanced features. 