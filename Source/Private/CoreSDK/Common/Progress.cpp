#include "CoreSDK/Common/Progress.h"

MCP_NAMESPACE_BEGIN

ProgressTracker::ProgressTracker(const std::string& InRequestID,
                                 std::shared_ptr<MCPProtocol> InProtocol)
    : m_RequestID(InRequestID), m_Protocol(InProtocol) {}

MCPTask_Void ProgressTracker::UpdateProgress(double InProgress, std::optional<int64_t> InTotal) {
    if (m_IsComplete.load() || !m_Protocol) { co_return; }

    try {
        ProgressNotification notification;
        notification.ProgressToken = m_RequestID;
        notification.Progress = InProgress;
        if (InTotal.has_value()) { notification.Total = *InTotal; }

        co_await m_Protocol->SendNotification("notifications/progress", JSONValue(notification));
    } catch (const std::exception&) {
        // Ignore progress reporting errors to not break main operation
    }

    co_return;
}

MCPTask_Void ProgressTracker::CompleteProgress() {
    bool expected = false;
    if (m_IsComplete.compare_exchange_strong(expected, true)) {
        // Send 100% completion
        co_await UpdateProgress(1.0);
    }
    co_return;
}

MCP_NAMESPACE_END