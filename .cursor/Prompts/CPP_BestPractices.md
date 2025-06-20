# C++20 Modernization and Best Practices Checklist

As an AI code quality agent, your task is to scan the C++ codebase and identify deviations from the modern C++20 best practices outlined in this document. For each violation, you should suggest a specific, actionable remediation. Your effectiveness will be judged on the accuracy and completeness of your findings.

## 1. Memory and Resource Management

The primary goal is to eliminate manual memory management and prevent resource leaks using C++'s RAII (Resource Acquisition Is Initialization) capabilities.

### 1.1. Avoid Raw Memory Management (`new`/`delete`)
- **Check**: Scan for explicit calls to `new`, `new[]`, `delete`, and `delete[]`. Also, look for usage of C-style `malloc()`, `calloc()`, `realloc()`, and `free()`.
- **Suggestion**:
    - Replace `new T(...)` with `std::make_unique<T>(...)` or `std::make_shared<T>(...)`.
    - Replace `new T[N]` with `std::vector<T>(N)` or `std::make_unique<T[]>(N)`.
    - If C-API interaction is necessary, wrap the returned pointer in a custom deleter with a smart pointer.
- **Why**: Automates memory management, prevents memory leaks and dangling pointers, and clarifies ownership semantics.

### 1.2. Use Smart Pointers for Dynamic Ownership
- **Check**: Identify raw pointers that are used as owners of memory (i.e., they are responsible for `delete`ing the object). Look for raw pointer member variables or factory functions returning raw pointers.
- **Suggestion**:
    - For exclusive ownership, use `std::unique_ptr`. It should be the default choice.
    - For shared ownership, use `std::shared_ptr`.
    - Use `std::weak_ptr` to break reference cycles when using `std::shared_ptr`.
    - Raw pointers should only be used as non-owning, observing pointers.
- **Why**: Smart pointers make resource ownership explicit and automatic, preventing entire classes of bugs.

### 1.3. Avoid C-Style Arrays
- **Check**: Scan for declarations of C-style arrays (e.g., `int arr[10];`).
- **Suggestion**:
    - For fixed-size arrays known at compile-time, use `std::array<T, N>`.
    - For dynamic arrays, use `std::vector<T>`.
- **Why**: `std::array` and `std::vector` provide iterator support, size tracking, and are generally safer (e.g., `at()` for bounds-checked access) and more compatible with standard library algorithms.

### 1.4. Avoid C-Style Casts
- **Check**: Scan for C-style cast expressions (e.g., `(MyType*)ptr`).
- **Suggestion**: Replace with the appropriate C++ cast:
    - `static_cast` for most well-defined conversions (e.g., numeric types, up/down-casting in a class hierarchy).
    - `reinterpret_cast` for low-level, unsafe type-punning. Its use should be heavily scrutinized.
    - `dynamic_cast` for safely down-casting polymorphic types. Check for its performance implications.
    - `const_cast` to add or remove `const`. Its use is often a sign of a design flaw.
- **Why**: C++ casts are more explicit about intent and safer because they are more restrictive, allowing the compiler to catch more errors.

### 1.5. Use `nullptr` for Null Pointers
- **Check**: Scan for the use of `NULL` macro or literal `0` to represent null pointers.
- **Suggestion**: Replace all instances with `nullptr`.
- **Why**: `nullptr` is type-safe (it has type `std::nullptr_t`) and prevents ambiguities that can occur with `NULL`/`0` in function overloading and template instantiation.

## 2. Leveraging C++20 Features

This section focuses on adopting modern C++20 features to improve code safety, readability, and performance.

### 2.1. Use `std::span` for Non-Owning Views
- **Check**: Identify functions that accept a raw pointer and a size (e.g., `void process(int* data, size_t size);`) or two iterators to represent a contiguous sequence.
- **Suggestion**: Refactor the function to accept a `std::span` (e.g., `void process(std::span<int> data);`).
- **Why**: `std::span` provides a bounds-checked, non-owning view over contiguous data. It's safer than raw pointers, more expressive, and can be constructed from C-style arrays, `std::vector`, and `std::array` without cost.

