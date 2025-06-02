// *   `MCP_Resource`: (Defines a resource item)
//         *   `MCP_ResourceTemplate` *   `MCP_TextResourceContents` *   `MCP_BlobResourceContents`
//         *   `MCP_ResourcesReadResult`
//     : (Contains array of `MCP_TextResourceContents` | `MCP_BlobResourceContents`)
//     - Returned by the `Read` method
//               .*   `ResourcesClientStub` Methods : *   `List(Cursor ?:
//               std::optional<std::string>)`
//               ->Result : `MCP_ListResult<MCP_Resource>` *   `Read(URI : std::string)`
//               ->Result
//     : `MCP_ResourcesReadResult` *   `TemplatesList(Cursor ?: std::optional<std::string>)`
//               ->Result : `MCP_ListResult<MCP_ResourceTemplate>` *   `Subscribe(
//                   URI : std::string)` ->Result
//     : `void` *   `Unsubscribe(URI : std::string)` ->Result : `void` *   `notifications
//           / resources
//           / listChanged` (Notification)
//     : (no parameters) *   `MCP_NotificationsResourcesUpdatedParams`: (`URI`)

#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

MCP_NAMESPACE_END