#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Core.h"

MCP_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// ChildProcess (stub)
// Cross-platform placeholder representing a spawned child process whose
// stdin / stdout / stderr are attached to pipes. The implementation will be
// replaced with platform-specific code. For now it only stores string buffers
// and provides a minimal Write API so that transport code compiles.
//------------------------------------------------------------------------------
class ChildProcess {
  public:
    ChildProcess() = default;
    ~ChildProcess() = default;

    // Writes data to the child process's stdin. Currently a no-op placeholder.
    void Write(const std::string& /*Data*/) {
        // TODO: @HalcyonOmega Implement proper pipe IO.
    }

    // Returns true if the underlying process object is valid and still running.
    [[nodiscard]] bool IsValid() const {
        return false;
    }
};

using ChildProcessPtr = std::unique_ptr<ChildProcess>;

MCP_NAMESPACE_END