### 2.2. Use Concepts for Template Constraints
- **Check**: Look for complex `std::enable_if` or SFINAE techniques in template declarations. Also check for unconstrained `auto` in template parameters.
- **Suggestion**: Replace SFINAE with C++20 Concepts using `requires` clauses. This makes the template's requirements part of its interface.
- **Why**: Concepts dramatically improve compiler error messages when template constraints are not met, and make the code's intent clearer and more self-documenting.

### 2.3. Use Ranges for Composable Algorithms
- **Check**: Identify imperative loops (e.g., `for`, `while`) that perform filtering, transformation, or accumulation. Look for chained calls to standard algorithms that pass begin/end iterators.
- **Suggestion**: Refactor the code to use the Ranges library, composing views and actions with the `|` pipe operator.
- **Why**: Ranges lead to more declarative, readable, and less error-prone code by eliminating manual iterator management and boilerplate loops.

### 2.4. Simplify Comparisons with `operator<=>`
- **Check**: Identify classes or structs that define two or more of the six relational comparison operators (`==`, `!=`, `<`, `>`, `<=`, `>=`).
- **Suggestion**: Replace the manually defined operators with a single defaulted three-way comparison operator: `auto operator<=>(const MyClass&) const = default;`. You may also need to implement `operator==`.
- **Why**: The "spaceship operator" reduces boilerplate code significantly, as the compiler can generate all six relational operators from it.

### 2.7. Use `constinit` for Compile-Time Constant Initialization
- **Check**: Look for global or static variables that are `const` and initialized with a constant expression, but whose initialization is not guaranteed to happen at compile-time.
- **Suggestion**: Mark them with `constinit` to enforce static initialization.
- **Why**: `constinit` guarantees that a variable's initialization happens at compile time, avoiding potential static initialization order fiasco issues and ensuring the variable is ready before any dynamic initialization begins.

## 3. General Code Quality and Correctness

These are foundational practices for writing robust, maintainable C++ code.

### 3.1. Enforce `const`-Correctness
- **Check**:
    - Variables that are initialized once and never modified should be `const`.
    - Member functions that do not modify the object's state should be marked `const`.
    - Function parameters passed by pointer or reference that are not modified should be `const`.
- **Suggestion**: Apply `const` qualifier where applicable.
- **Why**: `const` documents the code's intent, prevents accidental modifications, and allows the compiler to perform more optimizations.

### 3.2. Use `constexpr` for Compile-Time Evaluation
- **Check**: Identify functions that can be evaluated at compile time if their arguments are known. Look for variables used as array bounds or in template parameters that could be `constexpr`.
- **Suggestion**: Mark functions and variables as `constexpr` where possible.
- **Why**: `constexpr` allows computations to be moved from runtime to compile time, improving performance and enabling compile-time validation of logic.

### 3.3. Prefer `enum class` over `enum`
- **Check**: Scan for usage of unscoped `enum`.
- **Suggestion**: Convert `enum` to `enum class`.
- **Why**: `enum class` is strongly-typed and scoped. This prevents accidental conversions to integer types and name clashes between enumerators of different enums.

### 3.4. Mark Overriding Functions with `override`
- **Check**: Look for virtual functions in derived classes that override a base class function but are not marked with the `override` specifier.
- **Suggestion**: Add `override` to all such functions.
- **Why**: `override` causes the compiler to verify that the function is actually overriding a base class virtual function, preventing subtle bugs caused by signature mismatches.

### 3.5. Use `final` to Prevent Inheritance or Overriding
- **Check**: Identify classes not designed to be base classes, or virtual functions not designed to be overridden further down a hierarchy.
- **Suggestion**: Mark them with the `final` specifier.
- **Why**: `final` clearly communicates design intent and allows the compiler to perform devirtualization optimizations.

### 3.6. Use `noexcept` for Non-Throwing Functions
- **Check**: Identify functions that do not and are not expected to throw exceptions. This is particularly important for move constructors, move assignment operators, and destructors.
- **Suggestion**: Mark them as `noexcept`.
- **Why**: `noexcept` gives the compiler optimization opportunities (e.g., using a move operation instead of a copy) and allows for compile-time checks on exception guarantees.

