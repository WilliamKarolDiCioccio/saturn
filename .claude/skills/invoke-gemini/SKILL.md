---
name: invoke-gemini
description: Delegate large file analysis and bulk code generation to Gemini CLI. Use when files exceed 1000 lines, structural file splitting needed, or repetitive boilerplate generation required. NOT for architecture, ECS, renderer, threading, or memory systems.
allowed-tools: ["Bash", "Read", "Write", "Grep", "Glob"]
---

# Invoke Gemini

Gemini CLI integration for large-context and bulk-generation tasks within Saturn.

**Authority:** Claude Code is sole decision-maker. Gemini is constrained executor.

## Configuration

```
LARGE_FILE_LINE_THRESHOLD = 1000
DEFAULT_GEMINI_MODEL = gemini-2.5-flash
```

## When to Use

**MUST invoke when:**

- File exceeds 1000 lines
- Structural understanding of very large file required
- Repetitive, pattern-based, low-risk code generation

**NEVER invoke for:**

- Renderer internals
- ECS
- Threading / async systems
- Memory ownership
- Vulkan / WebGPU logic
- Engine architecture
- Any architectural decisions

## Pre-Flight Check (MANDATORY)

Before reading ANY file, check line count:

```bash
python scripts/check_line_count.py <file_path>
```

Output: `DELEGATE` (>1000 lines) or `PROCEED` (<=1000 lines)

If `DELEGATE`: Claude MUST NOT read file directly.

## Action 1: Large File Analysis

### When

File exceeds 1000 lines and Claude needs structural understanding.

### Execute

```bash
python scripts/analyze_large_file.py <file_path>
```

### Post-Execution

- Claude reads only the output file
- Claude bases decisions on Gemini's summary
- Claude NEVER re-scans large file manually

## Action 2: Structural File Splitting

### When

Large file needs decomposition into logical components.

### Execute

```bash
python scripts/split_file.py <file_path> --intent "high-level goal"
```

### Post-Execution

- Claude validates Gemini's split boundaries
- Claude applies split (does not override boundaries)

## Action 3: Bulk Code Generation

### When

Repetitive, pattern-based, low-risk generation needed.

### Encouraged Domains

- Flutter UI code
- Dart bindings
- Generated adapters / glue code
- Serialization / deserialization
- Repetitive C++ declarations
- Platform boilerplate

### Forbidden Domains

- Renderer internals
- ECS
- Threading / async
- Memory ownership
- Vulkan / WebGPU
- Engine architecture

### Execute

```bash
python scripts/generate_bulk.py --spec "specification here"
```

### Workflow

1. Claude defines WHAT is needed (spec, constraints, patterns)
2. Claude invokes Gemini with spec
3. Gemini generates bulk output
4. Claude reviews, edits if necessary
5. Claude integrates

## Guardrails

**Invalid outputs (reject immediately):**

- Claude analyzed >1000 line file directly
- Gemini used for architecture/engine reasoning
- Gemini output integrated without Claude review
- CLAUDE.md rules bypassed

## Context Rules

All Gemini prompts MUST include:

- Reference to relevant CLAUDE.md files
- Instruction to assume Saturn conventions
- Explicit prohibition on new patterns

## Reference

See `references/GEMINI.md` for full Gemini role definition and constraints.
