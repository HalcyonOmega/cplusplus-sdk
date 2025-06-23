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
    [[nodiscard]] static ValidationResult ValidateAgainstSchema(const nlohmann::json& InData,
                                                                const JSONSchema& InSchema);

    // Type-specific validation methods
    [[nodiscard]] static ValidationResult ValidateObjectType(const nlohmann::json& InData,
                                                             const JSONSchema& InSchema);
    [[nodiscard]] static ValidationResult ValidateArrayType(const nlohmann::json& InData,
                                                            const JSONSchema& InSchema);
    [[nodiscard]] static ValidationResult ValidateStringType(const nlohmann::json& InData,
                                                             const JSONSchema& InSchema);
    [[nodiscard]] static ValidationResult ValidateNumberType(const nlohmann::json& InData,
                                                             const JSONSchema& InSchema);
    [[nodiscard]] static ValidationResult ValidateBooleanType(const nlohmann::json& InData,
                                                              const JSONSchema& InSchema);

    // Extended validation methods for advanced JSON Schema features
    [[nodiscard]] static ValidationResult ValidateAnyOf(const nlohmann::json& InData,
                                                        const std::vector<JSONSchema>& InSchemas);
    [[nodiscard]] static ValidationResult ValidateOneOf(const nlohmann::json& InData,
                                                        const std::vector<JSONSchema>& InSchemas);
    [[nodiscard]] static ValidationResult ValidateAllOf(const nlohmann::json& InData,
                                                        const std::vector<JSONSchema>& InSchemas);
    [[nodiscard]] static ValidationResult ValidateNot(const nlohmann::json& InData,
                                                      const JSONSchema& InSchema);
    [[nodiscard]] static ValidationResult
    ValidateConditional(const nlohmann::json& InData, const JSONSchema& InIfSchema,
                        const std::optional<JSONSchema>& InThenSchema,
                        const std::optional<JSONSchema>& InElseSchema);

    // Helper methods
    [[nodiscard]] static bool IsValidType(const nlohmann::json& InData, std::string_view InType);
    [[nodiscard]] static std::string GetJSONType(const nlohmann::json& InData);

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

    static std::string HashJSON(const nlohmann::json& InJSON);
    static std::string HashSchema(const JSONSchema& InSchema);
    static CacheKey MakeCacheKey(const nlohmann::json& InData, const JSONSchema& InSchema);

  private:
    // Private helper for recursive validation
    [[nodiscard]] static ValidationResult ValidateRecursive(const nlohmann::json& InData,
                                                            const JSONSchema& InSchema,
                                                            std::string_view InPath = "");

    // Advanced constraint validation
    [[nodiscard]] static ValidationResult ValidateStringConstraints(const nlohmann::json& InData,
                                                                    const JSONSchema& InSchema,
                                                                    std::string_view InPath);
    [[nodiscard]] static ValidationResult ValidateNumberConstraints(const nlohmann::json& InData,
                                                                    const JSONSchema& InSchema,
                                                                    std::string_view InPath);
    [[nodiscard]] static ValidationResult ValidateArrayConstraints(const nlohmann::json& InData,
                                                                   const JSONSchema& InSchema,
                                                                   std::string_view InPath);
    [[nodiscard]] static ValidationResult ValidateObjectConstraints(const nlohmann::json& InData,
                                                                    const JSONSchema& InSchema,
                                                                    std::string_view InPath);
};

MCP_NAMESPACE_END