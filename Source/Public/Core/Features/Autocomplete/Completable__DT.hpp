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

// Unified callback type that can return either sync or async results (like original)
template <typename T>
using CompleteCallback = function<variant<vector<T>, future<vector<T>>>(const T&)>;

// Error map function type (like original ZodErrorMap)
using ErrorMapFunction = function<string(const string& issueCode, const ParseContext& ctx)>;

struct ParseContext {
    JSON Data;
    vector<string> Path;
    optional<string> ErrorMessage;
};

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
template <typename T> ProcessedCreateParams ProcessCreateParams(const CreateParams<T>& InParams);

template <typename T> struct CompletableDef : public TypeDef {
    shared_ptr<MCP_Type<T, TypeDef, T>> Type; // Store the wrapped type like original
    CompleteCallback<T> Complete;

    CompletableDef() {
        TypeName = MCP_TypeKind::Completable;
    }
};

template <typename TOutput, typename TInput = TOutput> class ParseResult {
  public:
    bool Success;
    optional<TOutput> Value;
    optional<string> Error;

    ParseResult(bool InSuccess) : Success(InSuccess) {}
    ParseResult(const TOutput& InValue) : Success(true), Value(InValue) {}
    ParseResult(const string& InError) : Success(false), Error(InError) {}
};

template <typename TOutput, typename TDef, typename TInput = TOutput> class MCP_Type {
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

MCP_NAMESPACE_END