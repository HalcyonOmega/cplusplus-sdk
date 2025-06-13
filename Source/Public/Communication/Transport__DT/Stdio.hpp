#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "Communication/Transport/Transport.h"
#include "Communication/Utilities/ReadBuffer.h"
#include "Core.h"
#include "ErrorBase.h"
#include "MessageBase.h"

// Forward declarations
class Stream;
class ChildProcess;
class PassThrough;

MCP_NAMESPACE_BEGIN

// Platform-specific environment variables to inherit by default
#ifdef _WIN32
const std::vector<std::string> DEFAULT_INHERITED_ENV_VARS = {
    "APPDATA",     "HOMEDRIVE",  "HOMEPATH", "LOCALAPPDATA", "PATH",       "PROCESSOR_ARCHITECTURE",
    "SYSTEMDRIVE", "SYSTEMROOT", "TEMP",     "USERNAME",     "USERPROFILE"};
#else
const std::vector<std::string> DEFAULT_INHERITED_ENV_VARS = {"HOME",  "LOGNAME", "PATH",
                                                             "SHELL", "TERM",    "USER"};
#endif

// Parameters for configuring a stdio server process
struct StdioServerParameters {
    /**
     * The executable to run to start the server.
     */
    std::string Command;

    /**
     * Command line arguments to pass to the executable.
     */
    std::optional<std::vector<std::string>> Args;

    /**
     * The environment to use when spawning the process.
     * If not specified, the result of GetDefaultEnvironment() will be used.
     */
    std::optional<std::unordered_map<std::string, std::string>> Env;

    /**
     * How to handle stderr of the child process.
     * The default is "inherit", meaning messages to stderr will be printed to the parent process's
     * stderr.
     */
    std::string Stderr = "inherit";

    /**
     * The working directory to use when spawning the process.
     * If not specified, the current working directory will be inherited.
     */
    std::optional<std::string> CWD;
};

/**
 * Returns a default environment object including only environment variables deemed safe to inherit.
 */
inline std::unordered_map<std::string, std::string> GetDefaultEnvironment() {
    std::unordered_map<std::string, std::string> env;

    for (const auto& key : DEFAULT_INHERITED_ENV_VARS) {
        const char* value = std::getenv(key.c_str());
        if (value == nullptr) { continue; }

        std::string valueStr(value);
        if (valueStr.starts_with("()")) {
            // Skip functions, which are a security risk.
            continue;
        }

        env[key] = valueStr;
    }

    return env;
}

/**
 * Server transport for stdio: this communicates with a MCP client by reading from the current
 * process' stdin and writing to stdout.
 *
 * This transport provides cross-platform stdio communication capabilities.
 */
class StdioServerTransport : public Transport {
  private:
    std::unique_ptr<ReadBuffer> m_ReadBuffer;
    std::atomic<bool> m_Started{false};
    std::istream& m_Stdin;
    std::ostream& m_Stdout;
    std::thread m_ReadThread;
    std::atomic<bool> m_ShouldStop{false};
    std::mutex m_CallbackMutex;

    // Transport callbacks as per interface
    MessageCallback m_OnMessage;
    ErrorCallback m_OnError;
    CloseCallback m_OnClose;
    StartCallback m_OnStart;
    StopCallback m_OnStop;

  public:
    explicit StdioServerTransport(std::istream& stdin_stream = std::cin,
                                  std::ostream& stdout_stream = std::cout)
        : m_Stdin(stdin_stream), m_Stdout(stdout_stream) {
        m_ReadBuffer = std::make_unique<ReadBuffer>();
    }

    ~StdioServerTransport() override {
        Stop();
    }

    // Transport interface implementation
    void Start() override {
        if (m_Started.exchange(true)) {
            throw std::runtime_error(
                "StdioServerTransport already started! If using Server class, note "
                "that connect() calls start() automatically.");
        }

        m_ReadThread = std::thread(&StdioServerTransport::ReadLoop, this);

        std::lock_guard<std::mutex> lock(m_CallbackMutex);
        if (m_OnStart) { m_OnStart(); }
    }

    void Stop() override {
        if (!m_Started.exchange(false)) return;

        m_ShouldStop = true;

        if (m_ReadThread.joinable()) { m_ReadThread.join(); }

        if (m_ReadBuffer) { m_ReadBuffer->Clear(); }

        std::lock_guard<std::mutex> lock(m_CallbackMutex);
        if (m_OnStop) { m_OnStop(); }
        if (m_OnClose) { m_OnClose(); }
    }

