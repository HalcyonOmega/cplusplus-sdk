#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "MCPTask.h"

// TODO: @HalcyonOmega - Is this used? Does this facilitate the MCP spec?

// Circuit breaker states
enum class CircuitBreakerState { Closed, Open, HalfOpen };

// Circuit breaker for external operations
template <typename TResult> class CircuitBreaker {
  public:
    explicit CircuitBreaker(size_t InFailureThreshold = 5,
                            std::chrono::seconds InRecoveryTimeout = std::chrono::seconds(30))
        : m_FailureThreshold(InFailureThreshold), m_RecoveryTimeout(InRecoveryTimeout) {}

    MCPTask<TResult> Execute(std::function<MCPTask<TResult>()> InOperation);
    CircuitBreakerState GetState() const;
    void Reset();

  private:
    void RecordFailure();

    const size_t m_FailureThreshold;
    const std::chrono::seconds m_RecoveryTimeout;

    mutable std::mutex m_StateMutex;
    mutable CircuitBreakerState m_State = CircuitBreakerState::Closed;
    size_t m_FailureCount = 0;
    std::chrono::steady_clock::time_point m_LastFailureTime;
};

// Retry policy configuration
struct RetryPolicy {
    size_t MaxRetries = 3;
    std::chrono::milliseconds InitialDelay{100};
    double BackoffMultiplier = 2.0;
    std::chrono::milliseconds MaxDelay{5000};

    std::function<bool(const std::exception&)> ShouldRetry = [](const std::exception&) {
        return true;
    };
};

// Retry executor with exponential backoff
class RetryExecutor {
  public:
    template <typename TResult>
    static MCPTask<TResult> ExecuteWithRetry(std::function<MCPTask<TResult>()> InOperation,
                                             const RetryPolicy& InPolicy = {});
};

// Connection recovery manager
class ConnectionRecoveryManager {
  public:
    ConnectionRecoveryManager();
    ~ConnectionRecoveryManager();

    void StartRecovery(std::function<MCPTaskVoid()> InRecoveryFunction);
    void StopRecovery();
    bool IsInRecoveryMode() const;

  private:
    MCPTaskVoid RecoveryLoop();

    std::atomic<bool> m_IsInRecoveryMode{false};
    std::atomic<bool> m_ShouldStop{false};
    std::function<MCPTaskVoid()> m_RecoveryFunction;
    mutable std::mutex m_RecoveryMutex;
};

// Failed operation cleanup manager
class FailedOperationCleanup {
  public:
    void RegisterCleanupTask(const std::string& InOperationID,
                             std::function<void()> InCleanupFunction);
    void ExecuteCleanup(const std::string& InOperationID);
    void ExecuteAllCleanups();

  private:
    std::unordered_map<std::string, std::function<void()>> m_CleanupTasks;
    mutable std::mutex m_CleanupMutex;
};
