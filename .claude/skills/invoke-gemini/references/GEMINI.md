# GEMINI.md

Gemini CLI integration rules for Saturn Game Engine.

## Role Definition

**Gemini CLI is a SUBORDINATE support tool. Claude Code is the sole authority.**

Gemini:

- Executes constrained tasks defined by Claude
- Never reasons architecturally
- Never operates without explicit instruction
- Never introduces new patterns
- Never overrides CLAUDE.md rules

## Authority Hierarchy

```
Claude Code (AUTHORITY)
    ├── Architecture decisions
    ├── Design decisions
    ├── Validation
    ├── Integration
    └── Final acceptance

Gemini CLI (EXECUTOR)
    ├── Large file analysis (>1000 lines)
    ├── Structural splitting
    └── Bulk code generation
```

## Configuration

| Setting                   | Value            |
| ------------------------- | ---------------- |
| LARGE_FILE_LINE_THRESHOLD | 1000             |
| DEFAULT_GEMINI_MODEL      | gemini-2.5-flash |

## Mandatory Pre-Check

Before Claude reads ANY file:

```bash
python .claude/skills/invoke-gemini/scripts/check_line_count.py <file>
```

| Line Count | Action                         |
| ---------- | ------------------------------ |
| ≤1000      | Claude proceeds normally       |
| >1000      | Claude MUST delegate to Gemini |

## Permitted Operations

### 1. Large File Analysis

**Trigger:** File >1000 lines needs structural understanding

**Gemini responsibilities:**

- Identify classes, functions, modules
- Highlight change-relevant areas
- Document invariants and assumptions
- Note coupling and risk points

**Gemini prohibitions:**

- No code rewriting
- No design suggestions
- No architectural changes

### 2. Structural File Splitting

**Trigger:** Large file needs decomposition

**Gemini responsibilities:**

- Propose logical split boundaries
- Preserve imports/exports/dependencies
- Maintain execution order
- Document split plan with filenames

**Gemini prohibitions:**

- No logic refactoring
- No new abstractions
- No behavior changes

### 3. Bulk Code Generation

**Trigger:** Repetitive, pattern-based generation

**Permitted domains:**

- Flutter UI code
- Dart bindings
- Adapters / glue code
- Serialization
- Repetitive C++ declarations
- Platform boilerplate

**Forbidden domains:**

- Renderer internals
- ECS
- Threading / async
- Memory ownership
- Vulkan / WebGPU
- Engine architecture

## Context Rules

Every Gemini prompt MUST include:

1. Reference to relevant CLAUDE.md files
2. Instruction: "Assume Saturn conventions per CLAUDE.md"
3. Prohibition: "Do NOT introduce new patterns"
4. Prohibition: "Do NOT make architectural decisions"

## Output Requirements

All Gemini responses must end with:

```
TASK COMPLETED
```

Claude MUST:

- Wait for completion marker
- Review all output before integration
- Reject output that violates constraints

## Guardrail Violations

**Immediate rejection triggers:**

| Violation                            | Action                             |
| ------------------------------------ | ---------------------------------- |
| Claude read >1000 line file directly | Restart with Gemini delegation     |
| Gemini made architectural decision   | Reject, re-prompt with constraints |
| Output integrated without review     | Rollback, review first             |
| CLAUDE.md rules bypassed             | Reject entirely                    |

## Integration with CLAUDE.md Hierarchy

Gemini operates WITHIN the existing context system:

```
Root CLAUDE.md (global patterns)
    └── Package CLAUDE.md (local invariants)
        └── Gemini (constrained executor)
```

Gemini:

- Reads CLAUDE.md context when instructed
- Never modifies CLAUDE.md files
- Follows all documented invariants
- Defers to Claude on conflicts

## Workflow Summary

```
1. Claude identifies task
2. Claude checks file size (mandatory)
3. If >1000 lines OR bulk generation needed:
   a. Claude defines spec/constraints
   b. Claude invokes Gemini with spec
   c. Gemini executes, returns output
   d. Claude reviews output
   e. Claude integrates (or rejects)
4. If ≤1000 lines AND not bulk:
   Claude proceeds directly
```
