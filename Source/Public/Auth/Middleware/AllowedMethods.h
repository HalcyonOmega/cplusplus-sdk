#pragma once

#include "Auth/Types/AuthErrors.h"
#include "Core.h"

// TODO: Fix External Ref: MethodNotAllowedError

MCP_NAMESPACE_BEGIN

/**
 * Middleware to handle unsupported HTTP methods with a HTTP::Status::MethodNotAllowed Method Not
 * Allowed response.
 *
 * @param InAllowedMethods Array of allowed HTTP methods for this endpoint (e.g., {MTHD_GET,
 * MTHD_POST})
 * @returns Express middleware that returns a HTTP::Status::MethodNotAllowed error if method not in
 * allowed list
 */
RequestHandler AllowedMethods(const vector<string>& InAllowedMethods);

MCP_NAMESPACE_END