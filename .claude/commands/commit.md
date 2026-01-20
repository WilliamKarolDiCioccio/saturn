---
description: Generate conventional commit message from git changes. Analyze staged changes (or stage all if none staged). Infer type and scope from files modified. Craft subject line (max 72 chars) and optional body explaining what/why. Follow https://www.conventionalcommits.org/en/v1.0.0/#summary. Present message for approval before committing.
argument-hint: [message]
---

# /commit

## Purpose

Generate conventional commit message from git changes. Analyze staged changes (or stage all if none staged). Infer type and scope from files modified. Craft subject line (max 72 chars) and optional body explaining what/why. Present message for approval before committing.

## Contract

**Inputs:** `[message]` — optional commit message override
**Outputs:** `STATUS=<OK|FAIL> COMMIT=<hash> [ERROR=<message>]`

## Instructions

1. **Analyze git state:**

```bash
# Check for staged changes
git diff --cached --name-status

# If no staged changes, check working directory
git status --short
```

2. **Determine commit type from changes:**
   - `feat:` — new files in src/, new public APIs
   - `fix:` — bug fixes, patches
   - `docs:` — changes to \*.md, comments, docsgen/
   - `style:` — formatting, whitespace
   - `refactor:` — code restructuring without behavior change
   - `perf:` — performance improvements
   - `test:` — changes to _/tests/_, _/bench/_
   - `build:` — CMakeLists.txt, vcpkg.json, cmake/
   - `ci:` — .github/, CI configs
   - `chore:` — maintenance, tooling

3. **Infer scope from file paths:**
   - `pieces/` → scope: pieces
   - `saturn/src/ecs/` → scope: ecs
   - `saturn/src/input/` → scope: input
   - `saturn/src/graphics/` → scope: graphics
   - `saturn/src/platform/` → scope: platform
   - `saturn/src/window/` → scope: window
   - `saturn/src/exec/` → scope: exec
   - `saturn/src/core/` → scope: core
   - `saturn/src/scene/` → scope: scene
   - `codex/` → scope: codex
   - `docsgen/` → scope: docsgen
   - Multiple scopes → omit scope or use broader category

4. **Craft commit message:**
   - **Subject line:** `type(scope): subject` (max 72 chars)
   - Use imperative mood ("add" not "added")
   - No period at end
   - **Body (optional):** Explain what/why, not how
   - Wrap body at 72 characters
   - Separate subject and body with blank line

5. **Present for approval:**
   - Show generated message
   - Ask user to confirm, edit, or cancel
   - If user provides message argument, skip generation and use it

6. **Execute commit:**

```bash
# Stage all if nothing staged
[[ -z $(git diff --cached) ]] && git add -A

# Commit with generated/provided message
git commit -m "<message>"
```

7. **Output status:**
   - Print `STATUS=OK COMMIT=<hash>` on success
   - Print `STATUS=FAIL ERROR="message"` on failure

## Examples

**Changes:** Added ECS archetype iteration
**Message:**

```
feat(ecs): add archetype iteration support

Implements forEach() for iterating entities within archetypes.
Provides type-safe component access during iteration.
```

**Changes:** Fixed input context memory leak
**Message:**

```
fix(input): resolve context memory leak

InputContext now properly releases sources on destruction.
```

**Changes:** Updated build documentation
**Message:**

```
docs(build): update CMake preset instructions
```

## Constraints

- Idempotent: checks for changes before committing
- No network tools
- Interactive approval required unless message provided
- Follows conventional commits spec strictly
