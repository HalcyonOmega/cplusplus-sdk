# Phase 4: Core Features

## 1. Capabilities
- [ ] Implement `ClientCapabilities` and `ServerCapabilities` objects
    - [ ] Define capability objects in schema and C++ types
    - [ ] Support progressive feature negotiation during initialization
    - [ ] Implement sub-capabilities (e.g., `listChanged`, `subscribe`) for prompts, resources, tools
    - [ ] Exchange capabilities in `initialize`/`initialized` messages
    - [ ] Add unit tests for capability negotiation and conditional logic
    - [ ] Document all supported capabilities and negotiation flow

## 2. Roots
- [ ] Implement root directory/file management
    - [ ] Define `roots` capability and structure in schema and C++
    - [ ] Support `roots/list` request and response (`Root[]`)
    - [ ] Implement notifications: `notifications/roots/list_changed`
    - [ ] Respect root boundaries for resource operations
    - [ ] Document root management and update workflow
    - [ ] Add tests for root listing, updates, and notifications

## 3. Prompts
- [ ] Implement prompt listing, templating, and argument handling
    - [ ] Define `Prompt`, `PromptArgument`, and `PromptMessage` types
    - [ ] Support `prompts/list` and `prompts/get` requests
    - [ ] Implement prompt templates and argument validation
    - [ ] Support notifications: `notifications/prompts/list_changed`
    - [ ] Add autocompletion for prompt arguments
    - [ ] Document prompt structure, usage, and update flow
    - [ ] Add tests for prompt listing, retrieval, and notifications

## 4. Resources
- [ ] Implement resource listing, reading, and template expansion
    - [ ] Define `Resource`, `ResourceTemplate`, and content types
    - [ ] Support `resources/list`, `resources/read`, and `resources/templates/list` requests
    - [ ] Implement notifications: `notifications/resources/list_changed`, `notifications/resources/updated`
    - [ ] Support resource subscription/unsubscription if capable
    - [ ] Document resource management, templates, and notification flow
    - [ ] Add tests for resource listing, reading, and notifications

## 5. Tools
- [ ] Implement tool registration, listing, and invocation
    - [ ] Define `Tool`, `ToolCallResult`, and tool schema types
    - [ ] Support `tools/list` and tool call requests
    - [ ] Implement tool result and error handling (`isError` flag)
    - [ ] Support notifications: `notifications/tools/listChanged`
    - [ ] Document tool registration, invocation, and result flow
    - [ ] Add tests for tool listing, invocation, and notifications

## 6. Sampling
- [ ] Implement message sampling and completion requests
    - [ ] Define `SamplingMessage`, `ModelPreferences`, and sampling parameter types
    - [ ] Support `sampling/createMessage` request and response
    - [ ] Implement sampling parameter handling (temperature, maxTokens, stopSequences, etc.)
    - [ ] Enforce user approval and security for sampling requests
    - [ ] Document sampling flow, parameters, and controls
    - [ ] Add tests for sampling requests, responses, and edge cases 