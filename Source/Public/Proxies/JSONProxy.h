#pragma once

#include <optional>
#include <type_traits>

#include "Macros.h"
#include "json.hpp"

MCP_NAMESPACE_BEGIN

// Concept: checks if a type is a nlohmann::basic_json specialization
template <typename T>
concept IsBasicJSON = nlohmann::detail::is_basic_json<T>::value;

// Concept: checks if a type is a specialization of std::optional
template <typename T> struct is_optional : std::false_type {};
template <typename T> struct is_optional<std::optional<T>> : std::true_type {};
template <typename T>
concept IsOptional = is_optional<T>::value;

// -----------------------------------------------------------------------------
// Custom JSON serialization macros with key renaming support
// -----------------------------------------------------------------------------

// Macro for custom key mapping - generates the functions directly
#define DEFINE_TYPE_JSON(Type, ...)                                                                \
    template <typename BasicJSONType>                                                              \
        requires IsBasicJSON<BasicJSONType>                                                        \
    friend void to_json(BasicJSONType& json, const Type& obj) {                                    \
        __VA_ARGS__                                                                                \
    }                                                                                              \
    template <typename BasicJSONType>                                                              \
        requires IsBasicJSON<BasicJSONType>                                                        \
    friend void from_json(const BasicJSONType& json, Type& obj) {                                  \
        __VA_ARGS__                                                                                \
    }

// Helper macros that handle serialization and deserialization.
#define TO_JSON(member, key)                                                                       \
    if constexpr (IsOptional<decltype(obj.member)>) {                                              \
        if (obj.member.has_value()) { json[key] = *obj.member; }                                   \
    } else {                                                                                       \
        json[key] = obj.member;                                                                    \
    }

#define FROM_JSON(member, key)                                                                     \
    if constexpr (IsOptional<decltype(obj.member)>) {                                              \
        if (json.contains(key)) { json.at(key).get_to(obj.member); }                               \
    } else {                                                                                       \
        json.at(key).get_to(obj.member);                                                           \
    }

// User-facing macro for aliasing members.
#define KEY(member, key)                                                                           \
    TO_JSON(member, key)                                                                           \
    FROM_JSON(member, key)

// -----------------------------------------------------------------------------
// Enum serialization with C++20 features and longer parameter names
// -----------------------------------------------------------------------------

// C++20 concept to check if a type is an enum
template <typename EnumerationType>
concept IsEnumType = std::is_enum_v<EnumerationType>;

#define DEFINE_ENUM_JSON(EnumerationType, ...)                                                     \
    template <typename BasicJSONType>                                                              \
        requires IsBasicJSON<BasicJSONType> && IsEnumType<EnumerationType>                         \
    inline void to_json(BasicJSONType& jsonObject, const EnumerationType& enumValue) {             \
        static constexpr auto enumMappings =                                                       \
            std::to_array<std::pair<EnumerationType, BasicJSONType>>(__VA_ARGS__);                 \
        if constexpr (auto mappingIterator =                                                       \
                          std::ranges::find_if(enumMappings,                                       \
                                               [enumValue](const auto& enumJsonPair) {             \
                                                   return enumJsonPair.first == enumValue;         \
                                               });                                                 \
                      mappingIterator != std::ranges::end(enumMappings)) {                         \
            jsonObject = mappingIterator->second;                                                  \
        } else {                                                                                   \
            jsonObject = std::ranges::begin(enumMappings)->second;                                 \
        }                                                                                          \
    }                                                                                              \
    template <typename BasicJSONType>                                                              \
        requires IsBasicJSON<BasicJSONType> && IsEnumType<EnumerationType>                         \
    inline void from_json(const BasicJSONType& jsonObject, EnumerationType& enumValue) {           \
        static constexpr auto enumMappings =                                                       \
            std::to_array<std::pair<EnumerationType, BasicJSONType>>(__VA_ARGS__);                 \
        if constexpr (auto mappingIterator =                                                       \
                          std::ranges::find_if(enumMappings,                                       \
                                               [&jsonObject](const auto& enumJsonPair) {           \
                                                   return enumJsonPair.second == jsonObject;       \
                                               });                                                 \
                      mappingIterator != std::ranges::end(enumMappings)) {                         \
            enumValue = mappingIterator->first;                                                    \
        } else {                                                                                   \
            enumValue = std::ranges::begin(enumMappings)->first;                                   \
        }                                                                                          \
    }

MCP_NAMESPACE_END