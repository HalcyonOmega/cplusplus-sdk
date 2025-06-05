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
    optional<vector<uint8_t>> _buffer;

  public:
    void append(const vector<uint8_t>& chunk) {
        if (_buffer.has_value()) {
            _buffer->insert(_buffer->end(), chunk.begin(), chunk.end());
        } else {
            _buffer = chunk;
        }
    }

    // TODO: Fix External Ref: Should return JSON_RPC_Message type when available
    optional<JSON> readMessage() {
        if (!_buffer.has_value()) {
            return nullopt;
        }

        // Find newline character
        auto it = find(_buffer->begin(), _buffer->end(), '\n');
        if (it == _buffer->end()) {
            return nullopt;
        }

        // Extract line up to newline (equivalent to toString("utf8", 0, index))
        ptrdiff_t index = distance(_buffer->begin(), it);
        string line(reinterpret_cast<const char*>(_buffer->data()), index);

        // Remove carriage return if present (equivalent to .replace(/\r$/, ''))
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // Remove processed data from buffer (equivalent to subarray(index + 1))
        // Use erase for efficiency instead of creating new vector
        _buffer->erase(_buffer->begin(), _buffer->begin() + index + 1);

        // Clear buffer if empty to match TypeScript behavior
        if (_buffer->empty()) {
            _buffer = nullopt;
        }

        return deserializeMessage(line);
    }

    void clear() {
        _buffer = nullopt;
    }
};

// TODO: Fix External Ref: Should return JSON_RPC_Message type when available
JSON deserializeMessage(const string& line) {
    // TODO: Fix External Ref: JSON_RPC_MessageSchema validation equivalent
    // Original: JSON_RPC_MessageSchema.parse(JSON.parse(line))
    return JSON::parse(line);
}

// TODO: Fix External Ref: Should accept JSON_RPC_Message type when available
string serializeMessage(const JSON& message) {
    return message.dump() + "\n";
}

MCP_NAMESPACE_END
