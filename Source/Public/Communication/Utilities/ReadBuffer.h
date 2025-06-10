#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "Communication/Message.h"
#include "Core.h"
#include "Core/Constants/MessageConstants.h"

MCP_NAMESPACE_BEGIN

/**
 * Buffers a continuous stdio stream into discrete JSON-RPC messages.
 */
class ReadBuffer {
  public:
    ReadBuffer() = default;
    ~ReadBuffer() = default;

    /**
     * Append new data to the buffer.
     */
    void Append(const std::vector<uint8_t>& chunk) {
        if (m_Buffer.empty()) {
            m_Buffer = chunk;
        } else {
            m_Buffer.insert(m_Buffer.end(), chunk.begin(), chunk.end());
        }
    }

    /**
     * Attempt to read a complete message from the buffer.
     * Returns nullopt if no complete message is available.
     */
    std::optional<MessageBase> ReadMessage() {
        if (m_Buffer.empty()) { return std::nullopt; }

        // Find message boundary (newline)
        auto it = std::find(m_Buffer.begin(), m_Buffer.end(), '\n');
        if (it == m_Buffer.end()) { return std::nullopt; }

        // Extract message and remove carriage return if present
        std::string line(m_Buffer.begin(), it);
        if (!line.empty() && line.back() == '\r') { line.pop_back(); }

        // Remove the processed message from buffer
        m_Buffer.erase(m_Buffer.begin(), it + 1);

        try {
            return DeserializeMessage(line);
        } catch (const std::exception&) {
            // Invalid message, clear the buffer to prevent getting stuck
            m_Buffer.clear();
            return std::nullopt;
        }
    }

    /**
     * Clear the buffer.
     */
    void Clear() {
        m_Buffer.clear();
    }

  private:
    std::vector<uint8_t> m_Buffer;
};

MCP_NAMESPACE_END