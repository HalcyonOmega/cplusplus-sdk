#pragma once

#include <functional>

#include "CoreSDK/Common/Macros.h"
#include "Utilities/Async/MCPTask.h"

struct MessageBase;
struct RequestBase;
struct ResponseBase;
struct NotificationBase;
struct ErrorResponseBase;

MCP_NAMESPACE_BEGIN

// Message handlers
using MessageHandler = std::function<MCPTask_Void(const MessageBase& InMessage)>;
using RequestHandler = std::function<MCPTask<const ResponseBase&>(const RequestBase& InRequest)>;
using ResponseHandler = std::function<MCPTask_Void(const ResponseBase& InResponse)>;
using NotificationHandler = std::function<MCPTask_Void(const NotificationBase& InNotification)>;
using ErrorResponseHandler = std::function<MCPTask_Void(const ErrorResponseBase& InError)>;

MCP_NAMESPACE_END