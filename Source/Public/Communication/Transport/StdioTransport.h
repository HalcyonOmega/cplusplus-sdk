#pragma once

#include "Core.h"
#include "Transport.h"

MCP_NAMESPACE_BEGIN

// Type definitions to match Node.js types
using IOType = std::string; // "pipe", "inherit", "overlapped", etc.
using Stream = std::iostream;
using PassThrough = std::stringstream; // TODO: From "node:stream"

// Transport for stdio: this communicates by reading from stdin and writing to stdout.
class StdioTransport : public Transport {
  public:
    StdioTransport();
    ~StdioTransport() override;

    // Transport interface implementation
    future<void> Start() override;
    future<void> Close() override;
    future<void> Send(const MessageBase& InMessage,
                      const TransportSendOptions& InOptions = {}) override;
    void WriteSSEEvent(const string& InEvent, const string& InData) override;

    bool Resume(const string& InResumptionToken) override;

  private:
    void ReadLoop();

    atomic<bool> m_IsRunning;
    thread m_ReadThread;
};

// Server transport for stdio: this communicates with a MCP client by reading from the current
// process' stdin and writing to stdout.
// This transport provides cross-platform stdio communication capabilities.
class StdioServerTransport : public StdioTransport {
  private:
    unique_ptr<ReadBuffer> m_ReadBuffer;
    atomic<bool> m_Started{false};
    istream& m_Stdin;
    ostream& m_Stdout;
    thread m_ReadThread;
    atomic<bool> m_ShouldStop{false};

  public:
    // Parameters for configuring a stdio server process
    struct StdioServerParameters {
        // The executable to run to start the server.
        string Command;

        // Command line arguments to pass to the executable.
        optional<vector<string>> Args;

        // The environment to use when spawning the process.
        // If not specified, the result of GetDefaultEnvironment() will be used.
        optional<unordered_map<string, string>> Env;

        // How to handle stderr of the child process.
        // The default is "inherit", meaning messages to stderr will be printed to the parent
        // process's stderr.
        optional<variant<IOType, Stream*, int>> Stderr;

        // The working directory to use when spawning the process.
        // If not specified, the current working directory will be inherited.
        optional<string> CWD;
    };

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
            if (!m_Started.exchange(false)) { return; }

            m_ShouldStop = true;

            if (m_ReadThread.joinable()) { m_ReadThread.join(); }

            if (m_ReadBuffer) { m_ReadBuffer->Clear(); }

            if (OnClose) { OnClose.value(); }
        });
    }

    future<void> Send(const MessageBase& InMessage,
                      const TransportSendOptions& /* InOptions */ = {}) override {
        return async(launch::async, [this, &InMessage = InMessage]() {
            string serialized = SerializeMessage(InMessage);
            m_Stdout << serialized << "\n";
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
    // Event handler for incoming data
    void OnData(const vector<uint8_t>& chunk) {
        if (m_ReadBuffer) {
            m_ReadBuffer->Append(chunk);
            ProcessReadBuffer();
        }
    }

    // Processes the read buffer and extracts complete messages
    void ProcessReadBuffer() {
        while (true) {
            try {
                auto messageOpt = m_ReadBuffer->ReadMessage();
                if (!messageOpt) { break; }

                lock_guard<mutex> lock(m_CallbackMutex);
                if (OnMessage && *messageOpt) { OnMessage.value()(*messageOpt->get(), nullopt); }
                // Continue loop to check for more complete messages
            } catch (const exception& e) {
                CallOnError(e.what());
                break;
            }
        }
    }

    // Main reading loop that runs in a separate thread
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
                CallOnError(e.what());
                break;
            }
        }
    }
};

// Client transport for stdio: this will connect to a server by spawning a process and communicating
// with it over stdin/stdout. This transport is only available in environments that support process
// spawning.
class StdioClientTransport : public StdioTransport {
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

    thread m_MonitorThread;

