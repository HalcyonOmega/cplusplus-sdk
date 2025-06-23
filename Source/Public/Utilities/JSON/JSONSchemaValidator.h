#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

// JSON Schema Validator for MCP Tool Input Validation
// Following MCP Specification (2025-03-26)
class JSONSchemaValidator {
  public:
    // Validation result
    struct ValidationResult {
        bool IsValid{true};
        std::vector<std::string> Errors;

        void AddError(const std::string& InError) {
            IsValid = false;
            Errors.push_back(InError);
        }
    };

    // Main validation method
    [[nodiscard]] static ValidationResult ValidateAgainstSchema(const JSONValue& InData,
                                                                const JSONSchema& InSchema);

    // Type-specific validation methods
    [[nodiscard]] static ValidationResult ValidateObjectType(const JSONValue& InData,
                                                             const JSONSchema& InSchema);
    [[nodiscard]] static ValidationResult ValidateArrayType(const JSONValue& InData,
                                                            const JSONSchema& InSchema);
    [[nodiscard]] static ValidationResult ValidateStringType(const JSONValue& InData,
                                                             const JSONSchema& InSchema);
    [[nodiscard]] static ValidationResult ValidateNumberType(const JSONValue& InData,
                                                             const JSONSchema& InSchema);
    [[nodiscard]] static ValidationResult ValidateBooleanType(const JSONValue& InData,
                                                              const JSONSchema& InSchema);

    // Extended validation methods for advanced JSON Schema features
    [[nodiscard]] static ValidationResult ValidateAnyOf(const JSONValue& InData,
                                                        const std::vector<JSONSchema>& InSchemas);
    [[nodiscard]] static ValidationResult ValidateOneOf(const JSONValue& InData,
                                                        const std::vector<JSONSchema>& InSchemas);
    [[nodiscard]] static ValidationResult ValidateAllOf(const JSONValue& InData,
                                                        const std::vector<JSONSchema>& InSchemas);
    [[nodiscard]] static ValidationResult ValidateNot(const JSONValue& InData,
                                                      const JSONSchema& InSchema);
    [[nodiscard]] static ValidationResult
    ValidateConditional(const JSONValue& InData, const JSONSchema& InIfSchema,
                        const std::optional<JSONSchema>& InThenSchema,
                        const std::optional<JSONSchema>& InElseSchema);

    // Helper methods
    [[nodiscard]] static bool IsValidType(const JSONValue& InData, std::string_view InType);
    [[nodiscard]] static std::string GetJSONType(const JSONValue& InData);

    // Performance optimization - validation caching
    struct CacheKey {
        std::string DataHash;
        std::string SchemaHash;

        bool operator==(const CacheKey& InOther) const {
            return DataHash == InOther.DataHash && SchemaHash == InOther.SchemaHash;
        }
    };

    struct CacheKeyHash {
        [[nodiscard]] std::size_t operator()(const CacheKey& InKey) const {
            return std::hash<std::string>{}(InKey.DataHash + InKey.SchemaHash);
        }
    };

    static std::unordered_map<CacheKey, ValidationResult, CacheKeyHash> m_ValidationCache;
    static std::mutex m_CacheMutex;
    static constexpr size_t MAX_CACHE_SIZE{10000};

    static std::string HashJSON(const JSONValue& InJSON);
    static std::string HashSchema(const JSONSchema& InSchema);
    static CacheKey MakeCacheKey(const JSONValue& InData, const JSONSchema& InSchema);

  private:
    // Private helper for recursive validation
    [[nodiscard]] static ValidationResult ValidateRecursive(const JSONValue& InData,
                                                            const JSONSchema& InSchema,
                                                            std::string_view InPath = "");

    // Advanced constraint validation
    [[nodiscard]] static ValidationResult ValidateStringConstraints(const JSONValue& InData,
                                                                    const JSONSchema& InSchema,
                                                                    std::string_view InPath);
    [[nodiscard]] static ValidationResult ValidateNumberConstraints(const JSONValue& InData,
                                                                    const JSONSchema& InSchema,
                                                                    std::string_view InPath);
    [[nodiscard]] static ValidationResult ValidateArrayConstraints(const JSONValue& InData,
                                                                   const JSONSchema& InSchema,
                                                                   std::string_view InPath);
    [[nodiscard]] static ValidationResult ValidateObjectConstraints(const JSONValue& InData,
                                                                    const JSONSchema& InSchema,
                                                                    std::string_view InPath);
};

MCP_NAMESPACE_END