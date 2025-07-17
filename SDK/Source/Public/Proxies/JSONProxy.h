#pragma once

#include <memory>
#include <optional>
#include <type_traits>

#include "../CoreSDK/Common/Macros.h"
#include "../Utilities/ThirdParty/json.hpp"

// Define a template for serializing/deserializing any std::unique_ptr
template <typename T> struct nlohmann::adl_serializer<std::unique_ptr<T>>
{
	static void to_json(json& j, const std::unique_ptr<T>& ptr)
	{
		if (ptr)
		{
			j = *ptr; // If the pointer is not null, serialize the object it points to
		}
		else
		{
			j = nullptr; // If the pointer is null, serialize it as a JSON null
		}
	}

	static void from_json(const json& j, std::unique_ptr<T>& ptr)
	{
		if (j.is_null())
		{
			ptr = nullptr; // If the JSON is null, make the pointer null
		}
		else
		{
			ptr = std::make_unique<T>(
				j.get<T>()); // If the JSON is not null, create a new object and deserialize into it
		}
	}
};

MCP_NAMESPACE_BEGIN

// Concept: checks if a type is a nlohmann::basic_json specialization
template <typename T>
concept IsBasicJSON = nlohmann::detail::is_basic_json<T>::value;

// Concept: checks if a type is a specialization of std::optional
template <typename T>
concept IsOptional = requires { typename T::value_type; }
	&& requires { requires std::same_as<T, std::optional<typename T::value_type>>; };

// -----------------------------------------------------------------------------
// Custom JSON serialization macros with key renaming support
// -----------------------------------------------------------------------------
#define JSON_EXPAND(x) x

// JSON Key mapping system with default member name and optional override
#define JSON_KEY(...) JSON_EXPAND(JSON_KEY_OVERLOAD(__VA_ARGS__, JSON_KEY_3, JSON_KEY_2)(__VA_ARGS__))
#define JSON_KEY_OVERLOAD(_1, _2, _3, NAME, ...) NAME

// JSON_KEY with default JSON key (uses member name)
#define JSON_KEY_2(VarName, Member) JSON_KEY_3(VarName, Member, #Member)

// JSON_KEY with custom JSON key
#define JSON_KEY_3(VarName, Member, JSONKey)                    \
	static constexpr struct VarName##_t                         \
	{                                                           \
		static constexpr const char* json_key = JSONKey;        \
		static void to_json(auto& JSON_J, const auto& JSON_T)   \
		{                                                       \
			if constexpr (IsOptional<decltype(JSON_T.Member)>)  \
			{                                                   \
				if (JSON_T.Member.has_value())                  \
				{                                               \
					JSON_J[json_key] = JSON_T.Member;           \
				}                                               \
			}                                                   \
			else                                                \
			{                                                   \
				JSON_J[json_key] = JSON_T.Member;               \
			}                                                   \
		}                                                       \
		static void from_json(const auto& JSON_J, auto& JSON_T) \
		{                                                       \
			if constexpr (IsOptional<decltype(JSON_T.Member)>)  \
			{                                                   \
				if (JSON_J.contains(json_key))                  \
				{                                               \
					JSON_J.at(json_key).get_to(JSON_T.Member);  \
				}                                               \
			}                                                   \
			else                                                \
			{                                                   \
				JSON_J.at(json_key).get_to(JSON_T.Member);      \
			}                                                   \
		}                                                       \
	} VarName{};

// Usage macros for JSON_KEY tokens
#define USE_JKEY_TO_JSON(KeyToken) KeyToken.to_json(JSON_J, JSON_T);
#define USE_JKEY_FROM_JSON(KeyToken) KeyToken.from_json(JSON_J, JSON_T);

// Combined DEFINE_TYPE_JSON that works with JSON_KEY tokens
#define DEFINE_TYPE_JSON(Type, ...)                       \
	DEFINE_TO_JSON(Type, APPLY_TO_JSON_KEYS(__VA_ARGS__)) \
	DEFINE_FROM_JSON(Type, APPLY_FROM_JSON_KEYS(__VA_ARGS__))

#define DEFINE_TYPE_JSON_DERIVED(Type, BaseType, ...)                       \
	DEFINE_TO_JSON_DERIVED(Type, BaseType, APPLY_TO_JSON_KEYS(__VA_ARGS__)) \
	DEFINE_FROM_JSON_DERIVED(Type, BaseType, APPLY_FROM_JSON_KEYS(__VA_ARGS__))

// Helper macros to apply operations to all JSON_KEY tokens
#define APPLY_TO_JSON_KEYS(...) FOR_EACH_JSON_KEY(USE_JKEY_TO_JSON, __VA_ARGS__)
#define APPLY_FROM_JSON_KEYS(...) FOR_EACH_JSON_KEY(USE_JKEY_FROM_JSON, __VA_ARGS__)

// Variadic macro processor for JSON_KEY tokens (simplified)
#define FOR_EACH_JSON_KEY_1(Macro, x) Macro(x)
#define FOR_EACH_JSON_KEY_2(Macro, x, ...) Macro(x) FOR_EACH_JSON_KEY_1(Macro, __VA_ARGS__)
#define FOR_EACH_JSON_KEY_3(Macro, x, ...) Macro(x) FOR_EACH_JSON_KEY_2(Macro, __VA_ARGS__)
#define FOR_EACH_JSON_KEY_4(Macro, x, ...) Macro(x) FOR_EACH_JSON_KEY_3(Macro, __VA_ARGS__)
#define FOR_EACH_JSON_KEY_5(Macro, x, ...) Macro(x) FOR_EACH_JSON_KEY_4(Macro, __VA_ARGS__)
#define FOR_EACH_JSON_KEY_6(Macro, x, ...) Macro(x) FOR_EACH_JSON_KEY_5(Macro, __VA_ARGS__)
#define FOR_EACH_JSON_KEY_7(Macro, x, ...) Macro(x) FOR_EACH_JSON_KEY_6(Macro, __VA_ARGS__)
#define FOR_EACH_JSON_KEY_8(Macro, x, ...) Macro(x) FOR_EACH_JSON_KEY_7(Macro, __VA_ARGS__)

