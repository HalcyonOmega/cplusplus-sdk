#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "Message.h"

MCP_NAMESPACE_BEGIN

/**
 * Deserialize a JSON-RPC message from a string.
 */
inline MessageBase DeserializeMessage(const std::string& line) {
    MessageBase msg;
    msg.Message = JSON::parse(line);
    // TODO: Add schema validation if needed
    return msg;
}

/**
 * Serialize a message to JSON string with newline.
 */
inline std::string SerializeMessage(const MessageBase& message) {
    return message.Message.dump() + "\n";
}

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
        if (_buffer.empty()) {
            _buffer = chunk;
        } else {
            _buffer.insert(_buffer.end(), chunk.begin(), chunk.end());
        }
    }

    /**
     * Attempt to read a complete message from the buffer.
     * Returns nullopt if no complete message is available.
     */
    std::optional<MessageBase> ReadMessage() {
        if (_buffer.empty()) { return std::nullopt; }

        // Find message boundary (newline)
        auto it = std::find(_buffer.begin(), _buffer.end(), '\n');
        if (it == _buffer.end()) { return std::nullopt; }

        // Extract message and remove carriage return if present
        std::string line(_buffer.begin(), it);
        if (!line.empty() && line.back() == '\r') { line.pop_back(); }

        // Remove the processed message from buffer
        _buffer.erase(_buffer.begin(), it + 1);

        try {
            return DeserializeMessage(line);
        } catch (const std::exception&) {
            // Invalid message, clear the buffer to prevent getting stuck
            _buffer.clear();
            return std::nullopt;
        }
    }

    /**
     * Clear the buffer.
     */
    void Clear() {
        _buffer.clear();
    }

  private:
    std::vector<uint8_t> _buffer;
};

MCP_NAMESPACE_END