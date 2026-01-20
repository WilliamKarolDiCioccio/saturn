# GEMINI.md

Gemini CLI integration rules for Saturn Game Engine.

**Gemini is a SUBORDINATE support tool. Claude Code is sole authority.**

## Role

| Authority   | Responsibility                                             |
| ----------- | ---------------------------------------------------------- |
| Claude Code | Architecture, design, validation, integration, acceptance  |
| Gemini CLI  | Large file analysis, structural splitting, bulk generation |

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

- `>1000 lines` → Gemini MUST analyze; Claude MUST NOT read directly
- `≤1000 lines` → Claude proceeds normally

## Permitted Operations

### 1. Large File Analysis (>1000 lines)

- Identify structures (classes, functions, modules)
- Document invariants, assumptions, coupling
- NO code rewriting, NO design suggestions

### 2. Structural File Splitting

- Propose split boundaries
- Preserve dependencies and execution order
- NO logic refactoring, NO new abstractions

### 3. Bulk Code Generation

**Encouraged:**

- Flutter UI, Dart bindings
- Adapters, glue code, serialization
- Repetitive C++ declarations
- Platform boilerplate

**Forbidden:**

- Renderer internals
- ECS
- Threading / async
- Memory ownership
- Vulkan / WebGPU
- Engine architecture

## Workflow

```
1. Claude identifies task
2. Claude checks file size
3. If >1000 lines OR bulk generation:
   a. Claude defines spec
   b. Gemini executes
   c. Claude reviews
   d. Claude integrates
4. Otherwise: Claude proceeds directly
```

## Context Rules

All Gemini prompts MUST:

- Reference relevant CLAUDE.md files
- Assume Saturn conventions
- Prohibit new patterns
- Prohibit architectural decisions

## Guardrails

| Violation                        | Action              |
| -------------------------------- | ------------------- |
| Claude read >1000 line file      | Restart with Gemini |
| Gemini made arch decision        | Reject, re-prompt   |
| Output integrated without review | Rollback            |
| CLAUDE.md bypassed               | Reject entirely     |

## Skill Location

Full implementation: `.claude/skills/invoke-gemini/`
