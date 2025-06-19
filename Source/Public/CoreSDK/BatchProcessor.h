#pragma once

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

#include "MCPMessages.h"
#include "MCPTask.h"
#include "Macros.h"
#include "json.hpp"


// TODO: @HalcyonOmega - Is this used? Does this facilitate the MCP spec?

MCP_NAMESPACE_BEGIN

// Batch request result
struct BatchRequestResult {
    std::vector<nlohmann::json> Results;
    std::vector<std::string> Errors;
    size_t SuccessCount = 0;
    size_t FailureCount = 0;
    std::chrono::microseconds TotalProcessingTime{0};

    bool HasPartialFailures() const {
        return FailureCount > 0 && SuccessCount > 0;
    }

    bool IsCompleteFailure() const {
        return FailureCount > 0 && SuccessCount == 0;
    }

    bool IsCompleteSuccess() const {
        return FailureCount == 0 && SuccessCount > 0;
    }

    nlohmann::json ToJSON() const;
};

// Individual batch item result
struct BatchItemResult {
    std::optional<nlohmann::json> Result;
    std::optional<std::string> Error;
    std::chrono::microseconds ProcessingTime{0};
    size_t ItemIndex;

    bool IsSuccess() const {
        return Result.has_value();
    }
    bool IsError() const {
        return Error.has_value();
    }
};

// Batch processing configuration
struct BatchProcessingConfig {
    size_t MaxBatchSize = 100;
    size_t MaxParallelItems = 10;
    std::chrono::milliseconds ItemTimeout{30000};   // 30 seconds per item
    std::chrono::milliseconds BatchTimeout{300000}; // 5 minutes total
    bool StopOnFirstError = false;
    bool PreserveOrder = true;

    // Memory optimization
    bool EnableStreaming = false; // Process items as they complete
    size_t MaxMemoryUsageMB = 256;
};

// Batch processor interface
class IBatchProcessor {
  public:
    virtual ~IBatchProcessor() = default;

    virtual MCPTask<BatchRequestResult>
    ProcessBatch(const std::vector<nlohmann::json>& InBatchItems,
                 const BatchProcessingConfig& InConfig = {}) = 0;

    virtual void CancelBatch() = 0;
    virtual bool IsBatchInProgress() const = 0;
};

// JSON-RPC batch processor implementation
class JSONRPCBatchProcessor : public IBatchProcessor {
  public:
    // Callback for processing individual JSON-RPC requests
    using RequestProcessor = std::function<MCPTask<nlohmann::json>(const nlohmann::json&)>;

    explicit JSONRPCBatchProcessor(RequestProcessor InProcessor);
    ~JSONRPCBatchProcessor();

    MCPTask<BatchRequestResult> ProcessBatch(const std::vector<nlohmann::json>& InBatchItems,
                                             const BatchProcessingConfig& InConfig = {}) override;

    void CancelBatch() override;
    bool IsBatchInProgress() const override;

    // Statistics
    struct BatchStatistics {
        std::atomic<uint64_t> TotalBatchesProcessed{0};
        std::atomic<uint64_t> TotalItemsProcessed{0};
        std::atomic<uint64_t> TotalFailures{0};
        std::atomic<uint64_t> TotalProcessingTime{0}; // microseconds

        double GetAverageItemsPerBatch() const {
            auto batches = TotalBatchesProcessed.load();
            return batches > 0 ? static_cast<double>(TotalItemsProcessed.load()) / batches : 0.0;
        }

        double GetAverageProcessingTime() const {
            auto items = TotalItemsProcessed.load();
            return items > 0 ? static_cast<double>(TotalProcessingTime.load()) / items : 0.0;
        }

        double GetFailureRate() const {
            auto items = TotalItemsProcessed.load();
            return items > 0 ? static_cast<double>(TotalFailures.load()) / items : 0.0;
        }
    };

    BatchStatistics GetStatistics() const;
    void ResetStatistics();

  private:
    MCPTask<BatchItemResult> ProcessSingleItem(const nlohmann::json& InItem, size_t InItemIndex,
                                               const BatchProcessingConfig& InConfig);

    MCPTask<std::vector<BatchItemResult>>
    ProcessItemsParallel(const std::vector<nlohmann::json>& InItems,
                         const BatchProcessingConfig& InConfig);

    MCPTask<std::vector<BatchItemResult>>
    ProcessItemsSequential(const std::vector<nlohmann::json>& InItems,
                           const BatchProcessingConfig& InConfig);

    bool ValidateBatchRequest(const std::vector<nlohmann::json>& InBatchItems) const;
    void UpdateStatistics(const BatchRequestResult& InResult);

    RequestProcessor m_RequestProcessor;
    std::atomic<bool> m_BatchInProgress{false};
    std::atomic<bool> m_ShouldCancel{false};
    BatchStatistics m_Statistics;

