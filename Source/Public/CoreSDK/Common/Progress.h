#pragma once

#include <string>
#include <variant>

#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"
#include "Utilities/Async/MCPTask.h"

// Forward declarations
class MCPProtocol;

MCP_NAMESPACE_BEGIN

// A progress token, used to associate progress notifications with the original request.
// TODO: @HalcyonOmega - Relook handling - schema does not have any subfields
struct ProgressToken {
    std::variant<std::string, int64_t> Token;

    JKEY(TOKENKEY, Token, "token")

    DEFINE_TYPE_JSON(ProgressToken, TOKENKEY)
};

// Progress tracking class for long-running operations
class ProgressTracker {
  public:
    ProgressTracker(const std::string& InRequestID, std::shared_ptr<MCPProtocol> InProtocol);

    MCPTask_Void UpdateProgress(double InProgress, std::optional<int64_t> InTotal = {});
    MCPTask_Void CompleteProgress();

  private:
    std::string m_RequestID;
    std::shared_ptr<MCPProtocol> m_Protocol{nullptr};
    bool m_IsComplete{false};
};

MCP_NAMESPACE_END
