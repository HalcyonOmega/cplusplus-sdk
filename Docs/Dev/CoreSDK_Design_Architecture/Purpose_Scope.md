# Purpose & Scope

---

## What is This SDK?

This project is a C++ SDK for the Model Context Protocol (MCP) by Anthropic. The goal is to provide a robust, standards-compliant implementation of MCP for C++ developers, making it easy to integrate LLM applications with external data sources, tools, and services using the MCP spec.

For all protocol details, see the [SDK_FullReference.md](../../Admin/SDK_FullReference.md) — that's the source of truth for how MCP works and what this SDK aims to support.

## Project Vision

- **Build a high-quality, idiomatic C++ SDK for MCP.**
- **Enable easy integration with LLMs and external tools/data.**
- **Follow the official MCP spec as closely as possible.**
- **Lay the groundwork for this to (hopefully!) become the official C++ SDK for MCP, in collaboration with Anthropic.**

## Who's Building This?

Right now, it's a solo effort — just me, doing my best to make something great and useful for the community (and maybe for Anthropic, too).

## In Scope

- Full support for the MCP protocol as described in the spec (see SDK_FullReference.md)
- Clean, modern C++ API design
- Cross-platform support (Windows, Linux, macOS)
- Minimal dependencies, CMake-based build
- Comprehensive documentation and examples

## Out of Scope (for now)

- Non-C++ language bindings (Python, Rust, etc.)
- Deep integration with non-standard or legacy C++ toolchains
- Features not described in the MCP spec (unless clearly useful for C++ devs)
- Official endorsement by Anthropic (unless/until they want to collaborate!)

## Why Does This Matter?

MCP is a powerful protocol for connecting LLMs to the outside world. Having a solid C++ SDK means more developers can build cool, reliable, and secure integrations — and that's good for everyone.

---

**TL;DR:**
- C++ SDK for Anthropic's Model Context Protocol (MCP)
- Goal: robust, spec-compliant, cross-platform, and (hopefully) official
- See SDK_FullReference.md for all the protocol details
- Built solo, but open to collaboration! 