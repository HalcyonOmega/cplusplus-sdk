#include "CoreSDK/Features/SamplingManager.h"

#include <future>

#include "CoreSDK/Common/Logging.h"
#include "CoreSDK/Common/RuntimeError.h"

MCP_NAMESPACE_BEGIN

SamplingManager::SamplingManager(const std::optional<ModelPreferences> InDefaultModelPreferences)
	: m_DefaultModelPreferences(InDefaultModelPreferences)
{}

void SamplingManager::SetSamplingFunction(SamplingFunction InSamplingFunction)
{
	std::lock_guard Lock(m_SamplingMutex);

	m_SamplingFunction = std::move(InSamplingFunction);
	HandleRuntimeError("Sampling function has been configured");
}

void SamplingManager::SetDefaultModelPreferences(const ModelPreferences& InModelPreferences)
{
	std::lock_guard Lock(m_SamplingMutex);

	m_DefaultModelPreferences = InModelPreferences;
	HandleRuntimeError("Default model preferences updated");
}

std::optional<ModelPreferences> SamplingManager::GetDefaultModelPreferences() const
{
	std::lock_guard Lock(m_SamplingMutex);

	return m_DefaultModelPreferences;
}

void SamplingManager::ClearDefaultModelPreferences()
{
	std::lock_guard Lock(m_SamplingMutex);

	m_DefaultModelPreferences.reset();
	HandleRuntimeError("Default model preferences cleared");
}

bool SamplingManager::HasSamplingFunction() const
{
	std::lock_guard Lock(m_SamplingMutex);

	return m_SamplingFunction.has_value();
}

CreateMessageResponse::Result SamplingManager::CreateMessage(const CreateMessageRequest::Params* InParams) const
{
	(void)InParams;

	return {};
}

std::optional<ModelPreferences> SamplingManager::GetEffectiveModelPreferences(
	const std::optional<ModelPreferences>& InModelPreferences) const
{
	if (m_DefaultModelPreferences)
	{
		return m_DefaultModelPreferences;
	}
	if (InModelPreferences)
	{
		return InModelPreferences;
	}
	return std::nullopt;
}

MCP_NAMESPACE_END
