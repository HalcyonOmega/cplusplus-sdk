#pragma once

#include <functional>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Messages/ErrorResponseBase.h"
#include "CoreSDK/Messages/MessageBase.h"
#include "CoreSDK/Messages/NotificationBase.h"
#include "CoreSDK/Messages/RequestBase.h"
#include "CoreSDK/Messages/ResponseBase.h"

MCP_NAMESPACE_BEGIN

// Message handlers
using MessageHandler = std::function<void(const MessageBase& InMessage)>;
using RequestHandler = std::function<void(const RequestBase& InRequest)>;
using ResponseHandler = std::function<void(const ResponseBase& InResponse)>;
using NotificationHandler = std::function<void(const NotificationBase& InNotification)>;
using ErrorResponseHandler = std::function<void(const ErrorResponseBase& InError)>;

MCP_NAMESPACE_END