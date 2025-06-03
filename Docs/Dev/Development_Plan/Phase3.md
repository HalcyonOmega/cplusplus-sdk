# Phase 3: Schema-Driven Type System

1. **Integrate auto-generated schemas**
   - Use the TypeScript/JSON schemas as the source of truth.
   - Generate or hand-author C++ types that match the protocol schemas.
   - Ensure all message structures, enums, and objects are covered.

2. **Validation and Serialization**
   - Implement schema-based validation for all incoming/outgoing messages.
   - Provide robust (de)serialization to/from JSON. 