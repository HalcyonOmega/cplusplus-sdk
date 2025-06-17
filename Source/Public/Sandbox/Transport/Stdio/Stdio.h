#pragma once

#include "../ITransport.h"
#include "Core.h"

// Poco Net includes
#include <Poco/Event.h>
#include <Poco/Exception.h>
#include <Poco/Net/NetException.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Pipe.h>
#include <Poco/PipeStream.h>
#include <Poco/Process.h>
#include <Poco/Runnable.h>
#include <Poco/StreamCopier.h>
#include <Poco/Thread.h>
#include <Poco/URI.h>

MCP_NAMESPACE_BEGIN

// Process wrapper for Poco::Process
class ProcessWrapper {
  private:
    Poco::Process::PID m_ProcessID;
    std::unique_ptr<Poco::Pipe> m_StdinPipe;
    std::unique_ptr<Poco::Pipe> m_StdoutPipe;
    std::unique_ptr<Poco::Pipe> m_StderrPipe;
    std::unique_ptr<Poco::PipeOutputStream> m_StdinStream;
    std::unique_ptr<Poco::PipeInputStream> m_StdoutStream;
    std::unique_ptr<Poco::PipeInputStream> m_StderrStream;
    bool m_IsRunning;

  public:
    ProcessWrapper(const std::string& InExecutable, const std::vector<std::string>& InArguments)
        : m_IsRunning(false) {
        m_StdinPipe = std::make_unique<Poco::Pipe>();
        m_StdoutPipe = std::make_unique<Poco::Pipe>();
        m_StderrPipe = std::make_unique<Poco::Pipe>();

        Poco::Process::Args Args(InArguments.begin(), InArguments.end());

        try {
            Poco::ProcessHandle Handle = Poco::Process::launch(
                InExecutable, Args, m_StdinPipe.get(), m_StdoutPipe.get(), m_StderrPipe.get());

            m_ProcessID = Handle.id();
            m_IsRunning = true;

            m_StdinStream = std::make_unique<Poco::PipeOutputStream>(*m_StdinPipe);
            m_StdoutStream = std::make_unique<Poco::PipeInputStream>(*m_StdoutPipe);
            m_StderrStream = std::make_unique<Poco::PipeInputStream>(*m_StderrPipe);

        } catch (const Poco::Exception& Ex) {
            throw std::runtime_error("Failed to launch process: " + Ex.displayText());
        }
    }

    ~ProcessWrapper() {
        if (m_IsRunning) {
            try {
                Poco::Process::kill(m_ProcessID);
            } catch (...) {
                // Ignore errors during cleanup
            }
        }
    }

    MCPTask_Void WriteToStdin(const std::string& InData) {
        if (!m_IsRunning || !m_StdinStream) {
            throw std::runtime_error("Process not running or stdin not available");
        }

        try {
            *m_StdinStream << InData;
            m_StdinStream->flush();
        } catch (const Poco::Exception& Ex) {
            throw std::runtime_error("Failed to write to stdin: " + Ex.displayText());
        }

        co_return;
    }

    MCPTask_Void ReadLineFromStdout() {
        if (!m_IsRunning || !m_StdoutStream) { co_return std::string{}; }

        std::string Line;
        try {
            std::getline(*m_StdoutStream, Line);
        } catch (const Poco::Exception&) {
            // Stream may be closed
        }

        co_return Line;
    }

    MCPTask_Void ReadLineFromStderr() {
        if (!m_IsRunning || !m_StderrStream) { co_return std::string{}; }

        std::string Line;
        try {
            std::getline(*m_StderrStream, Line);
        } catch (const Poco::Exception&) {
            // Stream may be closed
        }

        co_return Line;
    }

    void CloseStdin() {
        if (m_StdinStream) {
            m_StdinStream->close();
            m_StdinStream.reset();
        }
    }

    MCPTask_Void WaitForExit() {
        if (m_IsRunning) {
            try {
                int ExitCode = Poco::Process::wait(m_ProcessID);
                m_IsRunning = false;
                co_return ExitCode;
            } catch (const Poco::Exception& Ex) {
                throw std::runtime_error("Failed to wait for process: " + Ex.displayText());
            }
        }
        co_return 0;
    }

