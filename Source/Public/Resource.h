// *   `Resource`: (Defines a resource item)
//         *   `ResourceTemplate` *   `TextResourceContents` *   `BlobResourceContents`
//         *   `ResourcesReadResult`
//     : (Contains array of `TextResourceContents` | `BlobResourceContents`)
//     - Returned by the `Read` method
//               .*   `ResourcesClientStub` Methods : *   `List(Cursor ?:
//               std::optional<std::string>)`
//               ->Result : `ListResult<Resource>` *   `Read(URI : std::string)`
//               ->Result
//     : `ResourcesReadResult` *   `TemplatesList(Cursor ?: std::optional<std::string>)`
//               ->Result : `ListResult<ResourceTemplate>` *   `Subscribe(
//                   URI : std::string)` ->Result
//     : `void` *   `Unsubscribe(URI : std::string)` ->Result : `void` *   `notifications
//           / resources
//           / listChanged` (Notification)
//     : (no parameters) *   `NotificationsResourcesUpdatedParams`: (`URI`)

#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

MCP_NAMESPACE_END