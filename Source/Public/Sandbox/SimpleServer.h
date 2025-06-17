#pragma once

#include "Core.h"
#include "Core/Types/Logging.h"
#include "Sandbox/ServerBase.h"

MCP_NAMESPACE_BEGIN

class MCP_Server : public ServerBase {
  public:
    MCP_Server(const ServerOptions& InOptions);
    ~MCP_Server() override;

    MCPTask<PingResponse> RequestPing(const PingRequest& InRequest = {});

    MCPTask<CreateMessageResponse> RequestCreateMessage(const CreateMessageRequest& InRequest = {});

    MCPTask<ListRootsResponse> RequestListRoots(const ListRootsRequest& InRequest = {});

    MCPTask_Void NotifyLoggingMessage(const LoggingMessageNotification& InMessage);

    MCPTask_Void NotifyResourceUpdated(const ResourceUpdatedNotification& InMessage);

    MCPTask_Void NotifyResourceListChanged(const ResourceListChangedNotification& InMessage);

    MCPTask_Void NotifyToolListChanged(const ToolListChangedNotification& InMessage);

    MCPTask_Void NotifyPromptListChanged(const PromptListChangedNotification& InMessage);
};

MCP_NAMESPACE_END