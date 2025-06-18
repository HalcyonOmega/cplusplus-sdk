#pragma once

#include <atomic>

#include "Core.h"

MCP_NAMESPACE_BEGIN

// TODO: @HalcyonOmega Cleanup this file
struct AbortSignal; // TODO: @HalcyonOmega Implement This

//------------------------------------------------------------------------------
// AbortController
// Mirrors the semantics of the WHATWG / Node.js AbortController. It owns an
// atomic flag that can be queried by long-running operations to detect
// cancellation requests.
//------------------------------------------------------------------------------
class AbortController {
  private:
    atomic<bool> m_Aborted{false};

  public:
    AbortController() = default;
    ~AbortController() = default;

    // Signals cancellation. Thread-safe.
    void Abort() {
        m_Aborted.store(true, memory_order_release);
    }

    // Returns true once Abort() has been invoked.
    [[nodiscard]] bool IsAborted() const {
        return m_Aborted.load(memory_order_acquire);
    }
};

MCP_NAMESPACE_END