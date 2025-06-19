# MCP Specification Compliance Review Prompt

## Task Overview
You are an expert AI agent tasked with conducting a comprehensive compliance review of a C++ Model Context Protocol (MCP) SDK implementation against the official MCP specification (2025-03-26 revision). Your goal is to identify any gaps, deviations, or non-compliance issues in the framework.

## Framework to Review
The C++ MCP SDK framework consists of files in `Source/Public/Sandbox/MCPTest/`. DO NOT REVIEW OR ATTEMPT TO REFERENCE ANY FILES OUTSIDE OF THIS DIRECTORY.

## MCP Specification Reference
Use the official Model Context Protocol specification (2025-03-26) as your compliance baseline:
- https://modelcontextprotocol.io/specification/2025-03-26/
- Focus on: Basic Protocol, Transports, Client Features, Server Features
- Key areas: Messages, Lifecycle, Capabilities, Tools, Prompts, Resources, Sampling, Roots

## Review Criteria

### 1. Core Protocol Compliance
- **JSON-RPC 2.0 Adherence**: Verify all message structures follow JSON-RPC 2.0 spec
- **Message Types**: Confirm proper implementation of requests, responses, notifications
- **ID Handling**: Check request/response ID matching and uniqueness requirements
- **Error Handling**: Validate error code structures and proper error propagation

### 2. Lifecycle Management
- **Initialization Phase**: Verify proper capability negotiation flow
- **Operation Phase**: Check normal protocol communication patterns  
- **Shutdown Phase**: Confirm graceful connection termination
- **State Management**: Review connection state tracking and transitions

### 3. Transport Layer Compliance
- **Stdio Transport**: Validate stdin/stdout communication protocol
- **StreamableHTTP Transport**: Check SSE (Server-Sent Events) implementation
- **Transport Abstraction**: Verify proper separation of concerns
- **Message Framing**: Confirm correct message boundary handling

### 4. Message Structure Compliance
For each message type, verify:
- **Request Messages**: Proper structure, required fields, parameter handling
- **Response Messages**: Correct result/error format, proper ID correlation
- **Notification Messages**: No ID field, proper method/params structure
- **Error Messages**: Standard error codes, message format, optional data field

### 5. Feature Implementation
#### Core Features (Required)
- **Tools**: List tools, call tool, tool result handling
- **Prompts**: List prompts, get prompt, prompt arguments
- **Resources**: List resources, read resource, resource subscriptions
- **Capabilities**: Proper capability declaration and negotiation

#### Optional Features
- **Sampling**: Model interaction requests, preference handling
- **Roots**: Root directory listing and management
- **Logging**: Log level handling, log output
- **Progress**: Progress notification support
- **Completion**: Argument completion for tools/prompts/resources

### 6. Data Type Compliance
- **Content Types**: Text, image, embedded resource content
- **Role Enumeration**: User, assistant role handling
- **Capability Structures**: Client/server capability declarations
- **Annotation Support**: Meta tags, audience, priority
- **URI Handling**: Proper URI template and resolution

### 7. Security & Error Handling
- **Input Validation**: Parameter validation and sanitization
- **Error Propagation**: Proper error code usage and messaging
- **Resource Access**: Safe resource access patterns
- **Authentication**: Bearer token support if implemented

### 8. C++ Specific Considerations
- **Memory Safety**: RAII patterns, smart pointers, exception safety
- **Async Patterns**: Proper coroutine usage, thread safety
- **Type Safety**: Strong typing, proper const-correctness
- **Modern C++**: C++20 feature usage appropriateness

## Review Process

### Step 1: Specification Mapping
1. Read each specification section thoroughly
2. Map specification requirements to code implementations
3. Create a compliance matrix for each major feature area
4. Identify any specification features not addressed

### Step 2: Code Analysis
1. Examine each header file for structural compliance
2. Analyze message flow patterns against specification
3. Check data type definitions against specification schemas
4. Verify transport implementations match specification requirements

### Step 3: Gap Analysis
1. Identify missing features or partial implementations
2. Find deviations from specification requirements
3. Note areas where implementation exceeds specification
4. Highlight potential compatibility issues

### Step 4: Quality Assessment
1. Evaluate code organization and maintainability
2. Check documentation completeness and accuracy
3. Assess example usage patterns
4. Review error handling robustness

## Deliverables

### Compliance Report Structure
```
# MCP C++ SDK Compliance Review Report

## Executive Summary
- Overall compliance score (0-100%)
- Critical issues count
- Major findings summary

## Detailed Findings

### 1. Core Protocol Compliance
- [Compliant/Non-Compliant/Partial] with details
- Specific issues and recommendations

### 2. Message Structure Compliance
- Review of each message type
- JSON-RPC adherence assessment
- Data type mapping accuracy

### 3. Transport Implementation
- Stdio transport compliance
- HTTP transport compliance
- Abstraction layer quality

### 4. Feature Coverage
- Required features status
- Optional features status
- Missing implementations

### 5. Quality Assessment
- Code organization
- Documentation quality
- Example completeness
- Error handling robustness

## Recommendations
### Critical Issues (Must Fix)
- List blocking compliance issues

### Major Issues (Should Fix)
- List important but non-blocking issues

### Minor Issues (Could Fix)
- List nice-to-have improvements

### Best Practices
- Suggest implementation improvements
- Recommend additional features

## Implementation Roadmap
- Priority-ordered list of required changes
- Estimated effort for each change
- Dependencies between changes
```

### Specific Focus Areas
1. **Message Validation**: Ensure all message types exactly match specification
2. **Capability Negotiation**: Verify proper client-server capability exchange
3. **Transport Protocols**: Confirm transport implementations are specification-compliant
4. **Error Handling**: Check error codes and structures match specification
5. **Data Types**: Validate all data structures against specification schemas
6. **Async Patterns**: Ensure coroutine usage aligns with protocol requirements
7. **Resource Management**: Verify proper resource lifecycle handling
8. **Security Considerations**: Check for potential security issues

## Success Criteria
- [ ] 100% compliance with required MCP features
- [ ] Proper JSON-RPC 2.0 implementation
- [ ] Complete transport layer support
- [ ] Robust error handling throughout
- [ ] Clear documentation and examples
- [ ] Memory-safe C++ implementation
- [ ] Thread-safe async operations
- [ ] Extensible architecture for future features

## Notes
- Focus on specification compliance over implementation efficiency
- Flag any areas where specification is ambiguous
- Suggest clarifications needed from MCP specification authors
- Consider cross-platform compatibility requirements
- Evaluate integration complexity for end users

Review the framework comprehensively and provide actionable recommendations for achieving full MCP specification compliance. 