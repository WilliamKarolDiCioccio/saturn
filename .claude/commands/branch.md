---
description: Create conventional branch from main based on task description. Parse description to determine type (feat/fix/docs/style/refactor/perf/test/build/ci/chore), extract optional scope from context, convert to kebab-case. Checkout main, pull latest, create and checkout new branch. Format: type/scope/description or type/description if no scope. Follow https://conventional-branch.github.io/#summary
argument-hint: <description>
---

# /branch

## Purpose

Create conventional branch from main based on task description. Parse description to determine type (feat/fix/docs/style/refactor/perf/test/build/ci/chore), extract optional scope from context, convert to kebab-case. Checkout main, pull latest, create and checkout new branch. Format: type/scope/description or type/description if no scope.

## Contract

**Inputs:** `<description>` — task description (e.g., "add user authentication", "fix login bug")
**Outputs:** `STATUS=<OK|FAIL> BRANCH=<name> [ERROR=<message>]`

## Instructions

1. **Parse description to determine branch type:**

   - `feat/` — new feature
   - `fix/` — bug fix
   - `docs/` — documentation changes
   - `style/` — formatting, missing semi-colons, etc.
   - `refactor/` — code restructuring without behavior change
   - `perf/` — performance improvements
   - `test/` — adding or updating tests
   - `build/` — build system or dependency changes
   - `ci/` — CI/CD configuration
   - `chore/` — maintenance tasks

2. **Extract scope (optional):**

   - Look for keywords: ecs, input, graphics, platform, window, exec, core, scene, etc.
   - If scope is clear from description, use format: `type/scope/description`
   - Otherwise use: `type/description`

3. **Convert description to kebab-case:**

   - Lowercase all words
   - Replace spaces with hyphens
   - Remove special characters
   - Keep it concise (max 5-6 words)

4. **Execute git operations:**

```bash
# Ensure we're on main and up-to-date
git checkout main
git pull origin main

# Create and checkout new branch
git checkout -b <branch-name>

# Confirm operation success
git branch --show-current
```

5. **Output status:**
   - Print `STATUS=OK BRANCH=<branch-name>` on success
   - Print `STATUS=FAIL ERROR="message"` on failure

## Examples

**Input:** `/branch add user authentication`
**Output:** `STATUS=OK BRANCH=feat/add-user-authentication`

**Input:** `/branch fix input system crash`
**Output:** `STATUS=OK BRANCH=fix/input/system-crash`

**Input:** `/branch update ecs documentation`
**Output:** `STATUS=OK BRANCH=docs/ecs/update-documentation`

## Constraints

- Idempotent: checks if branch exists before creating
- No network tools except git pull
- Minimal console output (STATUS line only)
