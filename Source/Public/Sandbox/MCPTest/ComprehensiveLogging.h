#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "json.hpp"

// TODO: @HalcyonOmega - Is this used? Does this facilitate the MCP spec?

// Log levels
enum class LogLevel { TRACE = 0, DEBUG = 1, INFO = 2, WARN = 3, ERROR = 4, FATAL = 5 };

// Log entry structure
struct LogEntry {
    std::chrono::system_clock::time_point Timestamp;
    LogLevel Level;
    std::string Component;
    std::string Message;
    std::string CorrelationID;
    nlohmann::json Context;
    std::string ThreadID;
    std::string Function;
    std::string File;
    int Line;

    nlohmann::json ToJSON() const;
    std::string ToString() const;
};

// Log sink interface
class ILogSink {
  public:
    virtual ~ILogSink() = default;
    virtual void WriteLog(const LogEntry& InEntry) = 0;
    virtual void Flush() = 0;
};

// Console log sink
class ConsoleLogSink : public ILogSink {
  public:
    ConsoleLogSink(bool InUseColors = true);
    void WriteLog(const LogEntry& InEntry) override;
    void Flush() override;

  private:
    bool m_UseColors;
    std::mutex m_OutputMutex;
    std::string GetColorCode(LogLevel InLevel) const;
    std::string GetResetCode() const;
};

// File log sink with rotation
class FileLogSink : public ILogSink {
  public:
    FileLogSink(const std::string& InFilePath, size_t InMaxFileSize = 10 * 1024 * 1024,
                size_t InMaxFiles = 5);
    ~FileLogSink();

    void WriteLog(const LogEntry& InEntry) override;
    void Flush() override;

  private:
    void RotateIfNeeded();
    void RotateFiles();

    std::string m_FilePath;
    size_t m_MaxFileSize;
    size_t m_MaxFiles;
    std::atomic<size_t> m_CurrentFileSize{0};
    std::unique_ptr<std::ofstream> m_FileStream;
    std::mutex m_FileMutex;
};

// JSON structured log sink
class JSONLogSink : public ILogSink {
  public:
    JSONLogSink(const std::string& InFilePath);
    void WriteLog(const LogEntry& InEntry) override;
    void Flush() override;

  private:
    std::string m_FilePath;
    std::unique_ptr<std::ofstream> m_FileStream;
    std::mutex m_FileMutex;
};

// Performance metrics log sink
class MetricsLogSink : public ILogSink {
  public:
    struct PerformanceMetrics {
        std::atomic<uint64_t> TotalLogs{0};
        std::atomic<uint64_t> ErrorCount{0};
        std::atomic<uint64_t> WarnCount{0};
        std::atomic<uint64_t> InfoCount{0};
        std::atomic<uint64_t> DebugCount{0};
        std::atomic<uint64_t> TraceCount{0};

        std::chrono::system_clock::time_point StartTime;

        nlohmann::json ToJSON() const;
    };

    MetricsLogSink();
    void WriteLog(const LogEntry& InEntry) override;
    void Flush() override;

    PerformanceMetrics GetMetrics() const;
    void ResetMetrics();

  private:
    PerformanceMetrics m_Metrics;
};

// Main logger class
class MCPLogger {
  public:
    static MCPLogger& GetInstance();

    // Configuration
    void SetLogLevel(LogLevel InLevel);
    void SetComponent(const std::string& InComponent);
    void AddSink(std::shared_ptr<ILogSink> InSink);
    void RemoveSink(std::shared_ptr<ILogSink> InSink);
    void ClearSinks();

    // Correlation ID management
    void SetCorrelationID(const std::string& InCorrelationID);
    std::string GetCorrelationID() const;
    std::string GenerateCorrelationID() const;

    // Logging methods
    void Log(LogLevel InLevel, const std::string& InMessage, const nlohmann::json& InContext = {},
             const std::string& InFunction = "", const std::string& InFile = "", int InLine = 0);

    void Trace(const std::string& InMessage, const nlohmann::json& InContext = {});
    void Debug(const std::string& InMessage, const nlohmann::json& InContext = {});
    void Info(const std::string& InMessage, const nlohmann::json& InContext = {});
    void Warn(const std::string& InMessage, const nlohmann::json& InContext = {});
    void Error(const std::string& InMessage, const nlohmann::json& InContext = {});
    void Fatal(const std::string& InMessage, const nlohmann::json& InContext = {});

