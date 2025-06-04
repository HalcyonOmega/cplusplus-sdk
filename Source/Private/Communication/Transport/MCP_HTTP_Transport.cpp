#include "Communication/Transport/MCP_HTTP_Transport.h"

#include <curl/curl.h>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

#include "Communication/Utilities/HTTPClient.h"
#include "Core.h"
#include "Core/Constants/HTTPConstants.h"

MCP_NAMESPACE_BEGIN

MCP_HTTP_Transport::MCP_HTTP_Transport(const std::string& Endpoint)
    : endpoint_(Endpoint), open_(false), http_client_(std::make_unique<HTTPClient>()) {}
MCP_HTTP_Transport::~MCP_HTTP_Transport() = default;

bool MCP_HTTP_Transport::SendMessage(const MCP_MessageBase& Message) {
    try {
        std::string Body = Message.GetMessage().dump();
        std::vector<std::string> HTTPHeaders = {
            std::string(HTTP_HEADER_CONTENT_TYPE) + HTTP_HEADER_SEPARATOR + HTTP_CONTENT_TYPE_JSON,
            std::string(HTTP_HEADER_ACCEPT) + HTTP_HEADER_SEPARATOR
                + HTTP_ACCEPT_JSON_AND_EVENT_STREAM};
        if (!session_id_.empty()) {
            HTTPHeaders.push_back(std::string(HTTP_HEADER_MCP_SESSION_ID) + HTTP_HEADER_SEPARATOR
                                  + session_id_);
        }
        std::string HTTPResponse = http_client_->Post(endpoint_, Body, HTTPHeaders);
        // Optionally, parse response for session id or errors
        // If server returns session id in header, extract and store it (not shown here)
        return true;
    } catch (const std::exception& Exception) {
        // Log or handle error as needed
        return false;
    }
}

std::unique_ptr<MCP_MessageBase> MCP_HTTP_Transport::ReceiveMessage() {
    // Blocking call: opens SSE stream, waits for one event, returns it as MCP_MessageBase
    std::unique_ptr<MCP_MessageBase> Result;
    std::mutex Mutex;
    std::condition_variable ConditionVar;
    bool IsReceived = false;
    std::string EventData;
    auto OnEvent = [&](const std::string& Data) {
        std::lock_guard<std::mutex> Lock(Mutex);
        EventData = Data;
        IsReceived = true;
        ConditionVar.notify_one();
    };
    std::vector<std::string> HTTPHeaders = {std::string(HTTP_HEADER_ACCEPT) + HTTP_HEADER_SEPARATOR
                                            + HTTP_ACCEPT_EVENT_STREAM};
    if (!session_id_.empty()) {
        HTTPHeaders.push_back(std::string(HTTP_HEADER_MCP_SESSION_ID) + HTTP_HEADER_SEPARATOR
                              + session_id_);
    }
    std::thread SSEThread([&]() {
        try {
            http_client_->GetSSE(endpoint_, HTTPHeaders, OnEvent);
        } catch (...) {
            // Ignore errors for now
        }
    });
    {
        std::unique_lock<std::mutex> Lock(Mutex);
        ConditionVar.wait(Lock, [&] { return IsReceived; });
    }
    SSEThread.detach();
    if (!EventData.empty()) {
        try {
            auto JSONObject = JSON::parse(EventData);
            Result = std::make_unique<MCP_MessageBase>();
            Result->SetMessage(JSONObject);
        } catch (...) {
            // Ignore parse errors
        }
    }
    return Result;
}

void MCP_HTTP_Transport::Start() {
    open_ = true;
}

void MCP_HTTP_Transport::Stop() {
    open_ = false;
}

bool MCP_HTTP_Transport::IsOpen() const {
    return open_;
}

void MCP_HTTP_Transport::SetSessionID(const SessionID& SessionId) {
    session_id_ = SessionId;
}

SessionID MCP_HTTP_Transport::GetSessionID() const {
    return session_id_;
}

std::string MCP_HTTP_Transport::GetTransportType() const {
    return HTTP_TRANSPORT_TYPE;
}

MCP_NAMESPACE_END
