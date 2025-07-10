#pragma once

#include <any>
#include <functional>
#include <future>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Messages/MCPMessages.h"
#include "SamplingBase.h"
#include "Utilities/Async/Task.h"

MCP_NAMESPACE_BEGIN

/**
 * Exception thrown when sampling operations fail.
 */
class SamplingError final : public std::runtime_error
{
public:
	explicit SamplingError(const std::string& InMessage) : std::runtime_error(InMessage) {}
};

/**
 * Manages MCP sampling operations.
 * Provides functionality for managing sampling requests, model preferences, and LLM interactions.
 */
class SamplingManager
{
public:
	using SamplingFunction = std::function<Task<SamplingResult>(const std::vector<SamplingMessage>&,
		const std::optional<ModelPreferences>&)>;

	/**
	 * Constructor
	 * @param InDefaultModelPreferences Default model preferences to use for sampling
	 */
	explicit SamplingManager(const std::optional<ModelPreferences>& InDefaultModelPreferences = std::nullopt);

	/**
	 * Set the sampling function to handle sampling requests.
	 * @param InSamplingFunction The function to execute sampling requests
	 */
	void SetSamplingFunction(SamplingFunction InSamplingFunction);

	/**
	 * Set default model preferences for all sampling requests.
	 * @param InModelPreferences The default model preferences
	 */
	void SetDefaultModelPreferences(const ModelPreferences& InModelPreferences);

	/**
	 * Get the current default model preferences.
	 * @return The default model preferences if set
	 */
	std::optional<ModelPreferences> GetDefaultModelPreferences() const;

	/**
	 * Clear the default model preferences.
	 */
	void ClearDefaultModelPreferences();

	/**
	 * Check if a sampling function is configured.
	 * @return True if sampling function is set, false otherwise
	 */
	bool HasSamplingFunction() const;

	CreateMessageResponse::Result CreateMessage(const CreateMessageRequest::Params& InParams) const;

private:
	std::optional<ModelPreferences> m_DefaultModelPreferences;
	std::optional<SamplingFunction> m_SamplingFunction;
	mutable std::mutex m_SamplingMutex;

	/**
	 * Get the effective model preferences for a request.
	 * @param InModelPreferences Optional request-specific preferences
	 * @return The effective model preferences to use
	 */
	std::optional<ModelPreferences> GetEffectiveModelPreferences(
		const std::optional<ModelPreferences>& InModelPreferences) const;
};

MCP_NAMESPACE_END
