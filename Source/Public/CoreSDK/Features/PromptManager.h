#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "CoreSDK/Common/Macros.h"
#include "PromptBase.h"

MCP_NAMESPACE_BEGIN

/**
 * Manages FastMCP prompts.
 * Provides functionality for registering, retrieving, and listing prompts.
 */
class PromptManager {
  public:
    /**
     * Constructor
     * @param InWarnOnDuplicatePrompts Whether to warn when duplicate prompts are added
     */
    explicit PromptManager(bool InWarnOnDuplicatePrompts = true);

    /**
     * Add a prompt to the manager.
     * @param InPrompt The prompt to add
     * @return The added prompt. If a prompt with the same name already exists, returns the existing
     * prompt.
     */
    Prompt AddPrompt(const Prompt& InPrompt);

    /**
     * Get prompt by name.
     * @param InName The name of the prompt to retrieve
     * @return The prompt if found, nullopt otherwise
     */
    std::optional<Prompt> GetPrompt(const std::string& InName) const;

    /**
     * List all registered prompts.
     * @return Vector containing all registered prompts
     */
    std::vector<Prompt> ListPrompts() const;

    /**
     * Check if a prompt with the given name exists.
     * @param InName The name to check
     * @return True if the prompt exists, false otherwise
     */
    bool HasPrompt(const std::string& InName) const;

  private:
    std::unordered_map<std::string, Prompt> m_Prompts;
    bool m_WarnOnDuplicatePrompts;
};

MCP_NAMESPACE_END