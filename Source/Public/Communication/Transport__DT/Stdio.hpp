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
class ChildProcess;
class AbortController;

// Type definitions to match Node.js types
using IOType = std::string; // "pipe", "inherit", "overlapped", etc.
using Stream = std::iostream;
using PassThrough = std::stringstream; // TODO: From "node:stream"

MCP_NAMESPACE_BEGIN

// Platform-specific environment variables to inherit by default
#ifdef _WIN32
const vector<string> DEFAULT_INHERITED_ENV_VARS = {
    "APPDATA",     "HOMEDRIVE",  "HOMEPATH", "LOCALAPPDATA", "PATH",       "PROCESSOR_ARCHITECTURE",
    "SYSTEMDRIVE", "SYSTEMROOT", "TEMP",     "USERNAME",     "USERPROFILE"};
#else
const vector<string> DEFAULT_INHERITED_ENV_VARS = {"HOME",  "LOGNAME", "PATH",
                                                   "SHELL", "TERM",    "USER"};
#endif

// Parameters for configuring a stdio server process
struct StdioServerParameters {
    /**
     * The executable to run to start the server.
     */
    string Command;

    /**
     * Command line arguments to pass to the executable.
     */
    optional<vector<string>> Args;

    /**
     * The environment to use when spawning the process.
     * If not specified, the result of GetDefaultEnvironment() will be used.
     */
    optional<unordered_map<string, string>> Env;

    /**
     * How to handle stderr of the child process.
     * The default is "inherit", meaning messages to stderr will be printed to the parent process's
     * stderr.
     */
    optional<variant<IOType, Stream*, int>> Stderr;

    /**
     * The working directory to use when spawning the process.
     * If not specified, the current working directory will be inherited.
     */
    optional<string> CWD;
};

/**
 * Returns a default environment object including only environment variables deemed safe to inherit.
 */
inline unordered_map<string, string> GetDefaultEnvironment() {
    unordered_map<string, string> Env;

    for (const auto& key : DEFAULT_INHERITED_ENV_VARS) {
        const char* value = getenv(key.c_str());
        if (value == nullptr) { continue; }

        string valueStr(value);
        if (valueStr.starts_with("()")) {
            // Skip functions, which are a security risk.
            continue;
        }

        Env[key] = valueStr;
    }

    return Env;
}

/**
 * Server transport for stdio: this communicates with a MCP client by reading from the current
 * process' stdin and writing to stdout.
 *
 * This transport provides cross-platform stdio communication capabilities.
 */
class StdioServerTransport : public Transport {
  private:
    unique_ptr<ReadBuffer> m_ReadBuffer;
    atomic<bool> m_Started{false};
    istream& m_Stdin;
    ostream& m_Stdout;
    thread m_ReadThread;
    atomic<bool> m_ShouldStop{false};

  public:
    explicit StdioServerTransport(istream& stdin_stream = cin, ostream& stdout_stream = cout)
        : m_Stdin(stdin_stream), m_Stdout(stdout_stream) {
        m_ReadBuffer = make_unique<ReadBuffer>();
    }

    ~StdioServerTransport() override {
        Close(); // Don't wait in destructor
    }

    // Transport interface implementation
    future<void> Start() override {
        return async(launch::async, [this]() {
            if (m_Started.exchange(true)) {
                throw runtime_error(
                    "StdioServerTransport already started! If using Server class, note "
                    "that connect() calls start() automatically.");
            }

            m_ReadThread = thread(&StdioServerTransport::ReadLoop, this);

            if (OnStart) { OnStart.value(); }
        });
    }

    future<void> Close() override {
        return async(launch::async, [this]() {
            if (!m_Started.exchange(false)) return;

            m_ShouldStop = true;

            if (m_ReadThread.joinable()) { m_ReadThread.join(); }

            if (m_ReadBuffer) { m_ReadBuffer->Clear(); }

            if (OnClose) { OnClose.value(); }
        });
    }

    future<void> Send(const MessageBase& /* InMessage */,
                      const TransportSendOptions& /* InOptions */ = {}) override {
        return async(launch::async, [this]() {
            // TODO: Serialize MessageBase to JSON string
            // For now, this is a placeholder - would need proper serialization
            string serialized = "{}"; // Placeholder
            string message = serialized + "\n";
            m_Stdout << message;
            m_Stdout.flush();
        });
    }

    void WriteSSEEvent(const string& InEvent, const string& InData) override {
        // SSE format: event: {event}\ndata: {data}\n\n
        m_Stdout << "event: " << InEvent << "\n";
        m_Stdout << "data: " << InData << "\n\n";
        m_Stdout.flush();
    }

    bool Resume(const string& /* InResumptionToken */) override {
        // Not implemented yet
        return false;
    }

  private:
    /**
     * Event handler for incoming data
     */
    void OnData(const vector<uint8_t>& chunk) {
        if (m_ReadBuffer) {
            m_ReadBuffer->Append(chunk);
            ProcessReadBuffer();
        }
    }

    /**
     * Event handler for errors
     */
    void OnErrorInternal(const string& error) {
        lock_guard<mutex> lock(m_CallbackMutex);
        if (m_OnError) {
            // Create ErrorBase from string message
            ErrorBase ErrorMessage = ErrorBase(Errors::InternalError, error);
            m_OnError(ErrorMessage);
        }
    }

