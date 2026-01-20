# Package: docsgen

**Location:** `docsgen/`
**Type:** Executable (CLI tool)
**Dependencies:** codex, bfgroup-lyra (CLI parsing)

---

## Purpose & Responsibility

### Owns

- MDX documentation generation from codex SourceNode AST
- CLI interface for documentation generation (--input, --output flags)
- File output management (writing .mdx files to output directory)
- Formatting SourceNode data into human-readable documentation
- Integration with Astro-based docs site (docs/ directory)

### Does NOT Own

- C++ source parsing (codex's responsibility)
- AST construction (codex's responsibility)
- Documentation hosting or serving (static site generator's responsibility)
- Markdown rendering (Astro/MDX runtime's responsibility)
- Build system integration beyond executable invocation

---

## Key Abstractions & Invariants

### Core Types

- **`MDXGenerator`** (`generators/mdx_generator.hpp`) — Converts SourceNode tree to MDX files
- **CLI arguments** (`main.cpp`) — Input directory, output directory (via lyra)

### Invariants (NEVER violate)

1. **Codex dependency**: MUST consume codex's SourceNode output (no direct tree-sitter usage)
2. **MDX output**: Generated files MUST be valid MDX (Markdown + JSX)
3. **File system**: MUST write to output directory (never modify input files)
4. **Deterministic output**: Same input MUST produce same output (no timestamps in generated files unless explicit)
5. **CLI contract**: --input and --output flags MUST be respected

### Architectural Patterns

- **Pipeline consumer**: Terminal node in FilesCollector → Parser → MDXGenerator pipeline
- **Generator pattern**: Visits SourceNode tree, emits MDX strings
- **CLI tool**: Stateless single-invocation executable

---

## Architectural Constraints

### Dependency Rules

**Allowed:**

- codex (SourceNode, Parser, FilesCollector, etc.)
- bfgroup-lyra (CLI argument parsing)
- STL (filesystem, iostream, string, etc.)

**Forbidden:**

- ❌ saturn headers — docsgen is tooling, not engine-coupled
- ❌ pieces directly — use via codex if needed
- ❌ GUI libraries — CLI-only tool
- ❌ Network libraries — local filesystem only

### Layering

- docsgen depends on codex (consumes output)
- codex does NOT depend on docsgen (unidirectional)
- saturn does NOT depend on docsgen (tooling, not library)

### Threading Model

- **Single-threaded**: Processes files sequentially
- **No concurrency**: MDXGenerator not thread-safe

### Lifetime & Ownership

- **SourceNode**: Owned by codex, docsgen holds const shared_ptr references
- **Output files**: Owned by filesystem, docsgen creates/overwrites

### Platform Constraints

- Must support: Windows, Linux, macOS (cross-platform CLI)
- Filesystem paths use std::filesystem
- No platform-specific documentation generation

---

## Modification Rules

### Safe to Change

- Add new generator types (JSONGenerator, HTMLGenerator, etc.)
- Extend MDX output format (add frontmatter, custom components)
- Improve CLI flags (--verbose, --exclude, --include-private)
- Add template customization (custom MDX templates)
- Optimize file writing (batch writes, parallel generation)

### Requires Coordination

- Changing MDX output format affects Astro docs site (verify docs/ still builds)
- Modifying CLI flags breaks existing build scripts (update cmake/scripts)
- Altering SourceNode consumption breaks with codex updates (pin codex version)

### Almost Never Change

- **codex dependency** — switching parsers invalidates entire tool
- **MDX output format** — Astro docs site depends on this structure
- **CLI contract** — build scripts and documentation generation workflows depend on flags

---

## Common Pitfalls

### Footguns

- ⚠️ **Overwriting existing files**: docsgen WILL overwrite output directory (no backup)
- ⚠️ **Invalid MDX syntax**: Generated MDX MUST escape JSX characters ({, }, <, >, etc.)
- ⚠️ **Large output files**: Generating docs for entire saturn codebase produces 100s of files
- ⚠️ **Path separators**: Windows vs Unix path separators in file links (use std::filesystem)
- ⚠️ **Encoding**: Output MUST be UTF-8 (some editors default to system encoding)

### Performance Traps

- 🐌 **Sequential file generation**: Processing 1000+ headers one-by-one is slow (consider parallelization)
- 🐌 **String concatenation**: Building large MDX strings with += is O(n²) (use stringstream)
- 🐌 **File I/O**: Writing many small files is slower than batching (buffered writes)

### Historical Mistakes (Do NOT repeat)

- **Generating HTML directly**: Switched to MDX for Astro integration (more flexible)
- **Embedding JSON in MDX**: Moved to frontmatter YAML (cleaner, better tooling support)

---

## How Claude Should Help

### Expected Tasks

- Implement MDXGenerator::generate() method (currently stubbed)
- Add CLI argument parsing (--input, --output, --verbose)
- Extend MDX output with frontmatter (title, description, category)
- Add support for code examples in documentation (syntax highlighting)
- Implement custom MDX components for API references
- Write tests for generator output (snapshot testing)
- Add incremental generation (only regenerate changed files)

### Conservative Approach Required

- **Changing MDX format**: Verify Astro docs site compatibility before deploying
- **Modifying SourceNode consumption**: Coordinate with codex package changes
- **Breaking CLI flags**: Update all build scripts and documentation first

### Before Making Changes

- [ ] Test generated MDX renders correctly in Astro (docs/ site)
- [ ] Verify CLI flags work on Windows and Linux (path separators)
- [ ] Run docsgen on full saturn/include directory (stress test)
- [ ] Check output files are valid UTF-8 (no encoding issues)
- [ ] Confirm no file overwrites without user intent (safety check)

---

## Quick Reference

### Files

**Public API:**

- `src/main.cpp` — Entry point, CLI argument handling
- `src/generators/mdx_generator.hpp` — MDXGenerator class
- `src/generators/mdx_generator.cpp` — MDXGenerator implementation

**Tests:**

- None currently (tests needed)

### Key Functions/Methods

- `MDXGenerator::generate(const vector<shared_ptr<SourceNode>>&)` — Generate MDX files from AST
- `main()` — CLI entry point: parse args → collect files → parse → generate

### Build Flags

- None (executable, standard C++ compilation)

**Invocation:**

```bash
./build/docsgen/docsgen --input ./saturn/include --output ./docs
```

---

## Status Notes

**In Development** — MDXGenerator is currently stubbed. Core implementation needed before production use.