#define FOR_EACH_JSON_KEY(Macro, ...)        \
	FOR_EACH_JSON_KEY_GET_MACRO(__VA_ARGS__, \
		FOR_EACH_JSON_KEY_8,                 \
		FOR_EACH_JSON_KEY_7,                 \
		FOR_EACH_JSON_KEY_6,                 \
		FOR_EACH_JSON_KEY_5,                 \
		FOR_EACH_JSON_KEY_4,                 \
		FOR_EACH_JSON_KEY_3,                 \
		FOR_EACH_JSON_KEY_2,                 \
		FOR_EACH_JSON_KEY_1)(Macro, __VA_ARGS__)

#define FOR_EACH_JSON_KEY_GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME

// Macro for custom key mapping - generates the functions directly
#define DEFINE_TO_JSON(Type, ...)                                  \
	template <typename BasicJSONType>                              \
		requires IsBasicJSON<BasicJSONType>                        \
	friend void to_json(BasicJSONType& JSON_J, const Type& JSON_T) \
	{                                                              \
		__VA_ARGS__                                                \
	}

#define DEFINE_FROM_JSON(Type, ...)                                  \
	template <typename BasicJSONType>                                \
		requires IsBasicJSON<BasicJSONType>                          \
	friend void from_json(const BasicJSONType& JSON_J, Type& JSON_T) \
	{                                                                \
		__VA_ARGS__                                                  \
	}

#define DEFINE_TO_JSON_DERIVED(Type, BaseType, ...)                \
	template <typename BasicJSONType>                              \
		requires IsBasicJSON<BasicJSONType>                        \
	friend void to_json(BasicJSONType& JSON_J, const Type& JSON_T) \
	{                                                              \
		to_json(JSON_J, static_cast<const BaseType&>(JSON_T));     \
		__VA_ARGS__                                                \
	}

#define DEFINE_FROM_JSON_DERIVED(Type, BaseType, ...)                \
	template <typename BasicJSONType>                                \
		requires IsBasicJSON<BasicJSONType>                          \
	friend void from_json(const BasicJSONType& JSON_J, Type& JSON_T) \
	{                                                                \
		from_json(JSON_J, static_cast<BaseType&>(JSON_T));           \
		__VA_ARGS__                                                  \
	}

// -----------------------------------------------------------------------------
// Enum serialization with C++20 features and longer parameter names
// -----------------------------------------------------------------------------

// C++20 concept to check if a type is an enum
template <typename EnumerationType>
concept IsEnumType = std::is_enum_v<EnumerationType>;

#define DEFINE_ENUM_JSON(EnumerationType, ...)                                                                      \
	template <typename BasicJSONType>                                                                               \
		requires IsBasicJSON<BasicJSONType> && IsEnumType<EnumerationType>                                          \
	inline void to_json(BasicJSONType& jsonObject, const EnumerationType& enumValue)                                \
	{                                                                                                               \
		static constexpr auto enumMappings = std::to_array<std::pair<EnumerationType, BasicJSONType>>(__VA_ARGS__); \
		if constexpr (auto mappingIterator = std::ranges::find_if(enumMappings,                                     \
						  [enumValue](const auto& enumJsonPair) { return enumJsonPair.first == enumValue; });       \
			mappingIterator != std::ranges::end(enumMappings))                                                      \
		{                                                                                                           \
			jsonObject = mappingIterator->second;                                                                   \
		}                                                                                                           \
		else                                                                                                        \
		{                                                                                                           \
			jsonObject = std::ranges::begin(enumMappings)->second;                                                  \
		}                                                                                                           \
	}                                                                                                               \
	template <typename BasicJSONType>                                                                               \
		requires IsBasicJSON<BasicJSONType> && IsEnumType<EnumerationType>                                          \
	inline void from_json(const BasicJSONType& jsonObject, EnumerationType& enumValue)                              \
	{                                                                                                               \
		static constexpr auto enumMappings = std::to_array<std::pair<EnumerationType, BasicJSONType>>(__VA_ARGS__); \
		if constexpr (auto mappingIterator = std::ranges::find_if(enumMappings,                                     \
						  [&jsonObject](const auto& enumJsonPair) { return enumJsonPair.second == jsonObject; });   \
			mappingIterator != std::ranges::end(enumMappings))                                                      \
		{                                                                                                           \
			enumValue = mappingIterator->first;                                                                     \
		}                                                                                                           \
		else                                                                                                        \
		{                                                                                                           \
			enumValue = std::ranges::begin(enumMappings)->first;                                                    \
		}                                                                                                           \
	}

using JSONData = nlohmann::json;

// JSON Schema
struct JSONSchema
{
	std::string Type{ "object" };
	std::optional<std::unordered_map<std::string, JSONData>> Properties;
	std::optional<std::vector<std::string>> Required;
	std::optional<JSONData> AdditionalProperties;

	JSON_KEY(TYPEKEY, Type, "type")
	JSON_KEY(PROPERTIESKEY, Properties, "properties")
	JSON_KEY(REQUIREDKEY, Required, "required")
	JSON_KEY(ADDITIONALPROPERTIESKEY, AdditionalProperties, "additionalProperties")

	DEFINE_TYPE_JSON(JSONSchema, TYPEKEY, PROPERTIESKEY, REQUIREDKEY, ADDITIONALPROPERTIESKEY)
};

MCP_NAMESPACE_END