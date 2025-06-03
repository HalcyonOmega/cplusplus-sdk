# Phase 3: Schema-Driven Type System

## 1. Integrate auto-generated schemas

- [ ] Use the TypeScript/JSON schemas as the source of truth
    - [ ] Reference `Schema.ts` and `Schema.json` for all protocol types, enums, and objects
    - [ ] Regularly update C++ types to match schema changes
- [ ] Generate or hand-author C++ types that match the protocol schemas
    - [ ] Map all protocol message structures, enums, and objects to C++ structs/classes
    - [ ] Use strong typing for all fields (avoid generic maps/variants unless required by schema)
    - [ ] Document mapping rules and any manual adjustments
    - [ ] Organize generated/hand-authored types in `Source/Public/Schemas/`
    - [ ] Ensure all message structures, enums, and objects are covered (Client, Server, Common)
    - [ ] Provide clear separation between auto-generated and hand-authored code
- [ ] Integrate schema updates into the build or CI process
    - [ ] Add checks to detect schema drift between C++ and JSON/TypeScript definitions
    - [ ] Document the schema update workflow for contributors

## 2. Validation and Serialization

- [ ] Implement schema-based validation for all incoming/outgoing messages
    - [ ] Validate C++ objects against the JSON schema before serialization
    - [ ] Validate incoming JSON against the schema before deserialization
    - [ ] Provide clear error reporting for validation failures
- [ ] Provide robust (de)serialization to/from JSON
    - [ ] Use a modern C++ JSON library (e.g., nlohmann/json) for serialization/deserialization
    - [ ] Implement or generate `to_json`/`from_json` functions for all protocol types
    - [ ] Use macros (e.g., `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE`) to reduce boilerplate
    - [ ] Ensure enums and custom types are correctly mapped to/from JSON
    - [ ] Support serialization/deserialization of STL containers and optional fields
    - [ ] Add unit tests for all (de)serialization logic
    - [ ] Document serialization conventions and edge cases 