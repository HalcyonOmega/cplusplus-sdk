#include "JSONSchemaValidator.h"

#include <algorithm>
#include <ranges>
#include <string>

MCP_NAMESPACE_BEGIN

JSONSchemaValidator::ValidationResult
JSONSchemaValidator::ValidateAgainstSchema(const JSONData& InData, const JSONSchema& InSchema) {
    return ValidateRecursive(InData, InSchema, "");
}

JSONSchemaValidator::ValidationResult
JSONSchemaValidator::ValidateRecursive(const JSONData& InData, const JSONSchema& InSchema,
                                       std::string_view InPath) {
    ValidationResult result;

    // Check basic type matching
    if (!IsValidType(InData, InSchema.Type)) {
        const std::string path = InPath.empty() ? "root" : std::string(InPath);
        result.AddError("Type mismatch at " + path + ": expected '" + InSchema.Type + "', got '"
                        + GetJSONType(InData) + "'");
        return result;
    }

    // Type-specific validation
    if (InSchema.Type == "object") {
        if (auto objectResult = ValidateObjectType(InData, InSchema); !objectResult.IsValid) {
            for (const auto& error : objectResult.Errors) {
                result.AddError(InPath.empty() ? error : std::string(InPath) + "." + error);
            }
        }
    } else if (InSchema.Type == "array") {
        if (auto arrayResult = ValidateArrayType(InData, InSchema); !arrayResult.IsValid) {
            for (const auto& error : arrayResult.Errors) {
                result.AddError(InPath.empty() ? error : std::string(InPath) + "." + error);
            }
        }
    } else if (InSchema.Type == "string") {
        if (auto stringResult = ValidateStringType(InData, InSchema); !stringResult.IsValid) {
            for (const auto& error : stringResult.Errors) {
                result.AddError(InPath.empty() ? error : std::string(InPath) + "." + error);
            }
        }
    } else if (InSchema.Type == "number" || InSchema.Type == "integer") {
        if (auto numberResult = ValidateNumberType(InData, InSchema); !numberResult.IsValid) {
            for (const auto& error : numberResult.Errors) {
                result.AddError(InPath.empty() ? error : std::string(InPath) + "." + error);
            }
        }
    } else if (InSchema.Type == "boolean") {
        if (auto boolResult = ValidateBooleanType(InData, InSchema); !boolResult.IsValid) {
            for (const auto& error : boolResult.Errors) {
                result.AddError(InPath.empty() ? error : std::string(InPath) + "." + error);
            }
        }
    }

    return result;
}

JSONSchemaValidator::ValidationResult
JSONSchemaValidator::ValidateObjectType(const JSONData& InData, const JSONSchema& InSchema) {
    ValidationResult result;

    if (!InData.is_object()) {
        result.AddError("Expected object type");
        return result;
    }

    // Check required properties
    if (InSchema.Required.has_value()) {
        for (const auto& requiredProp : *InSchema.Required) {
            if (!InData.contains(requiredProp)) {
                result.AddError("Missing required property: '" + requiredProp + "'");
            }
        }
    }

    // Validate properties if schema is provided
    if (InSchema.Properties.has_value()) {
        for (const auto& [key, value] : InData.items()) {
            if (InSchema.Properties->contains(key)) {
                // For simplicity, we'll do basic validation here
                // In a full implementation, we'd recursively validate nested schemas
                const auto& propSchema = InSchema.Properties->at(key);

                // Basic type checking for property values
                if (propSchema.contains("type")) {
                    const std::string expectedType = propSchema.at("type").get<std::string>();
                    if (!IsValidType(value, expectedType)) {
                        result.AddError("Property '" + key + "': expected type '" + expectedType
                                        + "', got '" + GetJSONType(value) + "'");
                    }
                }
            }
        }
    }

    return result;
}

JSONSchemaValidator::ValidationResult
JSONSchemaValidator::ValidateArrayType(const JSONData& InData, const JSONSchema& /*InSchema*/) {
    ValidationResult result;

    if (!InData.is_array()) {
        result.AddError("Expected array type");
        return result;
    }

    // Additional array validation could be added here
    // (e.g., minItems, maxItems, uniqueItems, items schema)

    return result;
}

JSONSchemaValidator::ValidationResult
JSONSchemaValidator::ValidateStringType(const JSONData& InData, const JSONSchema& /*InSchema*/) {
    ValidationResult result;

    if (!InData.is_string()) {
        result.AddError("Expected string type");
        return result;
    }

    // Additional string validation could be added here
    // (e.g., minLength, maxLength, pattern)

    return result;
}

JSONSchemaValidator::ValidationResult
JSONSchemaValidator::ValidateNumberType(const JSONData& InData, const JSONSchema& InSchema) {
    ValidationResult result;

    if (!InData.is_number()) {
        result.AddError("Expected number type");
        return result;
    }

    // For integer type, ensure it's actually an integer
    if (InSchema.Type == "integer" && !InData.is_number_integer()) {
        result.AddError("Expected integer type");
    }

    // Additional number validation could be added here
    // (e.g., minimum, maximum, multipleOf)

    return result;
}

JSONSchemaValidator::ValidationResult
JSONSchemaValidator::ValidateBooleanType(const JSONData& InData, const JSONSchema& /*InSchema*/) {
    ValidationResult result;

    if (!InData.is_boolean()) { result.AddError("Expected boolean type"); }

    return result;
}

bool JSONSchemaValidator::IsValidType(const JSONData& InData, std::string_view InType) {
    if (InType == "object") { return InData.is_object(); }
    if (InType == "array") { return InData.is_array(); }
    if (InType == "string") { return InData.is_string(); }
    if (InType == "number") { return InData.is_number(); }
    if (InType == "integer") { return InData.is_number_integer(); }
    if (InType == "boolean") { return InData.is_boolean(); }
    if (InType == "null") { return InData.is_null(); }

    return false;
}

std::string JSONSchemaValidator::GetJSONType(const JSONData& InData) {
    if (InData.is_object()) { return "object"; }
    if (InData.is_array()) { return "array"; }
    if (InData.is_string()) { return "string"; }
    if (InData.is_number_integer()) { return "integer"; }
    if (InData.is_number()) { return "number"; }
    if (InData.is_boolean()) { return "boolean"; }
    if (InData.is_null()) { return "null"; }

    return "unknown";
}

MCP_NAMESPACE_END