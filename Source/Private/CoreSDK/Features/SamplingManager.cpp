#include "CoreSDK/Features/SamplingManager.h"

#include <future>

#include "CoreSDK/Common/Logging.h"

MCP_NAMESPACE_BEGIN

SamplingManager::SamplingManager(const std::optional<ModelPreferences>& InDefaultModelPreferences)
    : m_DefaultModelPreferences(InDefaultModelPreferences) {}

void SamplingManager::SetSamplingFunction(SamplingFunction InSamplingFunction) {
    std::lock_guard<std::mutex> Lock(m_SamplingMutex);

    m_SamplingFunction = std::move(InSamplingFunction);
    Logger::Debug("Sampling function has been configured");
}

std::future<SamplingResult>
SamplingManager::RequestSampling(const std::vector<SamplingMessage>& InMessages,
                                 const std::optional<ModelPreferences>& InModelPreferences,
                                 MCPContext* InContext) {
    std::lock_guard<std::mutex> Lock(m_SamplingMutex);

    if (!m_SamplingFunction.has_value()) {
        return std::async(std::launch::async, []() -> SamplingResult {
            throw SamplingError("No sampling function configured");
        });
    }

    Logger::Debug("Requesting LLM sampling with " + std::to_string(InMessages.size())
                  + " messages");

    const auto EffectivePreferences = GetEffectiveModelPreferences(InModelPreferences);
    return m_SamplingFunction.value()(InMessages, EffectivePreferences, InContext);
}

SamplingResult
SamplingManager::RequestSamplingSync(const std::vector<SamplingMessage>& InMessages,
                                     const std::optional<ModelPreferences>& InModelPreferences,
                                     MCPContext* InContext) {
    auto Future = RequestSampling(InMessages, InModelPreferences, InContext);
    return Future.get();
}

void SamplingManager::SetDefaultModelPreferences(const ModelPreferences& InModelPreferences) {
    std::lock_guard<std::mutex> Lock(m_SamplingMutex);

    m_DefaultModelPreferences = InModelPreferences;
    Logger::Debug("Default model preferences updated");
}

std::optional<ModelPreferences> SamplingManager::GetDefaultModelPreferences() const {
    std::lock_guard<std::mutex> Lock(m_SamplingMutex);

    return m_DefaultModelPreferences;
}

void SamplingManager::ClearDefaultModelPreferences() {
    std::lock_guard<std::mutex> Lock(m_SamplingMutex);

    m_DefaultModelPreferences.reset();
    Logger::Debug("Default model preferences cleared");
}

bool SamplingManager::HasSamplingFunction() const {
    std::lock_guard<std::mutex> Lock(m_SamplingMutex);

    return m_SamplingFunction.has_value();
}

SamplingMessage SamplingManager::CreateMessage(
    const MCP::Role& InRole,
    const std::variant<TextContent, ImageContent, AudioContent>& InContent) {
    SamplingMessage Message;
    Message.Role = InRole;
    Message.Content = InContent;
    return Message;
}

ModelPreferences
SamplingManager::CreateModelPreferences(const std::optional<std::vector<ModelHint>>& InHints,
                                        const std::optional<double>& InCostPriority,
                                        const std::optional<double>& InSpeedPriority,
                                        const std::optional<double>& InIntelligencePriority) {
    // Validate priority values if provided
    if (InCostPriority.has_value()
        && (InCostPriority.value() < 0.0 || InCostPriority.value() > 1.0)) {
        throw SamplingError("Cost priority must be between 0.0 and 1.0");
    }
    if (InSpeedPriority.has_value()
        && (InSpeedPriority.value() < 0.0 || InSpeedPriority.value() > 1.0)) {
        throw SamplingError("Speed priority must be between 0.0 and 1.0");
    }
    if (InIntelligencePriority.has_value()
        && (InIntelligencePriority.value() < 0.0 || InIntelligencePriority.value() > 1.0)) {
        throw SamplingError("Intelligence priority must be between 0.0 and 1.0");
    }

    ModelPreferences Preferences;
    Preferences.Hints = InHints;
    Preferences.CostPriority = InCostPriority;
    Preferences.SpeedPriority = InSpeedPriority;
    Preferences.IntelligencePriority = InIntelligencePriority;
    return Preferences;
}

std::optional<ModelPreferences> SamplingManager::GetEffectiveModelPreferences(
    const std::optional<ModelPreferences>& InModelPreferences) const {
    // Request-specific preferences take precedence over defaults
    if (InModelPreferences.has_value()) { return InModelPreferences; }

    return m_DefaultModelPreferences;
}

MCP_NAMESPACE_END
