#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// Minimal internal HTTP client using libcurl
class HTTPClient {
  public:
    HTTPClient();
    ~HTTPClient();

    // HTTP POST: returns response body as string, throws on error
    std::string Post(const std::string& Url, const std::string& Body,
                     const std::vector<std::string>& HTTPHeaders);

    // HTTP GET for SSE: calls on_event for each event data
    void GetSSE(const std::string& Url, const std::vector<std::string>& HTTPHeaders,
                std::function<void(const std::string&)> OnEvent);

  private:
    static size_t WriteCallback(void* Contents, size_t Size, size_t Nmemb, void* UserPtr);
    struct SSEContext {
        std::string* Buffer;
        std::function<void(const std::string&)>* OnEvent;
    };
    static size_t SSEWriteCallback(void* Contents, size_t Size, size_t Nmemb, void* UserPtr);
};

MCP_NAMESPACE_END