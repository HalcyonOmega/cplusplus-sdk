#pragma once

#include "Core.h"

// TODO: @HalcyonOmega - Fix Includes

MCP_NAMESPACE_BEGIN

/* Client messages */
using ClientNotification = variant<CancelledNotification, ProgressNotification,
                                   InitializedNotification, RootsListChangedNotification>;

/* Server messages */
using ServerNotification =
    variant<CancelledNotification, ProgressNotification, LoggingMessageNotification,
            ResourceUpdatedNotification, ResourceListChangedNotification,
            ToolListChangedNotification, PromptListChangedNotification>;

MCP_NAMESPACE_END
