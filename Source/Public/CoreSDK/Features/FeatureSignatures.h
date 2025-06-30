#pragma once

#include <functional>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Features/PromptBase.h"
#include "CoreSDK/Features/ResourceBase.h"
#include "CoreSDK/Features/RootBase.h"
#include "CoreSDK/Features/ToolBase.h"
#include "CoreSDK/Messages/MCPMessages.h"
#include "Utilities/Async/MCPTask.h"

MCP_NAMESPACE_BEGIN

// Message handlers
using ToolHandler = std::function<MCPTask<CallToolResponse::Result>(const Tool& InTool)>;
using PromptHandler = std::function<MCPTask<GetPromptResponse::Result>(const Prompt& InPrompt)>;
using ResourceHandler =
    std::function<MCPTask<ReadResourceResponse::Result>(const Resource& InResource)>;
using ResourceTemplateHandler = std::function<MCPTask<ReadResourceResponse::Result>(
    const ResourceTemplate& InResourceTemplate)>;
using RootHandler = std::function<MCPTask<ReadResourceResponse::Result>(const Root& InRoot)>;

MCP_NAMESPACE_END