  public:
    explicit StdioClientTransport(const StdioServerParameters& ServerParams)
        : m_ServerParams(ServerParams) {
        m_ReadBuffer = make_unique<ReadBuffer>();

        // Check if stderr should be piped
        if (ServerParams.Stderr.has_value()) {
            const auto& stderr_val = ServerParams.Stderr.value();
            if (holds_alternative<IOType>(stderr_val)) {
                const auto& stderr_str = get<IOType>(stderr_val);
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
    future<void> Start() override {
        return async(launch::async, [this]() {
            if (m_Started.exchange(true)) {
                throw runtime_error(
                    "StdioClientTransport already started! If using Client class, note "
                    "that connect() calls start() automatically.");
            }

            try {
                SpawnProcess();
                if (OnStart) { OnStart.value(); }
            } catch (const exception& e) {
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

    future<void> Close() override {
        return async(launch::async, [this]() {
            if (!m_Started.exchange(false)) { return; }

            m_AbortRequested = true;
            m_Process.reset();

            if (m_ReadBuffer) { m_ReadBuffer->Clear(); }

            if (m_MonitorThread.joinable()) { m_MonitorThread.join(); }

            if (OnClose) { OnClose.value(); }
        });
    }

    future<void> Send(const MessageBase& InMessage,
                      const TransportSendOptions& /* InOptions */ = {}) override {
        return async(launch::async, [this, &InMessage = InMessage]() {
            if (!m_Process || !m_Started) { throw runtime_error("Not connected"); }

            string serialized = SerializeMessage(InMessage);
            WriteToProcess(serialized + "\n");
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

    // The stderr stream of the child process, if StdioServerParameters.Stderr was set to "pipe" or
    // "overlapped".
    PassThrough* GetStderr() const {
        return m_StderrStream.get();
    }

  private:
    // Spawns the child process with the configured parameters
    void SpawnProcess() {
        // Build child process options
        ChildProcess::Options procOpts;
        procOpts.Command = m_ServerParams.Command;
        procOpts.Args = m_ServerParams.Args.value_or(vector<string>{});
        procOpts.Environment = m_ServerParams.Env.value_or(GetDefaultEnvironment());
        procOpts.WorkingDirectory = m_ServerParams.CWD;

        bool pipeErr = false;
        if (m_ServerParams.Stderr.has_value()) {
            if (holds_alternative<IOType>(m_ServerParams.Stderr.value())) {
                string mode = get<IOType>(m_ServerParams.Stderr.value());
                pipeErr = (mode == "pipe" || mode == "overlapped");
            }
        }
        procOpts.PipeStderr = pipeErr;

        // Callback: forward stdout chunks into ReadBuffer
        procOpts.StdoutCallback = [this](const vector<uint8_t>& data) {
            if (m_ReadBuffer) {
                m_ReadBuffer->Append(data);
                ProcessReadBuffer();
            }
        };

        if (pipeErr) {
            procOpts.StderrCallback = [this](const vector<uint8_t>& data) {
                if (m_StderrStream) {
                    m_StderrStream->write(reinterpret_cast<const char*>(data.data()), data.size());
                }
            };
        }

        m_Process = make_unique<ChildProcess>(procOpts);

        // Start thread to monitor child exit
        m_MonitorThread = thread([this]() {
            if (!m_Process) return;
            m_Process->WaitForExit();

            // Notify close on exit
            {
                lock_guard<mutex> lock(m_CallbackMutex);
                if (OnClose) { OnClose.value(); }
            }
            m_Started = false;
        });
    }

    // Processes the read buffer and extracts complete messages
    void ProcessReadBuffer() {
        while (true) {
            try {
                auto messageOpt = m_ReadBuffer->ReadMessage();
                if (!messageOpt) { break; }

                {
                    lock_guard<mutex> lock(m_CallbackMutex);
                    if (OnMessage && *messageOpt) {
                        OnMessage.value()(*messageOpt->get(), nullopt);
                    }
                }

                // Continue looping in case multiple messages are queued
            } catch (const exception& /* e */) {
                if (OnError) {
                    // TODO: Create proper ErrorBase from exception
                    // For now, this is a placeholder
                }
                break;
            }
        }
    }

    // Writes data to the child process stdin
    void WriteToProcess(const string& data) {
        if (m_Process) {
            m_Process->Write(data);
        } else {
            throw runtime_error("Process communication not available (process not started)");
        }
    }
};

MCP_NAMESPACE_END