#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <queue>
#include <unordered_map>

#include "MCPTask.h"
#include "json.hpp"

// TODO: @HalcyonOmega - Is this used? Does this facilitate the MCP spec?

// Object pool for message reuse
template <typename T> class ObjectPool {
  public:
    explicit ObjectPool(size_t InInitialSize = 100) {
        for (size_t i = 0; i < InInitialSize; ++i) { m_Pool.push(std::make_unique<T>()); }
    }

    std::unique_ptr<T> Acquire() {
        std::lock_guard<std::mutex> lock(m_Mutex);

        if (m_Pool.empty()) { return std::make_unique<T>(); }

        auto obj = std::move(m_Pool.front());
        m_Pool.pop();
        return obj;
    }

    void Release(std::unique_ptr<T> InObject) {
        if (!InObject) return;

        std::lock_guard<std::mutex> lock(m_Mutex);

        if (m_Pool.size() < m_MaxPoolSize) { m_Pool.push(std::move(InObject)); }
    }

    size_t Size() const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Pool.size();
    }

  private:
    static constexpr size_t m_MaxPoolSize = 1000;
    std::queue<std::unique_ptr<T>> m_Pool;
    mutable std::mutex m_Mutex;
};

// Message pool for high-throughput scenarios
class MessagePool {
  public:
    MessagePool();
    ~MessagePool();

    std::unique_ptr<nlohmann::json> AcquireMessage();
    void ReleaseMessage(std::unique_ptr<nlohmann::json> InMessage);

    size_t GetPoolSize() const;
    size_t GetActiveCount() const;

  private:
    ObjectPool<nlohmann::json> m_JsonPool;
    std::atomic<size_t> m_ActiveCount{0};
};

// Connection pool for HTTP clients
class ConnectionPool {
  public:
    ConnectionPool(size_t InMaxConnections = 50);
    ~ConnectionPool();

    // Acquire a connection for the given host
    std::shared_ptr<Poco::Net::HTTPClientSession> AcquireConnection(const std::string& InHost,
                                                                    uint16_t InPort = 80);

    // Release a connection back to the pool
    void ReleaseConnection(const std::string& InHost, uint16_t InPort,
                           std::shared_ptr<Poco::Net::HTTPClientSession> InConnection);

    // Clean up expired connections
    void CleanupExpiredConnections();

    size_t GetActiveConnections() const;
    size_t GetTotalConnections() const;

  private:
    struct ConnectionInfo {
        std::shared_ptr<Poco::Net::HTTPClientSession> Connection;
        std::chrono::steady_clock::time_point LastUsed;
    };

    std::string MakeConnectionKey(const std::string& InHost, uint16_t InPort) const;

    const size_t m_MaxConnections;
    const std::chrono::minutes m_ConnectionTimeout{5};

    std::unordered_map<std::string, std::queue<ConnectionInfo>> m_ConnectionPools;
    std::atomic<size_t> m_ActiveConnections{0};
    std::atomic<size_t> m_TotalConnections{0};
    mutable std::mutex m_PoolMutex;
};

// Resource cache with TTL
template <typename TKey, typename TValue> class ResourceCache {
  public:
    explicit ResourceCache(std::chrono::seconds InTTL = std::chrono::seconds(300)) : m_TTL(InTTL) {}

    void Put(const TKey& InKey, const TValue& InValue) {
        std::lock_guard<std::mutex> lock(m_CacheMutex);

        auto now = std::chrono::steady_clock::now();
        m_Cache[InKey] = {InValue, now};
    }

    std::optional<TValue> Get(const TKey& InKey) {
        std::lock_guard<std::mutex> lock(m_CacheMutex);

        auto iter = m_Cache.find(InKey);
        if (iter == m_Cache.end()) { return std::nullopt; }

        auto now = std::chrono::steady_clock::now();
        if (now - iter->second.Timestamp > m_TTL) {
            m_Cache.erase(iter);
            return std::nullopt;
        }

        // Update timestamp for LRU-like behavior
        iter->second.Timestamp = now;
        return iter->second.Value;
    }

    void Remove(const TKey& InKey) {
        std::lock_guard<std::mutex> lock(m_CacheMutex);
        m_Cache.erase(InKey);
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(m_CacheMutex);
        m_Cache.clear();
    }

    size_t Size() const {
        std::lock_guard<std::mutex> lock(m_CacheMutex);
        return m_Cache.size();
    }

    void CleanupExpired() {
        std::lock_guard<std::mutex> lock(m_CacheMutex);

        auto now = std::chrono::steady_clock::now();
        for (auto iter = m_Cache.begin(); iter != m_Cache.end();) {
            if (now - iter->second.Timestamp > m_TTL) {
                iter = m_Cache.erase(iter);
            } else {
                ++iter;
            }
        }
    }

  private:
    struct CacheEntry {
        TValue Value;
        std::chrono::steady_clock::time_point Timestamp;
    };

    const std::chrono::seconds m_TTL;
    std::unordered_map<TKey, CacheEntry> m_Cache;
    mutable std::mutex m_CacheMutex;
};

// Async I/O optimization utilities
class AsyncIOOptimizer {
  public:
    AsyncIOOptimizer();
    ~AsyncIOOptimizer();

    // Batch multiple operations together
    template <typename TOperation>
    MCPTask<std::vector<typename TOperation::result_type>>
    BatchExecute(std::vector<TOperation> InOperations, size_t InBatchSize = 50);

    // Parallel execution with concurrency control
    template <typename TResult>
    MCPTask<std::vector<TResult>>
    ParallelExecute(std::vector<std::function<MCPTask<TResult>()>> InOperations,
                    size_t InMaxConcurrency = 10);

  private:
    std::atomic<size_t> m_ActiveOperations{0};
};

// Performance metrics collector
class PerformanceMetrics {
  public:
    struct Metrics {
        std::atomic<uint64_t> TotalRequests{0};
        std::atomic<uint64_t> SuccessfulRequests{0};
        std::atomic<uint64_t> FailedRequests{0};
        std::atomic<uint64_t> TotalResponseTime{0}; // in microseconds
        std::atomic<uint64_t> MaxResponseTime{0};
        std::atomic<uint64_t> MinResponseTime{UINT64_MAX};

        double GetAverageResponseTime() const {
            auto total = TotalRequests.load();
            return total > 0 ? static_cast<double>(TotalResponseTime.load()) / total : 0.0;
        }

        double GetSuccessRate() const {
            auto total = TotalRequests.load();
            return total > 0 ? static_cast<double>(SuccessfulRequests.load()) / total : 0.0;
        }
    };

    static PerformanceMetrics& GetInstance();

    void RecordRequest(std::chrono::microseconds InResponseTime, bool InSuccess);
    Metrics GetMetrics() const;
    void ResetMetrics();

  private:
    PerformanceMetrics() = default;
    Metrics m_Metrics;
};

// RAII timer for automatic performance tracking
class PerformanceTimer {
  public:
    PerformanceTimer();
    ~PerformanceTimer();

    void MarkSuccess();
    void MarkFailure();

  private:
    std::chrono::steady_clock::time_point m_StartTime;
    bool m_Success = false;
    bool m_Finished = false;
};