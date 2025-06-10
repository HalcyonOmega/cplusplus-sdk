#pragma once

#include "../Core/Common.hpp"

MCP_NAMESPACE_BEGIN

// TODO: Fix External Ref: JSON_RPC_Message type definition
// TODO: Fix External Ref: JSON_RPC_MessageSchema validation (Zod equivalent)

/**
 * Buffers a continuous stdio stream into discrete JSON-RPC messages.
 */
class ReadBuffer {
  private:
    optional<vector<uint8_t>> m_Buffer;

  public:
    void Append(const vector<uint8_t>& InChunk) {
        if (m_Buffer.has_value()) {
            m_Buffer->insert(m_Buffer->end(), InChunk.begin(), InChunk.end());
        } else {
            m_Buffer = InChunk;
        }
    }

    optional<JSON_RPC_Message> ReadMessage() {
        if (!m_Buffer.has_value()) {
            return nullopt;
        }

        // Find newline character
        auto it = find(m_Buffer->begin(), m_Buffer->end(), '\n');
        if (it == m_Buffer->end()) {
            return nullopt;
        }

        // Extract line up to newline (equivalent to toString("utf8", 0, index))
        ptrdiff_t Index = distance(m_Buffer->begin(), it);
        string Line(reinterpret_cast<const char*>(m_Buffer->data()), Index);

        // Remove carriage return if present (equivalent to .replace(/\r$/, ''))
        if (!Line.empty() && Line.back() == '\r') {
            Line.pop_back();
        }

        // Remove processed data from buffer (equivalent to subarray(index + 1))
        // Use erase for efficiency instead of creating new vector
        m_Buffer->erase(m_Buffer->begin(), m_Buffer->begin() + index + 1);

        // Clear buffer if empty to match TypeScript behavior
        if (m_Buffer->empty()) {
            m_Buffer = nullopt;
        }

        return DeserializeMessage(Line);
    }

    void Clear() {
        m_Buffer = nullopt;
    }
};

JSON_RPC_Message DeserializeMessage(const string& InLine) {
    // TODO: Fix External Ref: JSON_RPC_MessageSchema validation equivalent
    // Original: JSON_RPC_MessageSchema.parse(JSON.parse(line))
    return JSON::parse(InLine);
}

string SerializeMessage(const JSON_RPC_Message& InMessage) {
    return InMessage.dump() + "\n";
}

MCP_NAMESPACE_END
