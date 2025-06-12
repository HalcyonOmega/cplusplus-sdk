#pragma once

#include "Core.h"
#include "json.hpp"

MCP_NAMESPACE_BEGIN

// Concept: checks if a type is a nlohmann::basic_json specialization
// (modern C++20 replacement for std::enable_if used by nlohmann macros).
template <typename T>
concept IsBasicJSON = nlohmann::detail::is_basic_json<T>::value;

// DEFINE_STRUCT_JSON replicates NLOHMANN_DEFINE_TYPE_INTRUSIVE but
// uses C++20 concepts/`requires` instead of std::enable_if, eliminating
// the clang-tidy modernize-use-requires warning.
//
// Example usage inside a struct:
//   struct Foo { int Bar; string Baz; MCP_DEFINE_TYPE_INTRUSIVE(Foo, Bar, Baz); };
#define DEFINE_STRUCT_JSON(Type, ...)                                                              \
    template <typename BasicJSONType>                                                              \
        requires IsBasicJSON<BasicJSONType>                                                        \
    friend void to_json(BasicJSONType& nlohmann_json_j, const Type& nlohmann_json_t) {             \
        NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_TO, __VA_ARGS__))                   \
    }                                                                                              \
    template <typename BasicJSONType>                                                              \
        requires IsBasicJSON<BasicJSONType>                                                        \
    friend void from_json(const BasicJSONType& nlohmann_json_j, Type& nlohmann_json_t) {           \
        NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_FROM, __VA_ARGS__))                 \
    }

#define DEFINE_STRUCT_JSON_EXTERNAL

#define DEFINE_ENUM_JSON

using DEFINE_CHILD_TYPE_JSON;
using DEFINE_TYPE_JSON;

MCP_NAMESPACE_END