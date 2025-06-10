/* Needed Classes/Structs */
struct HTTP_Client;
struct HTTP_Server;
struct HTTP_Response;
struct HTTP_Request;


/* Begin AuthClient Implementation */
struct HTTP_Response {
    int StatusCode;
    unordered_map<string, string> Headers;
    string Body;
    bool IsOK() const;
};
/* End AuthClient Implementation */

/* Begin ClientAuth_MW Implementation */
using NextFunction = function<void()>;

// HTTP Request/Response abstraction for middleware pattern
struct HTTP_Request {
    JSON Body;
    shared_ptr<OAuthClientInformationFull> Client = nullptr;
};

struct HTTP_Response {
    int StatusCode = 200;
    JSON Body;

    void SetStatus(int Status);
    void SetJSON(const JSON& Data);
};

// Modern C++20 middleware function type
using RequestHandler = function<future<void>(HTTP_Request&, HTTPResponse&, NextFunction)>;
/* End ClientAuth_MW Implementation */

/* Begin SSE Implementation */

// Forward declarations for external dependencies
struct HTTP_Request {
    // TODO: Implement HTTP request structure
    map<string, string> Headers;
    optional<AuthInfo> Auth; // Optional auth info
};

// TODO: Consider making a class since it has logic
struct HTTP_Response {
    // TODO: Implement HTTP response structure
    bool IsEnded = false;

    void WriteHead(int InStatusCode, const optional<map<string, string>>& InHeaders = nullopt);

    void Write(const string& InData);

    void End(const optional<string>& InData = nullopt);

    void On(const string& InEvent, optional<function<void()>> InCallback);
};
/* End SSE Implementation */

/* Begin SSE Client Implementation */


// HTTP Response struct (moved up for forward declaration)
// TODO: Consider making a class since it has logic
struct HTTP_Response {
    int status;
    string body;
    map<string, string> headers;
    bool ok;

    future<string> text() const;
};
/* End SSE Client Implementation */

/* Begin Streamable HTTP Implementation */
struct HTTP_Request {
    string method;
    unordered_map<string, string> headers;
    string body;
    optional<AuthInfo> auth;
};

struct HTTP_Response {
    function<void(int, const unordered_map<string, string>&)> writeHead;
    function<void(const string&)> end;
    function<bool(const string&)> write;
    function<void()> flushHeaders;
    function<void(function<void()>)> on;
    bool closed = false;
};
/* End Streamable HTTP Implementation */