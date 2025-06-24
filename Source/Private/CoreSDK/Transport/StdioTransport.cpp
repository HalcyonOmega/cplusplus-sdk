#include "CoreSDK/Transport/StdioTransport.h"

#include <Poco/PipeStream.h>
#include <Poco/StreamCopier.h>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "CoreSDK/Common/RuntimeError.h"

MCP_NAMESPACE_BEGIN

// StdioClientTransport Implementation
StdioClientTransport::StdioClientTransport(const StdioClientTransportOptions& InOptions)
    : m_Options(InOptions) {}

StdioClientTransport::~StdioClientTransport() {
    if (m_CurrentState != TransportState::Disconnected) {
        try {
            Stop().GetResult();
        } catch (...) {
            // Ignore errors during destruction
        }
    }
}

MCPTask_Void StdioClientTransport::Start() {
    if (m_CurrentState != TransportState::Disconnected) {
        HandleRuntimeError("Transport already started or in progress");
        co_return;
    }

    try {
        TriggerStateChange(TransportState::Connecting);

        // Create pipes for communication
        m_StdinPipe = std::make_unique<Poco::Pipe>();
        m_StdoutPipe = std::make_unique<Poco::Pipe>();
        if (m_Options.UseStderr) { m_StderrPipe = std::make_unique<Poco::Pipe>(); }

        // Prepare process arguments
        std::vector<std::string> args = m_Options.Arguments;

        // Launch the process
        if (m_Options.UseStderr) {
            m_ProcessHandle = std::make_unique<Poco::ProcessHandle>(Poco::Process::launch(
                m_Options.Command, args, *m_StdinPipe, *m_StdoutPipe, *m_StderrPipe));
        } else {
            m_ProcessHandle = std::make_unique<Poco::ProcessHandle>(Poco::Process::launch(
                m_Options.Command, args, *m_StdinPipe, *m_StdoutPipe, *m_StdoutPipe));
        }

        // Create streams
        m_StdinStream = std::make_unique<Poco::PipeOutputStream>(*m_StdinPipe);
        m_StdoutStream = std::make_unique<Poco::PipeInputStream>(*m_StdoutPipe);
        if (m_Options.UseStderr) {
            m_StderrStream = std::make_unique<Poco::PipeInputStream>(*m_StderrPipe);
        }

        // Start reading thread
        m_ShouldStop = false;
        m_ReadThread.start(*this);

        TriggerStateChange(TransportState::Connected);
    } catch (const std::exception& e) {
        TriggerStateChange(TransportState::Error);
        HandleRuntimeError("Failed to start stdio transport: " + std::string(e.what()));
        co_return;
    }

    co_return;
}

MCPTask_Void StdioClientTransport::Stop() {
    if (m_CurrentState == TransportState::Disconnected) { co_return; }

    try {
        m_ShouldStop = true;

        // Close streams to wake up reading thread
        if (m_StdinStream) { m_StdinStream->close(); }

        // Wait for reading thread to finish
        if (m_ReadThread.isRunning()) { m_ReadThread.join(); }

        Cleanup();
        TriggerStateChange(TransportState::Disconnected);
    } catch (const std::exception& e) {
        TriggerStateChange(TransportState::Error);
        HandleRuntimeError("Error stopping stdio transport: " + std::string(e.what()));
    }

    co_return;
}

std::string StdioClientTransport::GetConnectionInfo() const {
    return "Stdio transport to: " + m_Options.Command;
}

void StdioClientTransport::run() {
    ProcessIncomingData();
}

void StdioClientTransport::ProcessIncomingData() {
    static constexpr std::chrono::milliseconds DEFAULT_SLEEP_FOR{10};

    std::string line;
    while (!m_ShouldStop) {
        try {
            if (m_StdoutStream && m_StdoutStream->good()) {
                std::getline(*m_StdoutStream, line);
                if (!line.empty()) { ProcessLine(line); }
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_SLEEP_FOR));
            }
        } catch (const std::exception& e) {
            if (!m_ShouldStop) {
                HandleRuntimeError("Error reading from process: " + std::string(e.what()));
            }
            break;
        }
    }
}

