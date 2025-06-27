#include "CoreSDK/Features/PromptManager.h"

#include "CoreSDK/Common/Logging.h"

MCP_NAMESPACE_BEGIN

PromptManager::PromptManager(bool InWarnOnDuplicatePrompts)
    : m_WarnOnDuplicatePrompts(InWarnOnDuplicatePrompts) {}

Prompt PromptManager::AddPrompt(const Prompt& InPrompt) {
    // Log the addition attempt
    MCP::Logger::Debug("Adding prompt: " + InPrompt.Name);

    const auto ExistingIt = m_Prompts.find(InPrompt.Name);
    if (ExistingIt != m_Prompts.end()) {
        if (m_WarnOnDuplicatePrompts) {
            MCP::Logger::Warning("Prompt already exists: " + InPrompt.Name);
        }
        return ExistingIt->second;
    }

    m_Prompts[InPrompt.Name] = InPrompt;
    return InPrompt;
}

std::optional<Prompt> PromptManager::GetPrompt(const std::string& InName) const {
    const auto It = m_Prompts.find(InName);
    if (It != m_Prompts.end()) { return It->second; }
    return std::nullopt;
}

std::vector<Prompt> PromptManager::ListPrompts() const {
    std::vector<Prompt> Result;
    Result.reserve(m_Prompts.size());

    for (const auto& [Name, PromptData] : m_Prompts) { Result.push_back(PromptData); }

    return Result;
}

bool PromptManager::HasPrompt(const std::string& InName) const {
    return m_Prompts.find(InName) != m_Prompts.end();
}

MCP_NAMESPACE_END