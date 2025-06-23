# MCPTypes.h vs. Schema.json: Type Structure Compliance

## Non-Compliant Type Structures

---

### 1. EmbeddedResource
- **Schema:** `resource` field is a union: can be either `TextResourceContents` (with `text`, `uri`, optional `mimeType`) or `BlobResourceContents` (with `blob`, `uri`, optional `mimeType`).
- **C++:** `EmbeddedResource::Resource` is a struct with only `URI`, `Text`, and `MimeType` (no support for `blob`).
- **Delta:** C++ does not support the `blob` variant. Needs to model `resource` as a variant/union of text and blob types.

---

### 2. Resource
- **Schema:** Optional fields: `annotations` (object), `size` (integer).
- **C++:** Missing both `Annotations` and `Size` fields.
- **Delta:** Add `std::optional<Annotations> Annotations;` and `std::optional<int64_t> Size;`.

---

### 3. ResourceTemplate
- **Schema:** Optional field: `annotations` (object).
- **C++:** Missing `Annotations` field.
- **Delta:** Add `std::optional<Annotations> Annotations;`.

---

### 4. ToolAnnotations
- **Schema:** Optional fields: `destructiveHint`, `idempotentHint`, `openWorldHint`, `readOnlyHint` (all bool), `title` (string).
- **C++:** Only has `Audience` and `Priority` (not in schema).
- **Delta:** Add all missing fields as `std::optional<bool>` or `std::optional<std::string>` as appropriate. Remove non-schema fields if strict compliance is required.

---

### 5. Tool
- **Schema:** Optional field: `annotations` (type: ToolAnnotations).
- **C++:** Field present but named differently (`ToolAnnotations` instead of `Annotations`).
- **Delta:** Ensure the field type matches the schema (type is correct, naming will be handled later).

---

### 6. SamplingMessage
- **Schema:** `content` is a union of `TextContent`, `ImageContent`, `AudioContent` only.
- **C++:** `Content` type also allows `EmbeddedResource`.
- **Delta:** Restrict `Content` variant for `SamplingMessage` to only the three allowed types.

---

### 7. PromptMessage
- **Schema:** Struct with `role` and `content` (union: `TextContent`, `ImageContent`, `AudioContent`, `EmbeddedResource`).
- **C++:** Not defined.
- **Delta:** Add `PromptMessage` struct with correct type structure.

---

### 8. General: Annotations
- **Schema:** Used as an optional field in multiple types (e.g., `Resource`, `ResourceTemplate`, `TextContent`, etc.).
- **C++:** Not present in all required places.
- **Delta:** Ensure `Annotations` is present as an optional field everywhere the schema allows.

---

**Note:**
- Enum value casing and field naming are not addressed here, per instructions.
- Only type structure mismatches are listed.
