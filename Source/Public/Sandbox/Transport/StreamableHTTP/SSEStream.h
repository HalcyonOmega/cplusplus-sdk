#pragma once

#include "Core.h"
#include "SSEEvent.h"

MCP_NAMESPACE_BEGIN

// SSE Stream wrapper for Poco streams
class SSEStream {
  private:
    unique_ptr<istream> m_Stream;
    bool m_IsOpen;

  public:
    SSEStream(unique_ptr<istream> InStream) : m_Stream(std::move(InStream)), m_IsOpen(true) {}

    ~SSEStream() {
        Close();
    }

    bool IsOpen() const {
        return m_IsOpen && m_Stream && m_Stream->good();
    }

    void Close() {
        m_IsOpen = false;
        m_Stream.reset();
    }

    MCPTask_Void ReadEvent() {
        SSEEvent Event;
        string Line;

        while (IsOpen() && getline(*m_Stream, Line)) {
            if (Line.empty()) {
                // Empty line signals end of event
                co_return Event;
            }

            if (Line.starts_with("id:")) {
                Event.ID = Line.substr(3);
            } else if (Line.starts_with("event:")) {
                Event.Type = Line.substr(6);
            } else if (Line.starts_with("data:")) {
                if (!Event.Data.empty()) Event.Data += "\n";
                Event.Data += Line.substr(5);
            } else if (Line.starts_with("retry:")) {
                Event.Retry = stoi(Line.substr(6));
            }
        }

        co_return Event;
    }
};

MCP_NAMESPACE_END