    bool IsRunning() const {
        return m_IsRunning;
    }
};

// stdio Transport Implementation using Poco::Process
class StdioTransport : public ITransport {
  private:
    std::string m_ExecutablePath;
    std::vector<std::string> m_Arguments;
    std::unique_ptr<ProcessWrapper> m_ServerProcess;
    std::function<void(const MessageBase&)> m_MessageHandler;
    std::function<void(const std::string&)> m_ErrorHandler;
    bool m_IsConnected;

  public:
    StdioTransport(const std::string& InExecutablePath, const std::vector<std::string>& InArguments)
        : m_ExecutablePath(InExecutablePath), m_Arguments(InArguments), m_IsConnected(false) {}

    MCPTask_Void Connect() override {
        try {
            // Launch subprocess using Poco::Process
            m_ServerProcess = std::make_unique<ProcessWrapper>(m_ExecutablePath, m_Arguments);

            // Start reading from stdout
            co_await StartReadingFromStdout();

            // Start reading from stderr for logging
            co_await StartReadingFromStderr();

            m_IsConnected = true;
        } catch (const std::exception& Ex) {
            if (m_ErrorHandler) {
                m_ErrorHandler("Failed to connect stdio transport: " + std::string(Ex.what()));
            }
            throw;
        }

        co_return;
    }

    MCPTask_Void Disconnect() override {
        if (m_ServerProcess) {
            // Close stdin to signal termination
            m_ServerProcess->CloseStdin();

            // Wait for process to terminate
            co_await m_ServerProcess->WaitForExit();

            m_ServerProcess.reset();
        }
        m_IsConnected = false;
        co_return;
    }

    MCPTask_Void SendMessage(const MessageBase& InMessage) override {
        if (!m_IsConnected || !m_ServerProcess) {
            throw std::runtime_error("Transport not connected");
        }

        // Serialize message to JSON
        std::string JsonData = SerializeToJSON(InMessage);

        // Ensure no embedded newlines (spec requirement)
        if (JsonData.find('\n') != std::string::npos) {
            throw std::runtime_error("Message contains embedded newlines");
        }

        // Write to stdin with newline delimiter
        co_await m_ServerProcess->WriteToStdin(JsonData + "\n");
        co_return;
    }

    MCPTask_Void SendBatch(const JSONRPCBatch& InBatch) override {
        MessageBase BatchMessage = ConvertBatchToMessage(InBatch);
        co_await SendMessage(BatchMessage);
        co_return;
    }

    void SetMessageHandler(std::function<void(const MessageBase&)> InHandler) override {
        m_MessageHandler = InHandler;
    }

    void SetErrorHandler(std::function<void(const std::string&)> InHandler) override {
        m_ErrorHandler = InHandler;
    }

    bool IsConnected() const override {
        return m_IsConnected;
    }
    std::string GetTransportType() const override {
        return "stdio";
    }

  private:
    MCPTask_Void StartReadingFromStdout() {
        while (m_IsConnected && m_ServerProcess && m_ServerProcess->IsRunning()) {
            std::string Line = co_await m_ServerProcess->ReadLineFromStdout();
            if (!Line.empty()) { ProcessReceivedMessage(Line); }
        }
        co_return;
    }

    MCPTask_Void StartReadingFromStderr() {
        while (m_IsConnected && m_ServerProcess && m_ServerProcess->IsRunning()) {
            std::string LogLine = co_await m_ServerProcess->ReadLineFromStderr();
            if (!LogLine.empty() && m_ErrorHandler) { m_ErrorHandler(LogLine); }
        }
        co_return;
    }

    void ProcessReceivedMessage(const std::string& InJsonData) {
        try {
            MessageBase Message = DeserializeFromJSON(InJsonData);
            if (m_MessageHandler) { m_MessageHandler(Message); }
        } catch (const std::exception& Ex) {
            if (m_ErrorHandler) {
                m_ErrorHandler("Failed to parse JSON-RPC message: " + std::string(Ex.what()));
            }
        }
    }
};

MCP_NAMESPACE_END