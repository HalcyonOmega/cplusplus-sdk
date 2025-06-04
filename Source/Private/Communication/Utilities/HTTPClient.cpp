#include "Communication/Utilities/HTTPClient.h"

#include <curl/curl.h>

#include "Core.h"
#include "Core/Constants/HTTPConstants.h"

MCP_NAMESPACE_BEGIN

HTTPClient::HTTPClient() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}
HTTPClient::~HTTPClient() {
    curl_global_cleanup();
}

// HTTP POST: returns response body as string, throws on error
std::string HTTPClient::Post(const std::string& Url, const std::string& Body,
                             const std::vector<std::string>& HTTPHeaders) {
    CURL* CURLHandle = curl_easy_init();
    if (!CURLHandle) throw std::runtime_error(HTTP_ERR_INIT_CURL);
    struct curl_slist* HeaderList = nullptr;
    for (const auto& Header : HTTPHeaders)
        HeaderList = curl_slist_append(HeaderList, Header.c_str());
    std::string HTTPResponse;
    curl_easy_setopt(CURLHandle, CURLOPT_URL, Url.c_str());
    curl_easy_setopt(CURLHandle, CURLOPT_POST, 1L);
    curl_easy_setopt(CURLHandle, CURLOPT_POSTFIELDS, Body.c_str());
    curl_easy_setopt(CURLHandle, CURLOPT_POSTFIELDSIZE, Body.size());
    curl_easy_setopt(CURLHandle, CURLOPT_HTTPHEADER, HeaderList);
    curl_easy_setopt(CURLHandle, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(CURLHandle, CURLOPT_WRITEDATA, &HTTPResponse);
    CURLcode CURLResult = curl_easy_perform(CURLHandle);
    curl_slist_free_all(HeaderList);
    curl_easy_cleanup(CURLHandle);
    if (CURLResult != CURLE_OK) throw std::runtime_error(curl_easy_strerror(CURLResult));
    return HTTPResponse;
}

// HTTP GET for SSE: calls on_event for each event data
void HTTPClient::GetSSE(const std::string& Url, const std::vector<std::string>& HTTPHeaders,
                        std::function<void(const std::string&)> OnEvent) {
    CURL* CURLHandle = curl_easy_init();
    if (!CURLHandle) throw std::runtime_error(HTTP_ERR_INIT_CURL);
    struct curl_slist* HeaderList = nullptr;
    for (const auto& Header : HTTPHeaders)
        HeaderList = curl_slist_append(HeaderList, Header.c_str());
    std::string Buffer;
    curl_easy_setopt(CURLHandle, CURLOPT_URL, Url.c_str());
    curl_easy_setopt(CURLHandle, CURLOPT_HTTPHEADER, HeaderList);
    curl_easy_setopt(CURLHandle, CURLOPT_WRITEFUNCTION, SSEWriteCallback);
    struct SSEContext Context{&Buffer, &OnEvent};
    curl_easy_setopt(CURLHandle, CURLOPT_WRITEDATA, &Context);
    CURLcode CURLResult = curl_easy_perform(CURLHandle);
    curl_slist_free_all(HeaderList);
    curl_easy_cleanup(CURLHandle);
    if (CURLResult != CURLE_OK) throw std::runtime_error(curl_easy_strerror(CURLResult));
}

size_t HTTPClient::WriteCallback(void* Contents, size_t Size, size_t Nmemb, void* UserPtr) {
    size_t Total = Size * Nmemb;
    std::string* Str = static_cast<std::string*>(UserPtr);
    Str->append(static_cast<char*>(Contents), Total);
    return Total;
}
size_t HTTPClient::SSEWriteCallback(void* Contents, size_t Size, size_t Nmemb, void* UserPtr) {
    size_t Total = Size * Nmemb;
    SSEContext* Context = static_cast<SSEContext*>(UserPtr);
    Context->Buffer->append(static_cast<char*>(Contents), Total);
    // Parse complete SSE events (delimited by double newlines)
    size_t Pos;
    while ((Pos = Context->Buffer->find(HTTP_SSE_EVENT_DELIMITER)) != std::string::npos) {
        std::string Event = Context->Buffer->substr(0, Pos);
        Context->Buffer->erase(0, Pos + std::strlen(HTTP_SSE_EVENT_DELIMITER));
        // Only process lines starting with HTTP_SSE_DATA_PREFIX
        std::istringstream Iss(Event);
        std::string Line;
        std::string Data;
        while (std::getline(Iss, Line)) {
            if (Line.rfind(HTTP_SSE_DATA_PREFIX, 0) == 0) {
                Data += Line.substr(HTTP_SSE_DATA_PREFIX_LEN);
            }
        }
        if (!Data.empty()) (*Context->OnEvent)(Data);
    }
    return Total;
}

MCP_NAMESPACE_END
