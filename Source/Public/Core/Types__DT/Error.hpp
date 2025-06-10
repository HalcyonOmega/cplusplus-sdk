#pragma once

#include <cstdint>

#include "Core.h"
#include "Core/Constants/ErrorConstants.h"

MCP_NAMESPACE_BEGIN

class MCP_Error {
  private:
    Errors m_Code;
    string m_Message;
    optional<JSON> m_Data;
    string m_FullMessage;

  public:
    Error(Errors Code, const string& Message, optional<JSON> Data = nullopt)
        : m_Code(Code), m_Message(Message), m_Data(Data) {
        m_FullMessage = "MCP error " + std::to_string(Code) + ": " + Message;
    }

    const char* what() const noexcept {
        return m_FullMessage.c_str();
    }

    Errors getCode() const {
        return m_Code;
    }
    const string& getMessage() const {
        return m_Message;
    }
    const optional<JSON>& getData() const {
        return m_Data;
    }
};

MCP_NAMESPACE_END