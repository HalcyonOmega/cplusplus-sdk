#pragma once

#include "../ITransport.h"
#include "Communication/Transport/EventStore.h"
#include "Core.h"
#include "HTTPProxy.h"
#include "SSEEvent.h"
#include "SSEStream.h"

// Poco Net includes
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/StreamCopier.h>
#include <Poco/URI.h>

MCP_NAMESPACE_BEGIN

// Base class for Streamable HTTP transport implementations
class StreamableHTTPBase : public ITransport {};

MCP_NAMESPACE_END