void StdioClientTransport::ProcessLine(const std::string& InLine) {
    try {
        auto message = JSONValue::parse(InLine);

        if (!IsValidJSONRPC(message)) {
            HandleRuntimeError("Invalid JSON-RPC message received");
            return;
        }

        if (m_MessageHandler) { m_MessageHandler(InLine); }

        // Check if it's a response to a pending request
        if (message.contains("id") && (message.contains("result") || message.contains("error"))) {
            std::string requestID = MessageUtils::ExtractRequestID(message);

            Poco::Mutex::ScopedLock lock(m_RequestsMutex);
            auto it = m_PendingRequests.find(requestID);
            if (it != m_PendingRequests.end()) {
                if (message.contains("result")) {
                    it->second->Promise.set_value(message["result"].dump());
                } else {
                    std::string errorMsg = message["error"]["message"].get<std::string>();
                    it->second->Promise.set_exception(
                        std::make_exception_ptr(std::runtime_error(errorMsg)));
                }
                m_PendingRequests.erase(it);
                return;
            }
        }

        // Handle as request or notification
        if (message.contains("method")) {
            std::string method = MessageUtils::ExtractMethod(message);
            JSONValue params = MessageUtils::ExtractParams(message);

            if (message.contains("id")) {
                // Request
                if (m_RequestHandler) {
                    std::string requestID = MessageUtils::ExtractRequestID(message);
                    m_RequestHandler(method, params, requestID);
                }
            } else {
                // Notification
                if (m_NotificationHandler) { m_NotificationHandler(method, params); }
            }
        }
    } catch (const std::exception& e) {
        HandleRuntimeError("Error parsing message: " + std::string(e.what()));
    }
}

MCPTask_Void StdioClientTransport::TransmitMessage(const JSONValue& InMessage) {
    if (!m_StdinStream || !IsConnected()) {
        HandleRuntimeError("Transport not connected");
        co_return;
    }

    try {
        Poco::Mutex::ScopedLock lock(m_WriteMutex);

        std::string messageStr = InMessage.dump() + "\n";
        m_StdinStream->write(messageStr.c_str(), messageStr.length());
        m_StdinStream->flush();
    } catch (const std::exception& e) {
        HandleRuntimeError("Error writing message: " + std::string(e.what()));
        co_return;
    }

    co_return;
}

void StdioClientTransport::Cleanup() {
    // Terminate process if still running
    if (m_ProcessHandle) {
        try {
            Poco::Process::kill(*m_ProcessHandle);
        } catch (...) {
            // Ignore errors during cleanup
        }
    }

    // Close streams
    m_StdinStream.reset();
    m_StdoutStream.reset();
    m_StderrStream.reset();

    // Close pipes
    m_StdinPipe.reset();
    m_StdoutPipe.reset();
    m_StderrPipe.reset();

    m_ProcessHandle.reset();

    // Clear pending requests
    {
        Poco::Mutex::ScopedLock lock(m_RequestsMutex);
        for (auto& [id, request] : m_PendingRequests) {
            request->Promise.set_exception(
                std::make_exception_ptr(std::runtime_error("Transport closed")));
        }
        m_PendingRequests.clear();
    }
}

// StdioServerTransport Implementation
StdioServerTransport::StdioServerTransport() {}

StdioServerTransport::~StdioServerTransport() {
    if (m_CurrentState != TransportState::Disconnected) {
        try {
            Stop().GetResult();
        } catch (...) {
            // Ignore errors during destruction
        }
    }
}

MCPTask_Void StdioServerTransport::Start() {
    if (m_CurrentState != TransportState::Disconnected) {
        HandleRuntimeError("Transport already started");
        co_return;
    }

    try {
        TriggerStateChange(TransportState::Connecting);

        // Server uses stdin/stdout directly
        m_ShouldStop = false;
        m_ReadThread.start(*this);

        TriggerStateChange(TransportState::Connected);
    } catch (const std::exception& e) {
        TriggerStateChange(TransportState::Error);
        if (m_ErrorResponseHandler) {
            m_ErrorResponseHandler("Failed to start stdio server transport: "
                                   + std::string(e.what()));
        }
        throw;
    }

    co_return;
}

