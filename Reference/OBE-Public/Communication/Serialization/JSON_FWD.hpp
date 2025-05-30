//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.12.0
// |_____|_____|_____|_|___|  https://github.com/nlohmann/JSON
//
// SPDX-FileCopyrightText: 2013 - 2025 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#ifndef INCLUDE_JSON_FWD_HPP_
#define INCLUDE_JSON_FWD_HPP_

#include <cstdint> // int64_t, uint64_t
#include <map> // map
#include <memory> // allocator
#include <string> // string
#include <vector> // vector

// #include <nlohmann/detail/abi_macros.hpp>
//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.12.0
// |_____|_____|_____|_|___|  https://github.com/nlohmann/JSON
//
// SPDX-FileCopyrightText: 2013 - 2025 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT



// This file contains all macro definitions affecting or depending on the ABI

#ifndef JSON_SKIP_LIBRARY_VERSION_CHECK
    #if defined(JSON_VERSION_MAJOR) && defined(JSON_VERSION_MINOR) && defined(JSON_VERSION_PATCH)
        #if JSON_VERSION_MAJOR != 3 || JSON_VERSION_MINOR != 12 || JSON_VERSION_PATCH != 0
            #warning "Already included a different version of the library!"
        #endif
    #endif
#endif

#define JSON_VERSION_MAJOR 3   // NOLINT(modernize-macro-to-enum)
#define JSON_VERSION_MINOR 12  // NOLINT(modernize-macro-to-enum)
#define JSON_VERSION_PATCH 0   // NOLINT(modernize-macro-to-enum)

#ifndef JSON_DIAGNOSTICS
    #define JSON_DIAGNOSTICS 0
#endif

#ifndef JSON_DIAGNOSTIC_POSITIONS
    #define JSON_DIAGNOSTIC_POSITIONS 0
#endif

#ifndef JSON_USE_LEGACY_DISCARDED_VALUE_COMPARISON
    #define JSON_USE_LEGACY_DISCARDED_VALUE_COMPARISON 0
#endif

#if JSON_DIAGNOSTICS
    #define JSON_ABI_TAG_DIAGNOSTICS _diag
#else
    #define JSON_ABI_TAG_DIAGNOSTICS
#endif

#if JSON_DIAGNOSTIC_POSITIONS
    #define JSON_ABI_TAG_DIAGNOSTIC_POSITIONS _dp
#else
    #define JSON_ABI_TAG_DIAGNOSTIC_POSITIONS
#endif

#if JSON_USE_LEGACY_DISCARDED_VALUE_COMPARISON
    #define JSON_ABI_TAG_LEGACY_DISCARDED_VALUE_COMPARISON _ldvcmp
#else
    #define JSON_ABI_TAG_LEGACY_DISCARDED_VALUE_COMPARISON
#endif

#ifndef JSON_NAMESPACE_NO_VERSION
    #define JSON_NAMESPACE_NO_VERSION 0
#endif

// Construct the namespace ABI tags component
#define JSON_ABI_TAGS_CONCAT_EX(a, b, c) JSON_abi ## a ## b ## c
#define JSON_ABI_TAGS_CONCAT(a, b, c) \
    JSON_ABI_TAGS_CONCAT_EX(a, b, c)

#define JSON_ABI_TAGS                                       \
    JSON_ABI_TAGS_CONCAT(                                   \
            JSON_ABI_TAG_DIAGNOSTICS,                       \
            JSON_ABI_TAG_LEGACY_DISCARDED_VALUE_COMPARISON, \
            JSON_ABI_TAG_DIAGNOSTIC_POSITIONS)

// Construct the namespace version component
#define JSON_NAMESPACE_VERSION_CONCAT_EX(major, minor, patch) \
    _v ## major ## _ ## minor ## _ ## patch
#define JSON_NAMESPACE_VERSION_CONCAT(major, minor, patch) \
    JSON_NAMESPACE_VERSION_CONCAT_EX(major, minor, patch)

#if JSON_NAMESPACE_NO_VERSION
#define JSON_NAMESPACE_VERSION
#else
#define JSON_NAMESPACE_VERSION                                 \
    JSON_NAMESPACE_VERSION_CONCAT(JSON_VERSION_MAJOR, \
                                           JSON_VERSION_MINOR, \
                                           JSON_VERSION_PATCH)
#endif

// Combine namespace components
#define JSON_NAMESPACE_CONCAT_EX(a, b) a ## b
#define JSON_NAMESPACE_CONCAT(a, b) \
    JSON_NAMESPACE_CONCAT_EX(a, b)

#ifndef JSON_NAMESPACE
#define JSON_NAMESPACE               \
    nlohmann::JSON_NAMESPACE_CONCAT( \
            JSON_ABI_TAGS,           \
            JSON_NAMESPACE_VERSION)
#endif

#ifndef JSON_NAMESPACE_BEGIN
#define JSON_NAMESPACE_BEGIN                \
    namespace nlohmann                               \
    {                                                \
    inline namespace JSON_NAMESPACE_CONCAT( \
                JSON_ABI_TAGS,              \
                JSON_NAMESPACE_VERSION)     \
    {
#endif

#ifndef JSON_NAMESPACE_END
#define JSON_NAMESPACE_END                                     \
    }  /* namespace (inline namespace) NOLINT(readability/namespace) */ \
    }  // namespace nlohmann
#endif


/*!
@brief namespace for Niels Lohmann
@see https://github.com/nlohmann
@since version 1.0.0
*/
JSON_NAMESPACE_BEGIN

/*!
@brief default JSON_Serializer template argument

This serializer ignores the template arguments and uses ADL
([argument-dependent lookup](https://en.cppreference.com/w/cpp/language/adl))
for serialization.
*/
template<typename T = void, typename SFINAE = void>
struct adl_serializer;

/// a class to store JSON values
/// @sa https://JSON.nlohmann.me/api/Basic_JSON/
template<template<typename U, typename V, typename... Args> class ObjectType =
         std::map,
         template<typename U, typename... Args> class ArrayType = std::vector,
         class StringType = std::string, class BooleanType = bool,
         class NumberIntegerType = std::int64_t,
         class NumberUnsignedType = std::uint64_t,
         class NumberFloatType = double,
         template<typename U> class AllocatorType = std::allocator,
         template<typename T, typename SFINAE = void> class JSON_Serializer =
         adl_serializer,
         class BinaryType = std::vector<std::uint8_t>, // cppcheck-suppress syntaxError
         class CustomBaseClass = void>
class Basic_JSON;

/// @brief JSON Pointer defines a string syntax for identifying a specific value within a JSON document
/// @sa https://JSON.nlohmann.me/api/JSON_Pointer/
template<typename RefStringType>
class JSON_Pointer;

/*!
@brief default specialization
@sa https://JSON.nlohmann.me/api/JSON/
*/
using JSON = Basic_JSON<>;

/// @brief a minimal map-like container that preserves insertion order
/// @sa https://JSON.nlohmann.me/api/Ordered_Map/
template<class Key, class T, class IgnoredLess, class Allocator>
struct Ordered_Map;

/// @brief specialization that maintains the insertion order of object keys
/// @sa https://JSON.nlohmann.me/api/Ordered_JSON/
using Ordered_JSON = Basic_JSON<nlohmann::Ordered_Map>;

JSON_NAMESPACE_END

#endif  // INCLUDE_JSON_FWD_HPP_