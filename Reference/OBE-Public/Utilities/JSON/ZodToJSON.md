# Zod to C++ nlohmann JSON Conversion Guide

## Overview

This guide provides templates and patterns for converting [Zod](https://zod.dev/) TypeScript schemas to C++ validation using the [nlohmann JSON](https://github.com/nlohmann/JSON) library.

## Basic Type Conversions

### Primitive Types

| Zod Type | C++ Type | nlohmann JSON Check |
|----------|----------|---------------------|
| `z.string()` | `std::string` | `JSON.is_string()` |
| `z.number()` | `double` | `JSON.is_number()` |
| `z.boolean()` | `bool` | `JSON.is_boolean()` |
| `z.null()` | `nullptr` | `JSON.is_null()` |

### Arrays

**Zod:**

```typescript
const schema = z.array(z.string());
```

**C++:**

```cpp
std::vector<std::string> parseStringArray(const nlohmann::JSON& JSON) {
    if (!JSON.is_array()) {
        throw std::invalid_argument("Expected array");
    }

    std::vector<std::string> result;
    for (const auto& item : JSON) {
        if (!item.is_string()) {
            throw std::invalid_argument("Array item must be string");
        }
        result.push_back(item.get<std::string>());
    }
    return result;
}
```

### Objects

**Zod:**

```typescript
const UserSchema = z.object({
    id: z.number(),
    name: z.string(),
    email: z.string().email(),
    age: z.number().optional()
});
```

**C++:**

```cpp
struct User {
    int id;
    std::string name;
    std::string email;
    std::optional<int> age;

    static User fromJSON(const nlohmann::JSON& JSON) {
        validateUserSchema(JSON);

        User user;
        user.id = JSON[MSG_KEY_ID].get<int>();
        user.name = JSON["name"].get<std::string>();
        user.email = JSON["email"].get<std::string>();

        if (JSON.contains("age") && !JSON["age"].is_null()) {
            user.age = JSON["age"].get<int>();
        }

        return user;
    }

private:
    static void validateUserSchema(const nlohmann::JSON& JSON) {
        if (!JSON.is_object()) {
            throw std::invalid_argument("Expected object");
        }

        // Required fields
        if (!JSON.contains(MSG_KEY_ID) || !JSON[MSG_KEY_ID].is_number_integer()) {
            throw std::invalid_argument("Missing or invalid 'id' field");
        }

        if (!JSON.contains("name") || !JSON["name"].is_string()) {
            throw std::invalid_argument("Missing or invalid 'name' field");
        }

        if (!JSON.contains("email") || !JSON["email"].is_string()) {
            throw std::invalid_argument("Missing or invalid 'email' field");
        }

        // Optional fields
        if (JSON.contains("age") && !JSON["age"].is_null() && !JSON["age"].is_number_integer()) {
            throw std::invalid_argument("Invalid 'age' field");
        }
    }
};
```

## Advanced Patterns

### Union Types

**Zod:**

```typescript
const schema = z.union([z.string(), z.number()]);
```

**C++:**

```cpp
#include <variant>

using StringOrNumber = std::variant<std::string, double>;

StringOrNumber parseStringOrNumber(const nlohmann::JSON& JSON) {
    if (JSON.is_string()) {
        return JSON.get<std::string>();
    } else if (JSON.is_number()) {
        return JSON.get<double>();
    } else {
        throw std::invalid_argument("Expected string or number");
    }
}
```

### Enums

**Zod:**

```typescript
const StatusSchema = z.enum(["pending", "completed", "failed"]);
```

**C++:**

```cpp
enum class Status {
    Pending,
    Completed,
    Failed
};

Status parseStatus(const nlohmann::JSON& JSON) {
    if (!JSON.is_string()) {
        throw std::invalid_argument("Status must be string");
    }
    std::string value = JSON.get<std::string>();
    if (value == "pending") return Status::Pending;
    if (value == "completed") return Status::Completed;
    if (value == "failed") return Status::Failed;

    throw std::invalid_argument("Invalid status value: " + value);
}

// Helper for serialization
std::string statusToString(Status status) {
    switch (status) {
        case Status::Pending: return "pending";
        case Status::Completed: return "completed";
        case Status::Failed: return "failed";
        default: throw std::invalid_argument("Unknown status");
    }
}
```

### Nested Objects

**Zod:**

```typescript
const AddressSchema = z.object({
    street: z.string(),
    city: z.string(),
    zipCode: z.string()
});

const PersonSchema = z.object({
    name: z.string(),
    address: AddressSchema
});
```

**C++:**

```cpp
struct Address {
    std::string street;
    std::string city;
    std::string zipCode;

    static Address fromJSON(const nlohmann::JSON& JSON) {
        if (!JSON.is_object()) {
            throw std::invalid_argument("Address must be object");
        }

        Address addr;
        addr.street = JSON.at("street").get<std::string>();
        addr.city = JSON.at("city").get<std::string>();
        addr.zipCode = JSON.at("zipCode").get<std::string>();
        return addr;
    }
};

struct Person {
    std::string name;
    Address address;

    static Person fromJSON(const nlohmann::JSON& JSON) {
        if (!JSON.is_object()) {
            throw std::invalid_argument("Person must be object");
        }

        Person person;
        person.name = JSON.at("name").get<std::string>();
        person.address = Address::fromJSON(JSON.at("address"));
        return person;
    }
};
```

## Validation Helper Templates

### Generic Validator Class

```cpp
template<typename T>
class JSON_Validator {
public:
    using ValidatorFunc = std::function<void(const nlohmann::JSON&)>;
    using ParserFunc = std::function<T(const nlohmann::JSON&)>;

    JSON_Validator(ValidatorFunc validator, ParserFunc parser)
        : m_validator(validator), m_parser(parser) {}

    T parse(const nlohmann::JSON& JSON) const {
        m_validator(JSON);
        return m_parser(JSON);
    }

    bool isValid(const nlohmann::JSON& JSON) const {
        try {
            m_validator(JSON);
            return true;
        } catch (...) {
            return false;
        }
    }

private:
    ValidatorFunc m_validator;
    ParserFunc m_parser;
};
```

### Field Validation Helpers

```cpp
namespace JSON_Validation {
    void requireField(const nlohmann::JSON& JSON, const std::string& field) {
        if (!JSON.contains(field)) {
            throw std::invalid_argument("Missing required field: " + field);
        }
    }

    void requireString(const nlohmann::JSON& JSON, const std::string& field) {
        requireField(JSON, field);
        if (!JSON[field].is_string()) {
            throw std::invalid_argument("Field '" + field + "' must be string");
        }
    }

    void requireNumber(const nlohmann::JSON& JSON, const std::string& field) {
        requireField(JSON, field);
        if (!JSON[field].is_number()) {
            throw std::invalid_argument("Field '" + field + "' must be number");
        }
    }

    void requireArray(const nlohmann::JSON& JSON, const std::string& field) {
        requireField(JSON, field);
        if (!JSON[field].is_array()) {
            throw std::invalid_argument("Field '" + field + "' must be array");
        }
    }

    template<typename T>
    std::optional<T> getOptional(const nlohmann::JSON& JSON, const std::string& field) {
        if (!JSON.contains(field) || JSON[field].is_null()) {
            return std::nullopt;
        }
        return JSON[field].get<T>();
    }
}
```

## Best Practices

### 1. Error Handling

```cpp
// Use specific exception types
class ValidationError : public std::runtime_error {
public:
    ValidationError(const std::string& msg) : std::runtime_error(msg) {}
};

class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& msg) : std::runtime_error(msg) {}
};
```

### 2. Macro for Boilerplate Reduction

```cpp
#define VALIDATE_REQUIRED_FIELD(JSON, field, type) \
    do { \
        if (!JSON.contains(#field)) { \
            throw ValidationError("Missing required field: " #field); \
        } \
        if (!JSON[#field].is_##type()) { \
            throw ValidationError("Field '" #field "' must be " #type); \
        } \
    } while(0)

// Usage:
// VALIDATE_REQUIRED_FIELD(JSON, name, string);
// VALIDATE_REQUIRED_FIELD(JSON, age, number);
```

### 3. Schema Registration Pattern

```cpp
class SchemaRegistry {
public:
    template<typename T>
    void registerSchema(const std::string& name, JSON_Validator<T> validator) {
        // Implementation for schema registration
    }

    template<typename T>
    T parse(const std::string& schemaName, const nlohmann::JSON& JSON) {
        // Implementation for parsing with registered schema
    }
};
```

## Common Gotchas

1. **Type Coercion**: Zod performs automatic type coercion, C++ doesn't
2. **Optional vs Nullable**: Handle both missing fields and null values
3. **Number Types**: Zod's `number()` can be int or float, be explicit in C++
4. **Error Messages**: Provide context about which field/path failed validation
5. **Performance**: Consider using string views for large JSON parsing
