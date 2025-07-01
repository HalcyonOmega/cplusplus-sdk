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
#include "SamplingBase.h"

MCP_NAMESPACE_BEGIN

// Forward declarations
class MCPContext;

/**
 * Exception thrown when sampling operations fail.
 */
class SamplingError : public std::runtime_error {
  public:
    explicit SamplingError(const std::string& InMessage) : std::runtime_error(InMessage) {}
};

/**
 * Manages FastMCP sampling operations.
 * Provides functionality for managing sampling requests, model preferences, and LLM interactions.
 */
class SamplingManager {
  public:
    using SamplingFunction = std::function<std::future<SamplingResult>(
        const std::vector<SamplingMessage>&, const std::optional<ModelPreferences>&, MCPContext*)>;

    /**
     * Constructor
     * @param InDefaultModelPreferences Default model preferences to use for sampling
     */
    explicit SamplingManager(
        const std::optional<ModelPreferences>& InDefaultModelPreferences = std::nullopt);

    /**
     * Set the sampling function to handle sampling requests.
     * @param InSamplingFunction The function to execute sampling requests
     */
    void SetSamplingFunction(SamplingFunction InSamplingFunction);

    /**
     * Request LLM sampling with messages and optional model preferences.
     * @param InMessages The messages to send for sampling
     * @param InModelPreferences Optional model preferences for this request
     * @param InContext Optional context for the sampling operation
     * @return Future containing the sampling result
     */
    std::future<SamplingResult>
    RequestSampling(const std::vector<SamplingMessage>& InMessages,
                    const std::optional<ModelPreferences>& InModelPreferences = std::nullopt,
                    MCPContext* InContext = nullptr);

    /**
     * Request LLM sampling synchronously.
     * @param InMessages The messages to send for sampling
     * @param InModelPreferences Optional model preferences for this request
     * @param InContext Optional context for the sampling operation
     * @return The sampling result
     */
    SamplingResult
    RequestSamplingSync(const std::vector<SamplingMessage>& InMessages,
                        const std::optional<ModelPreferences>& InModelPreferences = std::nullopt,
                        MCPContext* InContext = nullptr);

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

    /**
     * Create a sampling message with the given role and content.
     * @param InRole The role for the message
     * @param InContent The content for the message
     * @return The created sampling message
     */
    static SamplingMessage
    CreateMessage(const MCP::Role& InRole,
                  const std::variant<TextContent, ImageContent, AudioContent>& InContent);

    /**
     * Create model preferences with the given parameters.
     * @param InHints Optional model hints
     * @param InCostPriority Optional cost priority (0-1)
     * @param InSpeedPriority Optional speed priority (0-1)
     * @param InIntelligencePriority Optional intelligence priority (0-1)
     * @return The created model preferences
     */
    static ModelPreferences
    CreateModelPreferences(const std::optional<std::vector<ModelHint>>& InHints = std::nullopt,
                           const std::optional<double>& InCostPriority = std::nullopt,
                           const std::optional<double>& InSpeedPriority = std::nullopt,
                           const std::optional<double>& InIntelligencePriority = std::nullopt);

  private:
    std::optional<ModelPreferences> m_DefaultModelPreferences;
    std::optional<SamplingFunction> m_SamplingFunction;
    mutable std::mutex m_SamplingMutex;

    /**
     * Get the effective model preferences for a request.
     * @param InModelPreferences Optional request-specific preferences
     * @return The effective model preferences to use
     */
    std::optional<ModelPreferences>
    GetEffectiveModelPreferences(const std::optional<ModelPreferences>& InModelPreferences) const;
};

MCP_NAMESPACE_END
