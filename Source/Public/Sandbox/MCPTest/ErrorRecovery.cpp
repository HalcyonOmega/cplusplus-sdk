#include "ErrorRecovery.h"

#include <thread>

// CircuitBreaker template implementations
template <typename TResult>
MCPTask<TResult> CircuitBreaker<TResult>::Execute(std::function<MCPTask<TResult>()> InOperation) {
    auto state = GetState();

    if (state == CircuitBreakerState::Open) {
        throw std::runtime_error("Circuit breaker is open - operation not allowed");
    }

    try {
        auto result = co_await InOperation();

        // Operation succeeded, reset failure count if in half-open state
        if (state == CircuitBreakerState::HalfOpen) { Reset(); }

        co_return result;

    } catch (const std::exception& e) {
        RecordFailure();
        throw;
    }
}

template <typename TResult> CircuitBreakerState CircuitBreaker<TResult>::GetState() const {
    std::lock_guard<std::mutex> lock(m_StateMutex);

    if (m_State == CircuitBreakerState::Open) {
        auto now = std::chrono::steady_clock::now();
        if (now - m_LastFailureTime >= m_RecoveryTimeout) {
            m_State = CircuitBreakerState::HalfOpen;
        }
    }

    return m_State;
}

template <typename TResult> void CircuitBreaker<TResult>::Reset() {
    std::lock_guard<std::mutex> lock(m_StateMutex);
    m_State = CircuitBreakerState::Closed;
    m_FailureCount = 0;
}

template <typename TResult> void CircuitBreaker<TResult>::RecordFailure() {
    std::lock_guard<std::mutex> lock(m_StateMutex);
    m_FailureCount++;
    m_LastFailureTime = std::chrono::steady_clock::now();

    if (m_FailureCount >= m_FailureThreshold) { m_State = CircuitBreakerState::Open; }
}

// RetryExecutor template implementations
template <typename TResult>
MCPTask<TResult> RetryExecutor::ExecuteWithRetry(std::function<MCPTask<TResult>()> InOperation,
                                                 const RetryPolicy& InPolicy) {
    size_t attempt = 0;
    std::chrono::milliseconds delay = InPolicy.InitialDelay;

    while (attempt <= InPolicy.MaxRetries) {
        try {
            co_return co_await InOperation();
        } catch (const std::exception& e) {
            attempt++;

            if (attempt > InPolicy.MaxRetries || !InPolicy.ShouldRetry(e)) {
                throw; // Give up
            }

            // Wait before retry
            std::this_thread::sleep_for(delay);

            // Exponential backoff
            delay = std::min(static_cast<std::chrono::milliseconds>(static_cast<long long>(
                                 delay.count() * InPolicy.BackoffMultiplier)),
                             InPolicy.MaxDelay);
        }
    }

    throw std::runtime_error("All retry attempts exhausted");
}

// ConnectionRecoveryManager implementation
ConnectionRecoveryManager::ConnectionRecoveryManager() = default;

ConnectionRecoveryManager::~ConnectionRecoveryManager() {
    StopRecovery();
}

void ConnectionRecoveryManager::StartRecovery(std::function<MCPTaskVoid()> InRecoveryFunction) {
    std::lock_guard<std::mutex> lock(m_RecoveryMutex);

    if (m_IsInRecoveryMode.load()) {
        return; // Already in recovery mode
    }

    m_RecoveryFunction = InRecoveryFunction;
    m_IsInRecoveryMode = true;
    m_ShouldStop = false;

    // Start recovery loop in background
    std::thread([this]() { RecoveryLoop().GetResult(); }).detach();
}

void ConnectionRecoveryManager::StopRecovery() {
    m_ShouldStop = true;

    std::lock_guard<std::mutex> lock(m_RecoveryMutex);
    m_IsInRecoveryMode = false;
}

bool ConnectionRecoveryManager::IsInRecoveryMode() const {
    return m_IsInRecoveryMode.load();
}

MCPTaskVoid ConnectionRecoveryManager::RecoveryLoop() {
    while (!m_ShouldStop.load()) {
        try {
            if (m_RecoveryFunction) {
                co_await m_RecoveryFunction();

                // Recovery succeeded, exit recovery mode
                std::lock_guard<std::mutex> lock(m_RecoveryMutex);
                m_IsInRecoveryMode = false;
                co_return;
            }
        } catch (const std::exception&) {
            // Recovery failed, wait and try again
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    co_return;
}

// FailedOperationCleanup implementation
void FailedOperationCleanup::RegisterCleanupTask(const std::string& InOperationID,
                                                 std::function<void()> InCleanupFunction) {
    std::lock_guard<std::mutex> lock(m_CleanupMutex);
    m_CleanupTasks[InOperationID] = InCleanupFunction;
}

void FailedOperationCleanup::ExecuteCleanup(const std::string& InOperationID) {
    std::lock_guard<std::mutex> lock(m_CleanupMutex);

    auto iter = m_CleanupTasks.find(InOperationID);
    if (iter != m_CleanupTasks.end()) {
        try {
            iter->second();
        } catch (const std::exception&) {
            // Ignore cleanup errors
        }
        m_CleanupTasks.erase(iter);
    }
}

void FailedOperationCleanup::ExecuteAllCleanups() {
    std::lock_guard<std::mutex> lock(m_CleanupMutex);

    for (auto& [operationID, cleanupFunc] : m_CleanupTasks) {
        try {
            cleanupFunc();
        } catch (const std::exception&) {
            // Ignore cleanup errors
        }
    }

    m_CleanupTasks.clear();
}

// Explicit template instantiations for common types
template class CircuitBreaker<std::string>;
template class CircuitBreaker<nlohmann::json>;
template class CircuitBreaker<bool>;

template MCPTask<std::string>
RetryExecutor::ExecuteWithRetry<std::string>(std::function<MCPTask<std::string>()> InOperation,
                                             const RetryPolicy& InPolicy);
template MCPTask<nlohmann::json> RetryExecutor::ExecuteWithRetry<nlohmann::json>(
    std::function<MCPTask<nlohmann::json>()> InOperation, const RetryPolicy& InPolicy);
template MCPTask<bool>
RetryExecutor::ExecuteWithRetry<bool>(std::function<MCPTask<bool>()> InOperation,
                                      const RetryPolicy& InPolicy);