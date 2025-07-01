# MCP Method Map

## Client

### Requests
- Initialize
- ListTools
- CallTool
- ListPrompts
- GetPrompt
- ListResources
- ReadResource
- Subscribe
- Unsubscribe
- ListRoots
- SetLoggingLevel

### Notifications
- (?) ReportProgress

### Responses
- Complete
- CreateMessage

### Errors

## Server

### Requests
- Complete
- CreateMessage
- Sampling

### Notifications
- Initialized
- ToolListChanged
- PromptListChanged
- ResourceListChanged
- ResourceUpdated
- RootsListChanged
- LogMessage
- ReportProgress
- CancelRequest

### Responses

### Errors


OnError_
OnNotified_
OnRequest_
OnResponse_

Respond_
Notify_
Request_
ErrorRespond_