#pragma once

#include "Communication/Transport/ITransport.h"
#include "Macros.h"

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
class ChildProcess {
  private:
    Poco::Process::PID m_ProcessID;
    unique_ptr<Poco::Pipe> m_StdinPipe;
    unique_ptr<Poco::Pipe> m_StdoutPipe;
    unique_ptr<Poco::Pipe> m_StderrPipe;
    unique_ptr<Poco::PipeOutputStream> m_StdinStream;
    unique_ptr<Poco::PipeInputStream> m_StdoutStream;
    unique_ptr<Poco::PipeInputStream> m_StderrStream;
    bool m_IsRunning;

  public:
    ChildProcess(const string& InExecutable, const vector<string>& InArguments)
        : m_IsRunning(false) {
        m_StdinPipe = make_unique<Poco::Pipe>();
        m_StdoutPipe = make_unique<Poco::Pipe>();
        m_StderrPipe = make_unique<Poco::Pipe>();

        Poco::Process::Args Args(InArguments.begin(), InArguments.end());

        try {
            Poco::ProcessHandle Handle = Poco::Process::launch(
                InExecutable, Args, m_StdinPipe.get(), m_StdoutPipe.get(), m_StderrPipe.get());

            m_ProcessID = Handle.id();
            m_IsRunning = true;

            m_StdinStream = make_unique<Poco::PipeOutputStream>(*m_StdinPipe);
            m_StdoutStream = make_unique<Poco::PipeInputStream>(*m_StdoutPipe);
            m_StderrStream = make_unique<Poco::PipeInputStream>(*m_StderrPipe);

        } catch (const Poco::Exception& Ex) {
            throw runtime_error("Failed to launch process: " + Ex.displayText());
        }
    }

    ~ChildProcess() {
        if (m_IsRunning) {
            try {
                Poco::Process::kill(m_ProcessID);
            } catch (...) {
                // Ignore errors during cleanup
            }
        }
    }

    MCPTask_Void WriteToStdin(const string& InData) {
        if (!m_IsRunning || !m_StdinStream) {
            throw runtime_error("Process not running or stdin not available");
        }

        try {
            *m_StdinStream << InData;
            m_StdinStream->flush();
        } catch (const Poco::Exception& Ex) {
            throw runtime_error("Failed to write to stdin: " + Ex.displayText());
        }

        co_return;
    }

    MCPTask_Void ReadLineFromStdout() {
        if (!m_IsRunning || !m_StdoutStream) { co_return string{}; }

        string Line;
        try {
            getline(*m_StdoutStream, Line);
        } catch (const Poco::Exception&) {
            // Stream may be closed
        }

        co_return Line;
    }

    MCPTask_Void ReadLineFromStderr() {
        if (!m_IsRunning || !m_StderrStream) { co_return string{}; }

        string Line;
        try {
            getline(*m_StderrStream, Line);
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
                throw runtime_error("Failed to wait for process: " + Ex.displayText());
            }
        }
        co_return 0;
    }

    bool IsRunning() const {
        return m_IsRunning;
    }
};

MCP_NAMESPACE_END