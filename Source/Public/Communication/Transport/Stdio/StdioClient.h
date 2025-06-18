#pragma once

#include "../ITransport.h"
#include "Communication/Utilities/AbortController.h"
#include "Communication/Utilities/ChildProcess.h"
#include "Communication/Utilities/ReadBuffer.h"
#include "Core.h"

MCP_NAMESPACE_BEGIN

// TODO: @HalcyonOmega Fix conversion of typescript to c++ for following:
using IOType = int;
using Stream = int;
using StderrType = variant<IOType, Stream, int>;
using PassThrough = int;

class StdioClientTransport : public ITransport {
  public:
    // === ITransport Implementation ===
    MCPTask_Void Connect() override;
    MCPTask_Void Disconnect() override;
    MCPTask_Void SendMessage(const MessageBase& InMessage) override;
    // === End ITransport Implementation ===

    // TODO: @HalcyonOmega Fix conversion of typescript to c++
    struct ServerParameters {
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
         *
         * If not specified, the result of getDefaultEnvironment() will be used.
         */
        optional<unordered_map<string, string>> Env;

        /**
         * How to handle stderr of the child process. This matches the semantics of Node's
         * `child_process.spawn`.
         *
         * The default is "inherit", meaning messages to stderr will be printed to the parent
         * process's stderr.
         */
        optional<StderrType> Stderr;

        /**
         * The working directory to use when spawning the process.
         *
         * If not specified, the current working directory will be inherited.
         */
        optional<string> CWD;
    };

  private:
    void ProcessReadBuffer();
    optional<PassThrough> GetStderr() const;

    optional<ChildProcess> m_Process;
    ReadBuffer m_ReadBuffer;
    ServerParameters m_ServerParams;
    optional<PassThrough> m_StderrStream;
    AbortController m_AbortController;

    // === ITransport Members ===
    optional<string> m_SessionID;

    // Callbacks
    optional<ConnectCallback> m_ConnectCallback;
    optional<DisconnectCallback> m_DisconnectCallback;
    optional<ErrorCallback> m_ErrorCallback;
    optional<MessageCallback> m_MessageCallback;
};

MCP_NAMESPACE_END