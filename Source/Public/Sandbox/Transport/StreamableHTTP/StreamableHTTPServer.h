#pragma once

#include "Core.h"
#include "StreamableHTTPBase.h"

// Poco Net includes
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Thread.h>

MCP_NAMESPACE_BEGIN

// Server implementation of Streamable HTTP transport
class StreamableHTTPServer : public StreamableHTTPBase {};

MCP_NAMESPACE_END