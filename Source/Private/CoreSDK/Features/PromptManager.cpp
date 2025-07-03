#include "CoreSDK/Features/PromptManager.h"

#include "CoreSDK/Common/Logging.h"

MCP_NAMESPACE_BEGIN

PromptManager::PromptManager(bool InWarnOnDuplicatePrompts)
    : m_WarnOnDuplicatePrompts(InWarnOnDuplicatePrompts) {}

bool PromptManager::AddPrompt(const Prompt& InPrompt) {
    // Log the addition attempt
    Logger::Debug("Adding prompt: " + InPrompt.Name);
    std::lock_guard<std::mutex> Lock(m_Mutex);

    const auto ExistingIt = m_Prompts.find(InPrompt.Name);
    if (ExistingIt != m_Prompts.end()) {
        if (m_WarnOnDuplicatePrompts) {
            Logger::Warning("Prompt already exists: " + InPrompt.Name);
        }
        return false;
    }

    m_Prompts[InPrompt.Name] = InPrompt;
    return true;
}

bool PromptManager::RemovePrompt(const Prompt& InPrompt) {
    std::lock_guard<std::mutex> Lock(m_Mutex);

    const auto ExistingIt = m_Prompts.find(InPrompt.Name);
    if (ExistingIt == m_Prompts.end()) {
        Logger::Warning("Prompt does not exist: " + InPrompt.Name);
        return false;
    }

    m_Prompts.erase(ExistingIt);
    return true;
}

std::optional<std::vector<PromptMessage>>
PromptManager::GetPrompt(const std::string& InName,
                         const std::optional<std::vector<PromptArgument>>& InArguments) const {
    std::lock_guard<std::mutex> Lock(m_Mutex);

    // TODO @Agent - Find Prompt, Ingest Arguments, Construct PromptMessage type
    return std::nullopt;
}

std::vector<Prompt> PromptManager::ListPrompts() const {
    std::lock_guard<std::mutex> Lock(m_Mutex);

    std::vector<Prompt> Result;
    Result.reserve(m_Prompts.size());

    for (const auto& [Name, PromptData] : m_Prompts) { Result.push_back(PromptData); }

    return Result;
}

bool PromptManager::HasPrompt(const std::string& InName) const {
    std::lock_guard<std::mutex> Lock(m_Mutex);

    return m_Prompts.find(InName) != m_Prompts.end();
}

MCP_NAMESPACE_END