    void Send(const std::string& InMessage, const TransportSendOptions& InOptions = {}) override {
        std::string message = InMessage + "\n"; // Add newline for message boundary
        m_Stdout << message;
        m_Stdout.flush();
    }

    void SetOnMessage(MessageCallback InCallback) override {
        std::lock_guard<std::mutex> lock(m_CallbackMutex);
        m_OnMessage = std::move(InCallback);
    }

    void SetOnError(ErrorCallback InCallback) override {
        std::lock_guard<std::mutex> lock(m_CallbackMutex);
        m_OnError = std::move(InCallback);
    }

    void SetOnClose(CloseCallback InCallback) override {
        std::lock_guard<std::mutex> lock(m_CallbackMutex);
        m_OnClose = std::move(InCallback);
    }

    void SetOnStart(StartCallback InCallback) override {
        std::lock_guard<std::mutex> lock(m_CallbackMutex);
        m_OnStart = std::move(InCallback);
    }

    void SetOnStop(StopCallback InCallback) override {
        std::lock_guard<std::mutex> lock(m_CallbackMutex);
        m_OnStop = std::move(InCallback);
    }

    void WriteSSEEvent(const std::string& InEvent, const std::string& InData) override {
        // SSE format: event: {event}\ndata: {data}\n\n
        m_Stdout << "event: " << InEvent << "\n";
        m_Stdout << "data: " << InData << "\n\n";
        m_Stdout.flush();
    }

    bool Resume(const std::string& InResumptionToken) override {
        // Not implemented yet
        return false;
    }

  private:
    /**
     * Event handler for incoming data
     */
    void OnData(const std::vector<uint8_t>& chunk) {
        if (m_ReadBuffer) {
            m_ReadBuffer->Append(chunk);
            ProcessReadBuffer();
        }
    }

    /**
     * Event handler for errors
     */
    void OnErrorInternal(const std::string& error) {
        std::lock_guard<std::mutex> lock(m_CallbackMutex);
        if (m_OnError) { m_OnError(error); }
    }

    /**
     * Processes the read buffer and extracts complete messages
     */
    void ProcessReadBuffer() {
        while (true) {
            try {
                auto message = m_ReadBuffer->ReadMessage();
                if (!message) { break; }

                std::lock_guard<std::mutex> lock(m_CallbackMutex);
                if (m_OnMessage) {
                    m_OnMessage(message.value(), nullptr); // No auth info for stdio
                }
            } catch (const std::exception& e) { OnErrorInternal(e.what()); }
        }
    }

    /**
     * Main reading loop that runs in a separate thread
     */
    void ReadLoop() {
        std::vector<uint8_t> buffer(4096);

        while (!m_ShouldStop && m_Started) {
            try {
                m_Stdin.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
                std::streamsize bytesRead = m_Stdin.gcount();

                if (bytesRead > 0) {
                    buffer.resize(bytesRead);
                    OnData(buffer);
                    buffer.resize(4096);
                }

                if (m_Stdin.eof() || m_Stdin.fail()) { break; }
            } catch (const std::exception& e) {
                OnErrorInternal(e.what());
                break;
            }
        }
    }
};

/**
 * Client transport for stdio: this will connect to a server by spawning a process and communicating
 * with it over stdin/stdout. This transport is only available in environments that support process
 * spawning.
 */
class StdioClientTransport : public Transport {
  private:
    std::unique_ptr<ChildProcess> m_Process;
    std::atomic<bool> m_AbortRequested{false};
    std::unique_ptr<ReadBuffer> m_ReadBuffer;
    StdioServerParameters m_ServerParams;
    std::unique_ptr<PassThrough> m_StderrStream;
    std::mutex m_CallbackMutex;
    std::atomic<bool> m_Started{false};

    // Transport callbacks as per interface
    MessageCallback m_OnMessage;
    ErrorCallback m_OnError;
    CloseCallback m_OnClose;
    StartCallback m_OnStart;
    StopCallback m_OnStop;

  public:
    explicit StdioClientTransport(const StdioServerParameters& serverParams)
        : m_ServerParams(serverParams) {
        m_ReadBuffer = std::make_unique<ReadBuffer>();

        if (serverParams.Stderr == "pipe" || serverParams.Stderr == "overlapped") {
            m_StderrStream = std::make_unique<PassThrough>();
        }
    }

    ~StdioClientTransport() override {
        Stop();
    }

