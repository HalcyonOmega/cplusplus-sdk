#pragma once

#include "Macros.h"

MCP_NAMESPACE_BEGIN

// HTTP Methods
static constexpr const char* MTHD_POST = "POST";
static constexpr const char* MTHD_GET = "GET";

// MCP Methods
static constexpr const char* MTHD_INITIALIZE = "initialize";
static constexpr const char* MTHD_PING = "ping";
static constexpr const char* MTHD_NOTIFICATIONS_INITIALIZED = "notifications/initialized";
static constexpr const char* MTHD_NOTIFICATIONS_CANCELLED = "notifications/cancelled";
static constexpr const char* MTHD_NOTIFICATIONS_PROGRESS = "notifications/progress";
static constexpr const char* MTHD_NOTIFICATIONS_MESSAGE = "notifications/message";
static constexpr const char* MTHD_NOTIFICATIONS_PROMPTS_LIST_CHANGED =
    "notifications/prompts/list_changed";
static constexpr const char* MTHD_NOTIFICATIONS_RESOURCES_LIST_CHANGED =
    "notifications/resources/list_changed";
static constexpr const char* MTHD_NOTIFICATIONS_ROOTS_LIST_CHANGED =
    "notifications/roots/list_changed";
static constexpr const char* MTHD_NOTIFICATIONS_TOOLS_LIST_CHANGED =
    "notifications/tools/list_changed";
static constexpr const char* MTHD_TOOLS_LIST = "tools/list";
static constexpr const char* MTHD_TOOLS_CALL = "tools/call";
static constexpr const char* MTHD_PROMPTS_LIST = "prompts/list";
static constexpr const char* MTHD_PROMPTS_GET = "prompts/get";
static constexpr const char* MTHD_COMPLETION_COMPLETE = "completion/complete";
static constexpr const char* MTHD_LOGGING_SET_LEVEL = "logging/setLevel";
static constexpr const char* MTHD_ROOTS_LIST = "roots/list";
static constexpr const char* MTHD_SAMPLING_CREATE_MESSAGE = "sampling/createMessage";
static constexpr const char* MTHD_RESOURCES_LIST = "resources/list";
static constexpr const char* MTHD_RESOURCES_TEMPLATES_LIST = "resources/templates/list";
static constexpr const char* MTHD_NOTIFICATIONS_RESOURCES_UPDATED =
    "notifications/resources/updated";
static constexpr const char* MTHD_RESOURCES_SUBSCRIBE = "resources/subscribe";
static constexpr const char* MTHD_RESOURCES_UNSUBSCRIBE = "resources/unsubscribe";
static constexpr const char* MTHD_RESOURCES_READ = "resources/read";

MCP_NAMESPACE_END