MCPTask_Void StdioServerTransport::Stop() {
    if (m_CurrentState == TransportState::Disconnected) { co_return; }

    try {
        m_ShouldStop = true;

        // Wait for reading thread to finish
        if (m_ReadThread.isRunning()) { m_ReadThread.join(); }

        // Clear pending requests
        {
            Poco::Mutex::ScopedLock lock(m_RequestsMutex);
            for (auto& [id, request] : m_PendingRequests) {
                request->Promise.set_exception(
                    std::make_exception_ptr(std::runtime_error("Transport stopped")));
            }
            m_PendingRequests.clear();
        }

        TriggerStateChange(TransportState::Disconnected);
    } catch (const std::exception& e) {
        TriggerStateChange(TransportState::Error);
        HandleRuntimeError("Error stopping stdio server transport: " + std::string(e.what()));
    }

    co_return;
}

std::string StdioServerTransport::GetConnectionInfo() const {
    return "Stdio server transport (stdin/stdout)";
}

void StdioServerTransport::run() {
    ProcessIncomingData();
}

void StdioServerTransport::ProcessIncomingData() {
    std::string line;
    while (!m_ShouldStop) {
        try {
            if (std::getline(std::cin, line)) {
                if (!line.empty()) { ProcessLine(line); }
            } else {
                // EOF reached
                break;
            }
        } catch (const std::exception& e) {
            if (!m_ShouldStop) {
                HandleRuntimeError("Error reading from stdin: " + std::string(e.what()));
            }
            break;
        }
    }
}

void StdioServerTransport::ProcessLine(const std::string& InLine) {
    try {
        auto message = JSONValue::parse(InLine);

        if (!IsValidJSONRPC(message)) {
            HandleRuntimeError("Invalid JSON-RPC message received");
            return;
        }

        if (m_MessageHandler) { m_MessageHandler(InLine); }

        // Check if it's a response to a pending request
        if (message.contains("id") && (message.contains("result") || message.contains("error"))) {
            std::string requestID = MessageUtils::ExtractRequestID(message);

            Poco::Mutex::ScopedLock lock(m_RequestsMutex);
            auto Iterator = m_PendingRequests.find(requestID);
            if (Iterator != m_PendingRequests.end()) {
                if (message.contains("result")) {
                    Iterator->second->Promise.set_value(message["result"].dump());
                } else {
                    std::string errorMsg = message["error"]["message"].get<std::string>();
                    Iterator->second->Promise.set_exception(
                        std::make_exception_ptr(std::runtime_error(errorMsg)));
                }
                m_PendingRequests.erase(Iterator);
                return;
            }
        }

        // Handle as request or notification
        if (message.contains("method")) {
            std::string method = MessageUtils::ExtractMethod(message);
            JSONValue params = MessageUtils::ExtractParams(message);

            if (message.contains("id")) {
                // Request
                if (m_RequestHandler) {
                    std::string requestID = MessageUtils::ExtractRequestID(message);
                    m_RequestHandler(method, params, requestID);
                }
            } else {
                // Notification
                if (m_NotificationHandler) { m_NotificationHandler(method, params); }
            }
        }
    } catch (const std::exception& e) {
        HandleRuntimeError("Error parsing message: " + std::string(e.what()));
    }
}

MCPTask_Void StdioServerTransport::TransmitMessage(const JSONValue& InMessage) {
    try {
        Poco::Mutex::ScopedLock lock(m_WriteMutex);

        std::string messageStr = InMessage.dump() + "\n";
        std::cout << messageStr;
        std::cout.flush();
    } catch (const std::exception& e) {
        HandleRuntimeError("Error writing message: " + std::string(e.what()));
    }

    co_return;
}

// Factory functions
std::unique_ptr<ITransport>
CreateStdioClientTransportImpl(const StdioClientTransportOptions& InOptions) {
    return std::make_unique<StdioClientTransport>(InOptions);
}

MCP_NAMESPACE_END