    mutable std::mutex m_ProcessingMutex;
    std::vector<std::future<void>> m_ActiveTasks;
};

// Specialized MCP batch processor
class MCPBatchProcessor : public JSONRPCBatchProcessor {
  public:
    MCPBatchProcessor(std::shared_ptr<class MCPServer> InServer);

    // MCP-specific batch processing optimizations
    MCPTask<BatchRequestResult> ProcessMCPBatch(const std::vector<nlohmann::json>& InBatchItems,
                                                const std::string& InClientID,
                                                const BatchProcessingConfig& InConfig = {});

    // Handle different MCP request types in batches
    MCPTask<BatchRequestResult>
    ProcessToolCallBatch(const std::vector<CallToolRequest>& InToolCalls,
                         const BatchProcessingConfig& InConfig = {});

    MCPTask<BatchRequestResult>
    ProcessPromptBatch(const std::vector<GetPromptRequest>& InPromptRequests,
                       const BatchProcessingConfig& InConfig = {});

    MCPTask<BatchRequestResult>
    ProcessResourceBatch(const std::vector<ReadResourceRequest>& InResourceRequests,
                         const BatchProcessingConfig& InConfig = {});

  private:
    std::shared_ptr<class MCPServer> m_Server;

    MCPTask<nlohmann::json> ProcessMCPRequest(const nlohmann::json& InRequest);
    bool IsToolCallRequest(const nlohmann::json& InRequest) const;
    bool IsPromptRequest(const nlohmann::json& InRequest) const;
    bool IsResourceRequest(const nlohmann::json& InRequest) const;
};

// Batch processing utilities
class BatchProcessingUtils {
  public:
    // Split large batch into smaller chunks
    static std::vector<std::vector<nlohmann::json>>
    ChunkBatch(const std::vector<nlohmann::json>& InBatch, size_t InChunkSize);

    // Validate JSON-RPC batch format
    static bool ValidateJSONRPCBatch(const std::vector<nlohmann::json>& InBatch);

    // Extract batch statistics
    static nlohmann::json ExtractBatchMetrics(const BatchRequestResult& InResult);

    // Memory usage estimation
    static size_t EstimateMemoryUsage(const std::vector<nlohmann::json>& InBatch);

    // Optimize batch order for better performance
    static std::vector<nlohmann::json>
    OptimizeBatchOrder(const std::vector<nlohmann::json>& InBatch);

    // Create batch response in JSON-RPC format
    static nlohmann::json CreateBatchResponse(const BatchRequestResult& InResult);
};

// RAII batch processing timer
class BatchProcessingTimer {
  public:
    explicit BatchProcessingTimer(const std::string& InBatchID);
    ~BatchProcessingTimer();

    void SetItemCount(size_t InItemCount);
    void RecordItemProcessed(bool InSuccess);
    void AddMetric(const std::string& InKey, const nlohmann::json& InValue);

  private:
    std::string m_BatchID;
    std::chrono::steady_clock::time_point m_StartTime;
    size_t m_ItemCount = 0;
    size_t m_ProcessedItems = 0;
    size_t m_SuccessfulItems = 0;
    nlohmann::json m_AdditionalMetrics;
};

// Streaming batch processor for large batches
class StreamingBatchProcessor {
  public:
    using ItemCallback = std::function<void(const BatchItemResult&)>;
    using CompletionCallback = std::function<void(const BatchRequestResult&)>;

    StreamingBatchProcessor(JSONRPCBatchProcessor::RequestProcessor InProcessor);

    void ProcessStreamingBatch(const std::vector<nlohmann::json>& InBatchItems,
                               ItemCallback InItemCallback, CompletionCallback InCompletionCallback,
                               const BatchProcessingConfig& InConfig = {});

    void Cancel();
    bool IsProcessing() const;

  private:
    JSONRPCBatchProcessor::RequestProcessor m_RequestProcessor;
    std::atomic<bool> m_IsProcessing{false};
    std::atomic<bool> m_ShouldCancel{false};

    MCPTaskVoid ProcessStreamingItem(const nlohmann::json& InItem, size_t InItemIndex,
                                     ItemCallback InItemCallback,
                                     const BatchProcessingConfig& InConfig);
};

// Batch processing factory
class BatchProcessorFactory {
  public:
    static std::unique_ptr<IBatchProcessor>
    CreateJSONRPCProcessor(JSONRPCBatchProcessor::RequestProcessor InProcessor);

    static std::unique_ptr<MCPBatchProcessor>
    CreateMCPProcessor(std::shared_ptr<class MCPServer> InServer);

    static std::unique_ptr<StreamingBatchProcessor>
    CreateStreamingProcessor(JSONRPCBatchProcessor::RequestProcessor InProcessor);
};

MCP_NAMESPACE_END