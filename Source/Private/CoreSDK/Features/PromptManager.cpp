#include "CoreSDK/Features/PromptManager.h"

#include <algorithm>

#include "CoreSDK/Common/Logging.h"

MCP_NAMESPACE_BEGIN

PromptManager::PromptManager(const bool InWarnOnDuplicatePrompts) : m_WarnOnDuplicatePrompts(InWarnOnDuplicatePrompts)
{}

bool PromptManager::AddPrompt(const Prompt& InPrompt, const PromptFunction& InFunction)
{
	// Log the addition attempt
	Logger::Debug("Adding prompt: " + InPrompt.Name);
	std::lock_guard Lock(m_Mutex);

	if (const auto ExistingIt = m_Prompts.find(InPrompt); ExistingIt != m_Prompts.end())
	{
		if (m_WarnOnDuplicatePrompts)
		{
			Logger::Warning("Prompt already exists: " + InPrompt.Name);
		}
		return false;
	}

	m_Prompts[InPrompt] = InFunction;
	return true;
}

bool PromptManager::RemovePrompt(const Prompt& InPrompt)
{
	std::lock_guard Lock(m_Mutex);

	const auto ExistingIt = m_Prompts.find(InPrompt);
	if (ExistingIt == m_Prompts.end())
	{
		Logger::Warning("Prompt does not exist: " + InPrompt.Name);
		return false;
	}

	m_Prompts.erase(ExistingIt);
	return true;
}

GetPromptResponse::Result PromptManager::GetPrompt(const GetPromptRequest::Params* InRequest) const
{
	std::lock_guard Lock(m_Mutex);

	const std::optional<Prompt> FoundPrompt = FindPrompt(InRequest->Name);
	if (!FoundPrompt)
	{
		Logger::Warning("Prompt does not exist: " + InRequest->Name);
		return {};
	}

	return GetPromptResponse::Result(m_Prompts.find(FoundPrompt.value())->second(InRequest->Arguments.value()),
		FoundPrompt->Description);
}

ListPromptsResponse::Result PromptManager::ListPrompts(const PaginatedRequestParams* InRequest) const
{
	std::lock_guard Lock(m_Mutex);

	std::vector<Prompt> Prompts;
	Prompts.reserve(m_Prompts.size());

	for (const auto& Prompt : m_Prompts | std::views::keys)
	{
		Prompts.emplace_back(Prompt);
	}

	ListPromptsResponse::Result Result;
	Result.Prompts = Prompts;
	// TODO: @HalcyonOmega - Add Cursor support
	Result.NextCursor = InRequest->Cursor;

	return Result;
}

std::optional<Prompt> PromptManager::FindPrompt(const std::string& InName) const
{
	std::lock_guard Lock(m_Mutex);

	if (const auto Iterator
		= std::ranges::find_if(m_Prompts, [&](const auto& Pair) { return Pair.first.Name == InName; });
		Iterator != m_Prompts.end())
	{
		return Iterator->first;
	}

	return std::nullopt;
}

MCP_NAMESPACE_END