    // Structured logging helpers
    void LogRequest(const std::string& InMethod, const nlohmann::json& InParams,
                    const std::string& InRequestID);
    void LogResponse(const std::string& InMethod, const nlohmann::json& InResult,
                     const std::string& InRequestID, std::chrono::microseconds InDuration);
    void LogError(const std::string& InMethod, const std::string& InError,
                  const std::string& InRequestID, std::chrono::microseconds InDuration);

    // Performance logging
    void LogPerformance(const std::string& InOperation, std::chrono::microseconds InDuration,
                        const nlohmann::json& InMetrics = {});

    // Batch logging for high-throughput scenarios
    void StartBatch();
    void EndBatch();
    void FlushBatch();

    // Flush all sinks
    void Flush();

  private:
    MCPLogger();
    ~MCPLogger();

    LogLevel m_LogLevel = LogLevel::INFO;
    std::string m_Component = "MCP";
    std::vector<std::shared_ptr<ILogSink>> m_Sinks;
    mutable std::mutex m_SinksMutex;

    // Correlation ID storage (thread-local)
    static thread_local std::string m_CorrelationID;

    // Batch logging
    std::atomic<bool> m_BatchMode{false};
    std::vector<LogEntry> m_BatchedEntries;
    std::mutex m_BatchMutex;

    // Async logging support
    std::atomic<bool> m_AsyncMode{false};
    std::unique_ptr<std::thread> m_LoggingThread;
    std::queue<LogEntry> m_LogQueue;
    std::mutex m_QueueMutex;
    std::condition_variable m_QueueCondition;
    std::atomic<bool> m_ShouldStop{false};

    void ProcessLogQueue();
    void WriteToSinks(const LogEntry& InEntry);
    std::string GetCurrentThreadID() const;
};

// RAII correlation ID setter
class CorrelationIDScope {
  public:
    explicit CorrelationIDScope(const std::string& InCorrelationID);
    ~CorrelationIDScope();

  private:
    std::string m_PreviousCorrelationID;
};

// Logging macros for convenience
#define LOG_TRACE(message, ...) MCPLogger::GetInstance().Trace(message, ##__VA_ARGS__)
#define LOG_DEBUG(message, ...) MCPLogger::GetInstance().Debug(message, ##__VA_ARGS__)
#define LOG_INFO(message, ...) MCPLogger::GetInstance().Info(message, ##__VA_ARGS__)
#define LOG_WARN(message, ...) MCPLogger::GetInstance().Warn(message, ##__VA_ARGS__)
#define LOG_ERROR(message, ...) MCPLogger::GetInstance().Error(message, ##__VA_ARGS__)
#define LOG_FATAL(message, ...) MCPLogger::GetInstance().Fatal(message, ##__VA_ARGS__)

// Structured logging macros
#define LOG_REQUEST(method, params, requestId)                                                     \
    MCPLogger::GetInstance().LogRequest(method, params, requestId)
#define LOG_RESPONSE(method, result, requestId, duration)                                          \
    MCPLogger::GetInstance().LogResponse(method, result, requestId, duration)
#define LOG_ERROR_RESPONSE(method, error, requestId, duration)                                     \
    MCPLogger::GetInstance().LogError(method, error, requestId, duration)

// Performance logging macro
#define LOG_PERFORMANCE(operation, duration, ...)                                                  \
    MCPLogger::GetInstance().LogPerformance(operation, duration, ##__VA_ARGS__)

// RAII performance timer
class PerformanceLogTimer {
  public:
    explicit PerformanceLogTimer(const std::string& InOperation,
                                 const nlohmann::json& InContext = {});
    ~PerformanceLogTimer();

    void AddContext(const std::string& InKey, const nlohmann::json& InValue);
    void Cancel(); // Don't log on destruction

  private:
    std::string m_Operation;
    nlohmann::json m_Context;
    std::chrono::steady_clock::time_point m_StartTime;
    bool m_Cancelled = false;
};

#define PERF_TIMER(operation) PerformanceLogTimer _perfTimer(operation)
#define PERF_TIMER_WITH_CONTEXT(operation, context)                                                \
    PerformanceLogTimer _perfTimer(operation, context)