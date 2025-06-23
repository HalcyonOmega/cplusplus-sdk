#include "CoreSDK/Transport/StdioTransport.h"

#include <Poco/PipeStream.h>
#include <Poco/StreamCopier.h>

#include <chrono>
#include <iostream>
#include <thread>

MCP_NAMESPACE_BEGIN

// StdioTransport Implementation
StdioTransport::StdioTransport(const StdioTransportOptions& InOptions) : m_Options(InOptions) {
    TriggerStateChange(TransportState::Disconnected);
}

StdioTransport::~StdioTransport() {
    if (m_CurrentState != TransportState::Disconnected) {
        try {
            Stop().GetResult();
        } catch (...) {
            // Ignore errors during destruction
        }
    }
}

MCPTask_Void StdioTransport::Start() {
    if (m_CurrentState != TransportState::Disconnected) {
        throw std::runtime_error("Transport already started or in progress");
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
        if (m_ErrorHandler) {
            m_ErrorHandler("Failed to start stdio transport: " + std::string(e.what()));
        }
        throw;
    }

    co_return;
}

MCPTask_Void StdioTransport::Stop() {
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
        if (m_ErrorHandler) {
            m_ErrorHandler("Error stopping stdio transport: " + std::string(e.what()));
        }
    }

    co_return;
}

bool StdioTransport::IsConnected() const {
    return m_CurrentState == TransportState::Connected;
}

TransportState StdioTransport::GetState() const {
    return m_CurrentState;
}

MCPTask<std::string> StdioTransport::SendRequest(const RequestBase& InRequest) {
    if (!IsConnected()) { throw std::runtime_error("Transport not connected"); }

    RequestID requestID = InRequest.ID;

    // Create promise for response
    auto pendingRequest = std::make_unique<PendingRequest>();
    pendingRequest->RequestID = requestID;
    pendingRequest->StartTime = std::chrono::steady_clock::now();

    auto future = pendingRequest->Promise.get_future();

    {
        Poco::Mutex::ScopedLock lock(m_RequestsMutex);
        m_PendingRequests[requestID] = std::move(pendingRequest);
    }

    // Send the request
    co_await WriteMessage(InRequest);

    // Wait for response with timeout
    static constexpr std::chrono::seconds DEFAULT_RESPONSE_WAIT_FOR{30};
    auto status = future.wait_for(std::chrono::seconds(DEFAULT_RESPONSE_WAIT_FOR));
    if (status == std::future_status::timeout) {
        Poco::Mutex::ScopedLock lock(m_RequestsMutex);
        m_PendingRequests.erase(requestID);
        throw std::runtime_error("Request timeout");
    }

    co_return future.get();
}

MCPTask_Void StdioTransport::SendResponse(const ResponseBase& InResponse) {
    co_await WriteMessage(InResponse);
}

MCPTask_Void StdioTransport::SendErrorResponse(const ErrorBase& InError) {
    co_await WriteMessage(InError);
}

MCPTask_Void StdioTransport::SendNotification(const NotificationBase& InNotification) {
    co_await WriteMessage(InNotification);
}

void StdioTransport::SetMessageHandler(MessageHandler InHandler) {
    m_MessageHandler = InHandler;
}

void StdioTransport::SetRequestHandler(RequestHandler InHandler) {
    m_RequestHandler = InHandler;
}

void StdioTransport::SetResponseHandler(ResponseHandler InHandler) {
    m_ResponseHandler = InHandler;
}

void StdioTransport::SetNotificationHandler(NotificationHandler InHandler) {
    m_NotificationHandler = InHandler;
}

void StdioTransport::SetErrorHandler(ErrorHandler InHandler) {
    m_ErrorHandler = InHandler;
}

void StdioTransport::SetStateChangeHandler(StateChangeHandler InHandler) {
    m_StateChangeHandler = InHandler;
}

std::string StdioTransport::GetConnectionInfo() const {
    return "Stdio transport to: " + m_Options.Command;
}

void StdioTransport::run() {
    ProcessIncomingData();
}

void StdioTransport::ProcessIncomingData() {
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
                HandleError("Error reading from process: " + std::string(e.what()));
            }
            break;
        }
    }
}

