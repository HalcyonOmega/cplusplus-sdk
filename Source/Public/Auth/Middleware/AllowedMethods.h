#pragma once

#include "Auth/Types/AuthErrors.h"
#include "Core.h"

// TODO: Fix External Ref: MethodNotAllowedError

MCP_NAMESPACE_BEGIN

/**
 * Middleware to handle unsupported HTTP methods with a 405 Method Not Allowed response.
 *
 * @param InAllowedMethods Array of allowed HTTP methods for this endpoint (e.g., {"GET", "POST"})
 * @returns Express middleware that returns a 405 error if method not in allowed list
 */
RequestHandler AllowedMethods(const vector<string>& InAllowedMethods);

MCP_NAMESPACE_END