#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// TODO: Fix External Ref: ZodTypeAny - Using template parameter T instead
// TODO: Fix External Ref: ZodTypeDef - Using custom TypeDef struct
// TODO: Fix External Ref: ParseInput/ParseReturnType - Using custom Parse types

enum class MCP_TypeKind {
    Completable // Represents "McpCompletable"
};

constexpr string_view GetMCP_TypeKind(MCP_TypeKind Kind);

// Forward declarations
template <typename TOutput, typename TDef, typename TInput = TOutput> class MCP_Type;

template <typename T> class Completable;

// Define ParseContext first before using it
struct ParseContext {
    JSON Data;
    vector<string> Path;
    optional<string> ErrorMessage;
};

// Error map function type (like original ZodErrorMap)
using ErrorMapFunction = function<string(const string& issueCode, const ParseContext& ctx)>;

// Unified callback type that can return either sync or async results (like original)
template <typename T>
using CompleteCallback = function<variant<vector<T>, future<vector<T>>>(const T&)>;

struct TypeDef {
    MCP_TypeKind TypeName;
    string Description;
    optional<string> ErrorMessage;
};

// Create params structure (like original RawCreateParams & { complete: CompleteCallback<T> })
template <typename T> struct CreateParams {
    CompleteCallback<T> Complete;
    optional<ErrorMapFunction> ErrorMap;
    optional<string> InvalidTypeError;
    optional<string> RequiredError;
    optional<string> Description;
    optional<string> Message;
};

// Processed params (like original ProcessedCreateParams)
struct ProcessedCreateParams {
    optional<ErrorMapFunction> ErrorMap;
    string Description;
    optional<string> ErrorMessage;
};

// Like original processCreateParams function - DECLARED EARLY
template <typename T> ProcessedCreateParams ProcessCreateParams(const CreateParams<T>& params);

template <typename T> struct CompletableDef : public TypeDef {
    shared_ptr<MCP_Type<T, TypeDef, T>> Type; // Store the wrapped type like original
    CompleteCallback<T> Complete;

    CompletableDef() {
        TypeName = MCP_TypeKind::Completable;
    }
};

template <typename TOutput, typename TInput> class ParseResult {
  public:
    bool Success;
    optional<TOutput> Value;
    optional<string> Error;

    ParseResult(bool success) : Success(success) {}
    ParseResult(const TOutput& value) : Success(true), Value(value) {}
    ParseResult(const string& error) : Success(false), Error(error) {}
};

template <typename TOutput, typename TDef, typename TInput> class MCP_Type {
  public:
    TDef Definition;

    virtual ParseResult<TOutput, TInput> Parse(const ParseContext& input) = 0;
    virtual ~MCP_Type() = default;

  protected:
    ParseContext ProcessInputParams(const ParseContext& input);
};

template <typename T> class Completable : public MCP_Type<T, CompletableDef<T>, T> {
  public:
    // Delegate parsing to the wrapped type (like original _parse method)
    ParseResult<T, T> Parse(const ParseContext& input) override;

    // Return the wrapped type (like original unwrap method)
    shared_ptr<MCP_Type<T, TypeDef, T>> Unwrap();

    vector<T> GetCompletions(const T& value);

    future<vector<T>> GetAsyncCompletions(const T& value);

    // Like original static create method
    static shared_ptr<Completable<T>> Create(shared_ptr<MCP_Type<T, TypeDef, T>> type,
                                             const CreateParams<T>& params);
};

/**
 * Wraps an MCP type to provide autocompletion capabilities.
 * Equivalent to original completable<T extends ZodTypeAny>(schema: T, complete:
 * CompleteCallback<T>)
 */
template <typename T>
shared_ptr<Completable<T>> CreateCompletable(shared_ptr<MCP_Type<T, TypeDef, T>> schema,
                                             CompleteCallback<T> complete);

// Template implementations (must be in header)