### 3.7. Use `[[nodiscard]]` for Important Return Values
- **Check**: Identify functions whose return value must be checked (e.g., functions returning an error code, a resource handle, or a new object).
- **Suggestion**: Add the `[[nodiscard]]` attribute to the function declaration. A reason can be provided (e.g., `[[nodiscard("Ignoring the result will leak memory")]]`).
- **Why**: `[[nodiscard]]` causes the compiler to issue a warning if the return value of a function is ignored, helping to prevent bugs.

### 3.8. Follow the Rule of Zero
- **Check**: Scan for classes that manually define any of the special member functions (destructor, copy/move constructor, copy/move assignment operator).
- **Suggestion**: Refactor the class to use RAII-compliant members (like `std::unique_ptr` and `std::vector`) so that it doesn't need to define any of them. The default, compiler-generated special members will then be correct.
- **Why**: The Rule of Zero leads to simpler, less error-prone code by delegating resource management to dedicated resource-managing classes.

## 4. Advanced C++ Idioms and Concurrency

This section covers modern idioms that lead to safer, more expressive, and more performant code, especially in concurrent and asynchronous applications.

### 4.1. Prefer `std::jthread` and Coroutines for Concurrency
- **Check**:
    - Scan for usage of `std::thread` that requires manual `.join()` or `.detach()` calls.
    - Look for complex callback-based or future-based asynchronous code.
- **Suggestion**:
    - Replace `std::thread` with `std::jthread`, which automatically joins upon destruction, preventing accidentally orphaned threads.
    - Refactor complex asynchronous workflows to use C++20 Coroutines for more readable, sequential-looking code.
- **Why**: `std::jthread` is a safer RAII-compliant thread wrapper. Coroutines drastically simplify asynchronous logic, reducing callback hell and making the code easier to reason about.

### 4.2. Embrace Modules over Header Files
- **Check**: Identify opportunities to convert related sets of headers into C++20 modules. This is a forward-looking check for new code.
- **Suggestion**: For new components, prefer creating a module interface (`.cppm`) instead of a public header file. Gradually migrate existing code where feasible.
- **Why**: Modules offer superior encapsulation (no more macro leakage), eliminate problems with include order, and can dramatically reduce build times by avoiding repetitive parsing of headers.

### 4.3. Use Modern Type-Safe Unions and Optionals
- **Check**:
    - Scan for usage of C-style `union` or tag-based discriminated unions.
    - Identify functions that return pointers or use magic values (e.g., `-1`, `nullptr`) to indicate the absence of a value.
- **Suggestion**:
    - Replace `union` with `std::variant`, a type-safe sum type.
    - Replace nullable pointers or magic return values with `std::optional`.
- **Why**: `std::variant` and `std::optional` are more expressive and type-safe. `std::optional` clearly communicates the possibility of an absent value, while `std::variant` provides a safe way to handle types that can hold one of several alternatives.

### 4.4. Prefer Uniform Initialization
- **Check**: Look for inconsistent initialization syntax (using `()` vs `{}`).
- **Suggestion**: Use brace-initialization (`{}`) consistently where possible.
- **Why**: Uniform initialization syntax helps prevent the "most vexing parse", forbids narrowing conversions (e.g., `int x {7.0};` is a compiler error), and provides a consistent way to initialize objects, including aggregates and standard containers.

### 4.5. Leverage Modern Lambda Expressions
- **Check**: Look for hand-written function objects (functors) or complex `std::bind` expressions.
- **Suggestion**:
    - Replace function objects with lambdas.
    - Replace `std::bind` with lambdas, which are more readable and often more efficient.
    - Use template lambdas (`[]<typename T>(T arg){...}`) and stateless lambdas where appropriate.
- **Why**: Lambdas provide a concise, inline syntax for creating function objects. C++20 enhances them with features like template parameters, making them even more powerful and further reducing the need for older, more verbose idioms.
