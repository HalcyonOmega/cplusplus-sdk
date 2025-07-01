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
    [[nodiscard]] static ValidationResult ValidateAgainstSchema(const JSONData& InData,
                                                                const JSONSchema& InSchema);

    // Type-specific validation methods
    [[nodiscard]] static ValidationResult ValidateObjectType(const JSONData& InData,
                                                             const JSONSchema& InSchema);
    [[nodiscard]] static ValidationResult ValidateArrayType(const JSONData& InData,
                                                            const JSONSchema& InSchema);
    [[nodiscard]] static ValidationResult ValidateStringType(const JSONData& InData,
                                                             const JSONSchema& InSchema);
    [[nodiscard]] static ValidationResult ValidateNumberType(const JSONData& InData,
                                                             const JSONSchema& InSchema);
    [[nodiscard]] static ValidationResult ValidateBooleanType(const JSONData& InData,
                                                              const JSONSchema& InSchema);

    // Extended validation methods for advanced JSON Schema features
    [[nodiscard]] static ValidationResult ValidateAnyOf(const JSONData& InData,
                                                        const std::vector<JSONSchema>& InSchemas);
    [[nodiscard]] static ValidationResult ValidateOneOf(const JSONData& InData,
                                                        const std::vector<JSONSchema>& InSchemas);
    [[nodiscard]] static ValidationResult ValidateAllOf(const JSONData& InData,
                                                        const std::vector<JSONSchema>& InSchemas);
    [[nodiscard]] static ValidationResult ValidateNot(const JSONData& InData,
                                                      const JSONSchema& InSchema);
    [[nodiscard]] static ValidationResult
    ValidateConditional(const JSONData& InData, const JSONSchema& InIfSchema,
                        const std::optional<JSONSchema>& InThenSchema,
                        const std::optional<JSONSchema>& InElseSchema);

    // Helper methods
    [[nodiscard]] static bool IsValidType(const JSONData& InData, std::string_view InType);
    [[nodiscard]] static std::string GetJSONType(const JSONData& InData);

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

    static std::string HashJSON(const JSONData& InJSON);
    static std::string HashSchema(const JSONSchema& InSchema);
    static CacheKey MakeCacheKey(const JSONData& InData, const JSONSchema& InSchema);

  private:
    // Private helper for recursive validation
    [[nodiscard]] static ValidationResult ValidateRecursive(const JSONData& InData,
                                                            const JSONSchema& InSchema,
                                                            std::string_view InPath = "");

    // Advanced constraint validation
    [[nodiscard]] static ValidationResult ValidateStringConstraints(const JSONData& InData,
                                                                    const JSONSchema& InSchema,
                                                                    std::string_view InPath);
    [[nodiscard]] static ValidationResult ValidateNumberConstraints(const JSONData& InData,
                                                                    const JSONSchema& InSchema,
                                                                    std::string_view InPath);
    [[nodiscard]] static ValidationResult ValidateArrayConstraints(const JSONData& InData,
                                                                   const JSONSchema& InSchema,
                                                                   std::string_view InPath);
    [[nodiscard]] static ValidationResult ValidateObjectConstraints(const JSONData& InData,
                                                                    const JSONSchema& InSchema,
                                                                    std::string_view InPath);
};

MCP_NAMESPACE_END