    // Transport interface implementation
    void Start() override {
        if (m_Started.exchange(true)) {
            throw std::runtime_error(
                "StdioClientTransport already started! If using Client class, note "
                "that connect() calls start() automatically.");
        }

        try {
            SpawnProcess();

            std::lock_guard<std::mutex> lock(m_CallbackMutex);
            if (m_OnStart) { m_OnStart(); }
        } catch (const std::exception& e) {
            m_Started = false;
            std::lock_guard<std::mutex> lock(m_CallbackMutex);
            if (m_OnError) { m_OnError(e.what()); }
            throw;
        }
    }

    void Stop() override {
        if (!m_Started.exchange(false)) return;

        m_AbortRequested = true;
        m_Process.reset();

        if (m_ReadBuffer) { m_ReadBuffer->Clear(); }

        std::lock_guard<std::mutex> lock(m_CallbackMutex);
        if (m_OnStop) { m_OnStop(); }
        if (m_OnClose) { m_OnClose(); }
    }

    void Send(const std::string& InMessage, const TransportSendOptions& InOptions = {}) override {
        if (!m_Process || !m_Started) { throw std::runtime_error("Not connected"); }

        std::string message = InMessage + "\n"; // Add newline for message boundary
        WriteToProcess(message);
    }

    void SetOnMessage(MessageCallback InCallback) override {
        std::lock_guard<std::mutex> lock(m_CallbackMutex);
        m_OnMessage = std::move(InCallback);
    }

    void SetOnError(ErrorCallback InCallback) override {
        std::lock_guard<std::mutex> lock(m_CallbackMutex);
        m_OnError = std::move(InCallback);
    }

    void SetOnClose(CloseCallback InCallback) override {
        std::lock_guard<std::mutex> lock(m_CallbackMutex);
        m_OnClose = std::move(InCallback);
    }

    void SetOnStart(StartCallback InCallback) override {
        std::lock_guard<std::mutex> lock(m_CallbackMutex);
        m_OnStart = std::move(InCallback);
    }

    void SetOnStop(StopCallback InCallback) override {
        std::lock_guard<std::mutex> lock(m_CallbackMutex);
        m_OnStop = std::move(InCallback);
    }

    void WriteSSEEvent(const std::string& InEvent, const std::string& InData) override {
        // SSE format: event: {event}\ndata: {data}\n\n
        std::string sseMessage = "event: " + InEvent + "\ndata: " + InData + "\n\n";
        WriteToProcess(sseMessage);
    }

    bool Resume(const std::string& InResumptionToken) override {
        // Not implemented yet
        return false;
    }

    /**
     * The stderr stream of the child process, if StdioServerParameters.Stderr was set to "pipe" or
     * "overlapped".
     */
    PassThrough* GetStderr() const {
        return m_StderrStream.get();
    }

  private:
    /**
     * Spawns the child process with the configured parameters
     */
    void SpawnProcess() {
        // This would typically use platform-specific process creation APIs
        // For now, this is a placeholder that would need to be implemented
        // with proper process spawning logic for the target platform

        // The actual implementation would:
        // 1. Create the child process with the specified command and arguments
        // 2. Set up stdin/stdout/stderr redirection
        // 3. Configure the environment variables
        // 4. Set the working directory
        // 5. Start monitoring the process for data and errors

        throw std::runtime_error(
            "Process spawning not yet implemented - requires platform-specific code");
    }

    /**
     * Processes the read buffer and extracts complete messages
     */
    void ProcessReadBuffer() {
        while (true) {
            try {
                auto message = m_ReadBuffer->ReadMessage();
                if (!message) { break; }

                std::lock_guard<std::mutex> lock(m_CallbackMutex);
                if (m_OnMessage) {
                    m_OnMessage(message.value(), nullptr); // No auth info for stdio
                }
            } catch (const std::exception& e) {
                std::lock_guard<std::mutex> lock(m_CallbackMutex);
                if (m_OnError) { m_OnError(e.what()); }
            }
        }
    }

    /**
     * Writes data to the child process stdin
     */
    void WriteToProcess(const std::string& data) {
        // This would write to the child process stdin
        // Placeholder implementation - would need actual process communication
        throw std::runtime_error("Process communication not yet implemented");
    }
};

/**
 * Utility function to check if running in Electron environment
 */
inline bool IsElectron() {
    // This would check for Electron-specific environment variables or properties
    // For now, returning false as a placeholder
    return false;
}

MCP_NAMESPACE_END