// Like original processCreateParams function
template <typename T> ProcessedCreateParams ProcessCreateParams(const CreateParams<T>& params) {
    ProcessedCreateParams result;

    if (params.ErrorMap.has_value()
        && (params.InvalidTypeError.has_value() || params.RequiredError.has_value())) {
        throw runtime_error("Can't use \"InvalidTypeError\" or \"RequiredError\" in conjunction "
                            "with custom error map.");
    }

    if (params.ErrorMap.has_value()) {
        result.ErrorMap = params.ErrorMap;
        result.Description = params.Description.value_or("");
        return result;
    }

    // Create custom error map like original
    auto customMap = [params](const string& issueCode, const ParseContext& ctx) -> string {
        const string& message = params.Message.value_or("");

        if (issueCode == "invalid_enum_value") {
            return !message.empty() ? message : "Invalid enum value";
        }
        if (ctx.Data.is_null()) {
            return !message.empty() ? message
                                    : params.RequiredError.value_or("Required field missing");
        }
        if (issueCode != "invalid_type") {
            return "Validation error";
        }
        return !message.empty() ? message : params.InvalidTypeError.value_or("Invalid type");
    };

    result.ErrorMap = customMap;
    result.Description = params.Description.value_or("");
    return result;
}

// Delegate parsing to the wrapped type (like original _parse method)
template <typename T> ParseResult<T, T> Completable<T>::Parse(const ParseContext& input) {
    auto ctx = this->ProcessInputParams(input);
    if (this->Definition.Type) {
        return this->Definition.Type->Parse(ctx);
    }

    // Fallback if no wrapped type
    try {
        T value = ctx.Data.template get<T>();
        return ParseResult<T, T>(value);
    } catch (const exception& e) {
        return ParseResult<T, T>(string("Parse error: ") + e.what());
    }
}

// Return the wrapped type (like original unwrap method)
template <typename T> shared_ptr<MCP_Type<T, TypeDef, T>> Completable<T>::Unwrap() {
    return this->Definition.Type;
}

template <typename T> vector<T> Completable<T>::GetCompletions(const T& value) {
    if (this->Definition.Complete) {
        auto result = this->Definition.Complete(value);

        // Handle variant return type
        if (std::holds_alternative<vector<T>>(result)) {
            return std::get<vector<T>>(result);
        } else {
            // If it's a future, wait for it
            auto future_result = std::get<future<vector<T>>>(result);
            return future_result.get();
        }
    }
    return vector<T>{};
}

template <typename T> future<vector<T>> Completable<T>::GetAsyncCompletions(const T& value) {
    if (this->Definition.Complete) {
        auto result = this->Definition.Complete(value);

        if (std::holds_alternative<future<vector<T>>>(result)) {
            return std::get<future<vector<T>>>(result);
        } else {
            // Wrap synchronous result in future
            promise<vector<T>> promise;
            future<vector<T>> future_result = promise.get_future();
            promise.set_value(std::get<vector<T>>(result));
            return future_result;
        }
    }

    promise<vector<T>> promise;
    future<vector<T>> future_result = promise.get_future();
    promise.set_value(vector<T>{});
    return future_result;
}

// Like original static create method
template <typename T>
shared_ptr<Completable<T>> Completable<T>::Create(shared_ptr<MCP_Type<T, TypeDef, T>> type,
                                                  const CreateParams<T>& params) {
    auto completable = make_shared<Completable<T>>();
    completable->Definition.Type = type;
    completable->Definition.Complete = params.Complete;

    // Process create params like original processCreateParams
    auto processedParams = ProcessCreateParams(params);
    completable->Definition.Description = processedParams.Description;
    completable->Definition.ErrorMessage = processedParams.ErrorMessage;

    return completable;
}

/**
 * Wraps an MCP type to provide autocompletion capabilities.
 * Equivalent to original completable<T extends ZodTypeAny>(schema: T, complete:
 * CompleteCallback<T>)
 */
template <typename T>
shared_ptr<Completable<T>> CreateCompletable(shared_ptr<MCP_Type<T, TypeDef, T>> schema,
                                             CompleteCallback<T> complete) {
    CreateParams<T> params;
    params.Complete = complete;
    // Copy schema definition like original: { ...schema._def, complete }
    params.Description = schema->Definition.Description;

    return Completable<T>::Create(schema, params);
}

MCP_NAMESPACE_END