    /**
     * Processes the read buffer and extracts complete messages
     */
    void ProcessReadBuffer() {
        while (true) {
            try {
                // TODO: ReadMessage should return optional<unique_ptr<MessageBase>>
                // For now, commented out until ReadBuffer interface is updated
                // auto message = m_ReadBuffer->ReadMessage();
                // if (!message) { break; }

                // lock_guard<mutex> lock(m_CallbackMutex);
                // if (m_OnMessage) {
                //     m_OnMessage(*message.value(), std::nullopt); // No auth info for stdio
                // }
                break; // Placeholder
            } catch (const exception& e) {
                OnErrorInternal(e.what());
                break;
            }
        }
    }

    /**
     * Main reading loop that runs in a separate thread
     */
    void ReadLoop() {
        vector<uint8_t> buffer(4096);

        while (!m_ShouldStop && m_Started) {
            try {
                m_Stdin.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
                streamsize bytesRead = m_Stdin.gcount();

                if (bytesRead > 0) {
                    buffer.resize(bytesRead);
                    OnData(buffer);
                    buffer.resize(4096);
                }

                if (m_Stdin.eof() || m_Stdin.fail()) { break; }
            } catch (const exception& e) {
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
    // Default Variables
    unique_ptr<ChildProcess> m_Process;
    unique_ptr<AbortController> m_AbortController; // TODO: @HalcyonOmega = new AbortController
    unique_ptr<ReadBuffer> m_ReadBuffer;
    StdioServerParameters m_ServerParams;
    unique_ptr<PassThrough> m_StderrStream;

    // Additional Variables
    atomic<bool> m_AbortRequested{false};
    atomic<bool> m_Started{false};

  public:
    explicit StdioClientTransport(const StdioServerParameters& ServerParams)
        : m_ServerParams(ServerParams) {
        m_ReadBuffer = make_unique<ReadBuffer>();

        // Check if stderr should be piped
        if (ServerParams.Stderr.has_value()) {
            const auto& stderr_val = ServerParams.Stderr.value();
            if (std::holds_alternative<IOType>(stderr_val)) {
                const auto& stderr_str = std::get<IOType>(stderr_val);
                if (stderr_str == "pipe" || stderr_str == "overlapped") {
                    m_StderrStream = make_unique<PassThrough>();
                }
            }
        }
    }

    ~StdioClientTransport() override {
        // Ensure proper cleanup by calling Close if not already closed
        if (m_Started.load()) {
            try {
                auto future = Close();
                // Don't wait in destructor to avoid blocking
            } catch (...) {
                // Ignore exceptions in destructor
            }
        }
    }

    // Transport interface implementation
    std::future<void> Start() override {
        return std::async(std::launch::async, [this]() {
            if (m_Started.exchange(true)) {
                throw std::runtime_error(
                    "StdioClientTransport already started! If using Client class, note "
                    "that connect() calls start() automatically.");
            }

            try {
                SpawnProcess();
                if (OnStart) { OnStart.value(); }
            } catch (const std::exception& e) {
                m_Started = false;
                if (OnError) {
                    // Create ErrorBase from string - this would need proper ErrorBase
                    // implementation For now, this is a placeholder
                    throw; // Re-throw for now
                }
                throw;
            }
        });
    }

    std::future<void> Close() override {
        return std::async(std::launch::async, [this]() {
            if (!m_Started.exchange(false)) { return; }

            m_AbortRequested = true;
            m_Process.reset();

            if (m_ReadBuffer) { m_ReadBuffer->Clear(); }

            if (OnClose) { OnClose.value(); }
        });
    }

    future<void> Send(const MessageBase& /* InMessage */,
                      const TransportSendOptions& /* InOptions */ = {}) override {
        return std::async(std::launch::async, [this]() {
            if (!m_Process || !m_Started) { throw runtime_error("Not connected"); }

            // TODO: Serialize MessageBase to JSON string
            // For now, this is a placeholder - would need proper serialization
            std::string serialized = "{}"; // Placeholder
            std::string message = serialized + "\n";
            WriteToProcess(message);
        });
    }

    void WriteSSEEvent(const string& InEvent, const string& InData) override {
        // SSE format: event: {event}\ndata: {data}\n\n
        string sseMessage = "event: " + InEvent + "\ndata: " + InData + "\n\n";
        WriteToProcess(sseMessage);
    }

    bool Resume(const string& /* InResumptionToken */) override {
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

        throw runtime_error(
            "Process spawning not yet implemented - requires platform-specific code");
    }

    /**
     * Processes the read buffer and extracts complete messages
     */
    void ProcessReadBuffer() {
        while (true) {
            try {
                // TODO: ReadMessage() returns optional<MessageBase> but MessageBase is abstract
                // This would need to be changed to return a concrete message type
                // For now, skip the actual message processing

                // auto message = m_ReadBuffer->ReadMessage();
                // if (!message) { break; }

                // if (OnMessage) {
                //     OnMessage.value()(message.value(), std::nullopt); // No auth info for stdio
                // }
                break; // Placeholder - would need proper message processing
            } catch (const std::exception& /* e */) {
                if (OnError) {
                    // TODO: Create proper ErrorBase from exception
                    // For now, this is a placeholder
                }
                break;
            }
        }
    }

    /**
     * Writes data to the child process stdin
     */
    void WriteToProcess(const string& /* data */) {
        // This would write to the child process stdin
        // TODO: Placeholder implementation - would need actual process communication
        throw runtime_error("Process communication not yet implemented");
    }
};

MCP_NAMESPACE_END