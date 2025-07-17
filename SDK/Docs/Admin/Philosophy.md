# Philosophy

## Communication

- Every request should be addressed. Optional responses should not be permitted - verify the response is not null.

## Error Handling

- Errors should be handled gracefully. If an error occurs, the server should respond with an error message.

## Dependencies

- We want to be backwards compatible with as many versions of C++ as possible WITHOUT depending on external libraries if at all possible. I do not want to use very early versions of C++ if it makes a major sacrifice in functionality, does not comply with MCP Spec, or provides significant degredation in performance, developer experience, or maintainability.
