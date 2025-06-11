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

    ParseResult(bool InSuccess) : Success(InSuccess) {}
    ParseResult(const TOutput& InValue) : Success(true), Value(InValue) {}
    ParseResult(const string& InError) : Success(false), Error(InError) {}
};

template <typename TOutput, typename TDef, typename TInput> class MCP_Type {
  public:
    TDef Definition;

    virtual ParseResult<TOutput, TInput> Parse(const ParseContext& InInput) = 0;
    virtual ~MCP_Type() = default;

  protected:
    ParseContext ProcessInputParams(const ParseContext& InInput);
};

template <typename T> class Completable : public MCP_Type<T, CompletableDef<T>, T> {
  public:
    // Delegate parsing to the wrapped type (like original _parse method)
    ParseResult<T, T> Parse(const ParseContext& InInput) override;

    // Return the wrapped type (like original unwrap method)
    shared_ptr<MCP_Type<T, TypeDef, T>> Unwrap();

    vector<T> GetCompletions(const T& InValue);

    future<vector<T>> GetAsyncCompletions(const T& InValue);

    // Like original static create method
    static shared_ptr<Completable<T>> Create(shared_ptr<MCP_Type<T, TypeDef, T>> InType,
                                             const CreateParams<T>& InParams);
};

/**
 * Wraps an MCP type to provide autocompletion capabilities.
 * Equivalent to original completable<T extends ZodTypeAny>(schema: T, complete:
 * CompleteCallback<T>)
 */
template <typename T>
shared_ptr<Completable<T>> CreateCompletable(shared_ptr<MCP_Type<T, TypeDef, T>> InSchema,
                                             CompleteCallback<T> InComplete);

// Template implementations (must be in header)

// Like original processCreateParams function
template <typename T> ProcessedCreateParams ProcessCreateParams(const CreateParams<T>& InParams) {
    ProcessedCreateParams Result;

    if (InParams.ErrorMap.has_value()
        && (params.InvalidTypeError.has_value() || params.RequiredError.has_value())) {
        throw runtime_error("Can't use \"InvalidTypeError\" or \"RequiredError\" in conjunction "
                            "with custom error map.");
    }

    if (params.ErrorMap.has_value()) {
        result.ErrorMap = params.ErrorMap;
        result.Description = params.Description.value_or(MSG_NULL);
        return result;
    }

    // Create custom error map like original
    auto CustomMap = [InParams](const string& IssueCode, const ParseContext& Context) -> string {
        const string& Message = InParams.Message.value_or(MSG_NULL);

        if (IssueCode == "invalid_enum_value") {
            return !Message.empty() ? Message : "Invalid enum value";
        }
        if (Context.Data.is_null()) {
            return !Message.empty() ? Message
                                    : InParams.RequiredError.value_or("Required field missing");
        }
        if (IssueCode != "invalid_type") { return "Validation error"; }
        return !Message.empty() ? Message : InParams.InvalidTypeError.value_or("Invalid type");
    };

    Result.ErrorMap = CustomMap;
    Result.Description = InParams.Description.value_or(MSG_NULL);
    return Result;
}

// Delegate parsing to the wrapped type (like original _parse method)
template <typename T> ParseResult<T, T> Completable<T>::Parse(const ParseContext& InInput) {
    auto Context = this->ProcessInputParams(InInput);
    if (this->Definition.Type) { return this->Definition.Type->Parse(Context); }

    // Fallback if no wrapped type
    try {
        T Value = Context.Data.template get<T>();
        return ParseResult<T, T>(Value);
    } catch (const exception& Exception) {
        return ParseResult<T, T>(string("Parse error: ") + Exception.what());
    }
}

// Return the wrapped type (like original unwrap method)
template <typename T> shared_ptr<MCP_Type<T, TypeDef, T>> Completable<T>::Unwrap() {
    return this->Definition.Type;
}

template <typename T> vector<T> Completable<T>::GetCompletions(const T& InValue) {
    if (this->Definition.Complete) {
        auto Result = this->Definition.Complete(InValue);

        // Handle variant return type
        if (std::holds_alternative<vector<T>>(Result)) {
            return std::get<vector<T>>(Result);
        } else {
            // If it's a future, wait for it
            auto FutureResult = std::get<future<vector<T>>>(Result);
            return FutureResult.get();
        }
    }
    return vector<T>{};
}

template <typename T> future<vector<T>> Completable<T>::GetAsyncCompletions(const T& InValue) {
    if (this->Definition.Complete) {
        auto Result = this->Definition.Complete(InValue);

        if (std::holds_alternative<future<vector<T>>>(Result)) {
            return std::get<future<vector<T>>>(Result);
        } else {
            // Wrap synchronous result in future
            promise<vector<T>> Promise;
            future<vector<T>> FutureResult = Promise.get_future();
            Promise.set_value(std::get<vector<T>>(Result));
            return FutureResult;
        }
    }

    promise<vector<T>> Promise;
    future<vector<T>> FutureResult = Promise.get_future();
    Promise.set_value(vector<T>{});
    return FutureResult;
}

// Like original static create method
template <typename T>
shared_ptr<Completable<T>> Completable<T>::Create(shared_ptr<MCP_Type<T, TypeDef, T>> InType,
                                                  const CreateParams<T>& InParams) {
    auto Completable = make_shared<Completable<T>>();
    Completable->Definition.Type = InType;
    Completable->Definition.Complete = InParams.Complete;

    // Process create params like original processCreateParams
    auto ProcessedParams = ProcessCreateParams(InParams);
    Completable->Definition.Description = ProcessedParams.Description;
    Completable->Definition.ErrorMessage = ProcessedParams.ErrorMessage;

    return Completable;
}

/**
 * Wraps an MCP type to provide autocompletion capabilities.
 * Equivalent to original completable<T extends ZodTypeAny>(schema: T, complete:
 * CompleteCallback<T>)
 */
template <typename T>
shared_ptr<Completable<T>> CreateCompletable(shared_ptr<MCP_Type<T, TypeDef, T>> InSchema,
                                             CompleteCallback<T> InComplete) {
    CreateParams<T> Params;
    Params.Complete = InComplete;
    // Copy schema definition like original: { ...schema._def, complete }
    Params.Description = InSchema->Definition.Description;

    return Completable<T>::Create(InSchema, Params);
}

MCP_NAMESPACE_END