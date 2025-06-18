#pragma once

#include "Auth/Providers/Provider.h"
#include "Core.h"
#include "StreamableHTTPBase.h"

// Poco Net includes
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/StreamCopier.h>
#include <Poco/Thread.h>

MCP_NAMESPACE_BEGIN

// Client implementation of Streamable HTTP transport
class StreamableHTTPClient : public StreamableHTTPBase {};

MCP_NAMESPACE_END