void StdioTransport::ProcessLine(const std::string& InLine) {
    try {
        auto message = JSONValue::parse(InLine);

        if (!IsValidJSONRPC(message)) {
            HandleError("Invalid JSON-RPC message received");
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
        HandleError("Error parsing message: " + std::string(e.what()));
    }
}

MCPTask_Void StdioTransport::WriteMessage(const JSONValue& InMessage) {
    if (!m_StdinStream || !IsConnected()) { throw std::runtime_error("Transport not connected"); }

    try {
        Poco::Mutex::ScopedLock lock(m_WriteMutex);

        std::string messageStr = InMessage.dump() + "\n";
        m_StdinStream->write(messageStr.c_str(), messageStr.length());
        m_StdinStream->flush();
    } catch (const std::exception& e) {
        HandleError("Error writing message: " + std::string(e.what()));
        throw;
    }

    co_return;
}

void StdioTransport::HandleError(const std::string& InError) {
    TriggerStateChange(TransportState::Error);
    if (m_ErrorHandler) { m_ErrorHandler(InError); }
}

void StdioTransport::Cleanup() {
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
StdioServerTransport::StdioServerTransport() {
    TriggerStateChange(TransportState::Disconnected);
}

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
        throw std::runtime_error("Transport already started");
    }

    try {
        TriggerStateChange(TransportState::Connecting);

        // Server uses stdin/stdout directly
        m_ShouldStop = false;
        m_ReadThread.start(*this);

        TriggerStateChange(TransportState::Connected);
    } catch (const std::exception& e) {
        TriggerStateChange(TransportState::Error);
        if (m_ErrorHandler) {
            m_ErrorHandler("Failed to start stdio server transport: " + std::string(e.what()));
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
        if (m_ErrorHandler) {
            m_ErrorHandler("Error stopping stdio server transport: " + std::string(e.what()));
        }
    }

    co_return;
}

bool StdioServerTransport::IsConnected() const {
    return m_CurrentState == TransportState::Connected;
}

TransportState StdioServerTransport::GetState() const {
    return m_CurrentState;
}

MCPTask<const ResponseBase&> StdioServerTransport::SendRequest(const RequestBase& InRequest) {
    if (!IsConnected()) { throw std::runtime_error("Transport not connected"); }

    RequestID requestID = InRequest.ID;

    // Create promise for response
    auto pendingRequest = std::make_unique<PendingRequest>();
    pendingRequest->RequestID = requestID;
    pendingRequest->StartTime = std::chrono::steady_clock::now();

    auto future = pendingRequest->Promise.get_future();

    {
        Poco::Mutex::ScopedLock lock(m_RequestsMutex);
        m_PendingRequests[requestID] = std::move(pendingRequest);
    }

    // Send the request
    co_await WriteMessage(InRequest);

    // Wait for response with timeout
    static constexpr std::chrono::seconds DEFAULT_RESPONSE_WAIT_FOR{30000};
    auto status = future.wait_for(std::chrono::seconds(DEFAULT_RESPONSE_WAIT_FOR));
    if (status == std::future_status::timeout) {
        Poco::Mutex::ScopedLock lock(m_RequestsMutex);
        m_PendingRequests.erase(requestID);
        throw std::runtime_error("Request timeout");
    }

    co_return future.get();
}

MCPTask_Void StdioServerTransport::SendResponse(const ResponseBase& InResponse) {
    co_await WriteMessage(InResponse);
}

MCPTask_Void StdioServerTransport::SendErrorResponse(const ErrorBase& InError) {
    co_await WriteMessage(InError);
}

MCPTask_Void StdioServerTransport::SendNotification(const NotificationBase& InNotification) {
    co_await WriteMessage(InNotification);
}

void StdioServerTransport::SetMessageHandler(MessageHandler InHandler) {
    m_MessageHandler = InHandler;
}

void StdioServerTransport::SetRequestHandler(RequestHandler InHandler) {
    m_RequestHandler = InHandler;
}

void StdioServerTransport::SetResponseHandler(ResponseHandler InHandler) {
    m_ResponseHandler = InHandler;
}

void StdioServerTransport::SetNotificationHandler(NotificationHandler InHandler) {
    m_NotificationHandler = InHandler;
}

void StdioServerTransport::SetErrorHandler(ErrorHandler InHandler) {
    m_ErrorHandler = InHandler;
}

void StdioServerTransport::SetStateChangeHandler(StateChangeHandler InHandler) {
    m_StateChangeHandler = InHandler;
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
                HandleError("Error reading from stdin: " + std::string(e.what()));
            }
            break;
        }
    }
}

void StdioServerTransport::ProcessLine(const std::string& InLine) {
    try {
        auto message = JSONValue::parse(InLine);

        if (!IsValidJSONRPC(message)) {
            HandleError("Invalid JSON-RPC message received");
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
        HandleError("Error parsing message: " + std::string(e.what()));
    }
}

MCPTask_Void StdioServerTransport::WriteMessage(const JSONValue& InMessage) {
    try {
        Poco::Mutex::ScopedLock lock(m_WriteMutex);

        std::string messageStr = InMessage.dump() + "\n";
        std::cout << messageStr;
        std::cout.flush();
    } catch (const std::exception& e) {
        HandleError("Error writing message: " + std::string(e.what()));
        throw;
    }

    co_return;
}

void StdioServerTransport::HandleError(const std::string& InError) {
    TriggerStateChange(TransportState::Error);
    if (m_ErrorHandler) { m_ErrorHandler(InError); }
}

// Factory functions
std::unique_ptr<ITransport> CreateStdioTransportImpl(const StdioTransportOptions& InOptions) {
    return std::make_unique<StdioTransport>(InOptions);
}

MCP_NAMESPACE_END