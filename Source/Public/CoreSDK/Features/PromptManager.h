#pragma once

#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Messages/MCPMessages.h"
#include "PromptBase.h"

MCP_NAMESPACE_BEGIN

/**
 * Manages MCP prompts.
 * Provides functionality for registering, retrieving, and listing prompts.
 */
class PromptManager
{
public:
	using PromptFunction = std::function<std::vector<PromptMessage>(std::optional<std::vector<PromptArgument>>)>;

	/**
	 * Constructor
	 * @param InWarnOnDuplicatePrompts Whether to warn when duplicate prompts are added
	 */
	explicit PromptManager(bool InWarnOnDuplicatePrompts = true);

	/**
	 * Add a prompt to the manager.
	 * @param InPrompt The prompt to add
	 * @return True if the prompt was added, false if a prompt with the same name already exists
	 */
	bool AddPrompt(const Prompt& InPrompt, const PromptFunction& InFunction);

	/**
	 * Remove a prompt from the manager.
	 * @param InPrompt The prompt to remove
	 * @return True if the prompt was removed, false if the prompt does not exist
	 */
	bool RemovePrompt(const Prompt& InPrompt);

	/**
	 * Get prompt by name.
	 * @param InName The name of the prompt to retrieve
	 * @param InArguments The arguments to use for templating the prompt
	 * @return The prompt if found, nullopt if the prompt does not exist or the arguments are
	 * invalid
	 */
	GetPromptResponse::Result GetPrompt(const GetPromptRequest::Params& InRequest) const;

	/**
	 * List all registered prompts.
	 * @return Vector containing all registered prompts
	 */
	ListPromptsResponse::Result ListPrompts(const PaginatedRequestParams& InRequest) const;

	/**
	 * Check if a prompt with the given name exists.
	 * @param InName The name to check
	 * @return An optional reference to the prompt if it exists
	 */
	std::optional<Prompt> FindPrompt(const std::string& InName) const;

private:
	std::map<Prompt, PromptFunction> m_Prompts;
	bool m_WarnOnDuplicatePrompts;
	mutable std::mutex m_Mutex;
};

MCP_NAMESPACE_END