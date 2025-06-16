#pragma once

#include "Core.h"
#include "Transport.h"
#include "Utilities/HTTP/HTTPLayer.hpp"

MCP_NAMESPACE_BEGIN

class StreamableHTTPTransportBase : public Transport {
  public:
    // Nothing to construct – common helpers are static. Keep default ctor/dtor.
    StreamableHTTPTransportBase() = default;
    ~StreamableHTTPTransportBase() override = default;

// Shared constants -----------------------------------------------------
#pragma region ReconnectionOptions

    // Default reconnection options for Streamable HTTP connections
    struct StreamableHTTPReconnectionOptions {
        int MaxReconnectionDelay = 30000;    // 30 seconds
        int InitialReconnectionDelay = 1000; // 1 second
        double ReconnectionDelayGrowFactor = 1.5;
        int MaxRetries = 2;
    };

    static inline constexpr StreamableHTTPReconnectionOptions
        DEFAULT_STREAMABLE_HTTP_RECONNECTION_OPTIONS = {/*Initial*/ 1000, /*Max*/ 30000,
                                                        /*Grow*/ 1.5, /*Retries*/ 2};

#pragma endregion

    // Pure virtuals that concrete sub-classes MUST implement ---------------
    virtual future<void> Start() override = 0;
    virtual future<void> Close() override = 0;
    virtual future<void> Send(const MessageBase& InMessage,
                              const TransportSendOptions& InOptions = {}) override = 0;
    virtual void WriteSSEEvent(const string& InEvent, const string& InData) override = 0;
    [[deprecated("Not yet implemented – will be supported in a future version")]]
    virtual bool Resume(const string& InResumptionToken) override = 0;

  protected:
    // Helper that formats a SSE event string compliant with RFC 6455 rules.
    static string FormatSSEEvent(const string& InEvent, const string& InData,
                                 const optional<string>& InID = nullopt) {
        stringstream ss;
        if (InID) { ss << "id: " << *InID << "\n"; }
        if (!InEvent.empty()) { ss << "event: " << InEvent << "\n"; }
        // SSE data MAY contain multiple lines; split on \n and prefix each line with "data: "
        size_t Start = 0;
        while (Start <= InData.size()) {
            size_t End = InData.find('\n', Start);
            string Line = InData.substr(Start, End == string::npos ? string::npos : End - Start);
            ss << "data: " << Line << "\n";
            if (End == string::npos) break;
            Start = End + 1;
        }
        ss << "\n"; // End of event
        return ss.str();
    }

    // Simple SSE parser (best-effort, for diagnostics & client-side use)
    struct ParsedSSE {
        optional<string> ID;
        optional<string> Event;
        string Data;
    };

    static optional<ParsedSSE> ParseSSEChunk(const string& InRaw) {
        ParsedSSE Result;
        stringstream Stream(InRaw);
        string Line;
        while (getline(Stream, Line)) {
            if (Line.rfind("id:", 0) == 0) {
                Result.ID = Trim(Line.substr(3));
            } else if (Line.rfind("event:", 0) == 0) {
                Result.Event = Trim(Line.substr(6));
            } else if (Line.rfind("data:", 0) == 0) {
                if (!Result.Data.empty()) Result.Data += "\n";
                Result.Data += Trim(Line.substr(5));
            }
        }
        if (Result.Data.empty()) return nullopt; // Not a valid event
        return Result;
    }

  private:
    // Helper to trim leading whitespace – internal use
    static string Trim(string S) {
        const char* White = " \t\r";
        S.erase(0, S.find_first_not_of(White));
        S.erase(S.find_last_not_of(White) + 1);
        return S;
    }
};

MCP_NAMESPACE_END