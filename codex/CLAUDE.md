# Package: codex

**Location:** `codex/`
**Type:** Static library
**Dependencies:** pieces, fmt (header-only), tree-sitter, tree-sitter-cpp

---

## Purpose & Responsibility

### Owns

- C++ source code parsing via tree-sitter
- Source file collection and preprocessing
- AST node extraction (classes, functions, namespaces, enums, etc.)
- Documentation comment extraction
- Structured data output for documentation generation
- NodeKind taxonomy (Source, Namespace, Class, Function, etc.)

### Does NOT Own

- Documentation rendering or formatting (docsgen's responsibility)
- Markdown/HTML/MDX generation (docsgen's responsibility)
- Build system integration beyond parsing .cpp/.hpp files
- saturn engine features (ECS, rendering, platform, etc.)
- Semantic C++ analysis beyond syntax tree parsing

---

## Key Abstractions & Invariants

### Core Types

- **`Source`** (`source.hpp`) — Represents parsed file (name, path, content, encoding, lastModifiedTime)
- **`SourceNode`** (`nodes.hpp`) — AST node base type with NodeKind, children, metadata
- **`NodeKind`** (`nodes.hpp`) — Enum of C++ constructs (Class, Function, Namespace, Enum, etc.)
- **`Parser`** (`parser.hpp`) — Tree-sitter based C++ parser, produces SourceNode tree
- **`FilesCollector`** (`files_collector.hpp`) — Recursively collects .hpp/.cpp files from directory
- **`Preprocessor`** (`preprocessor.hpp`) — Handles #include, #define, preprocessor directives
- **`SourceExtractor`** (`source_extractor.hpp`) — Extracts structured data from SourceNode tree

### Invariants (NEVER violate)

1. **Tree-sitter dependency**: MUST use tree-sitter-cpp for parsing (no hand-written lexer/parser)
2. **Syntax-only analysis**: NEVER attempt semantic analysis (type resolution, overload resolution, template instantiation)
3. **No saturn coupling**: MUST remain independent of saturn engine (can parse any C++ project)
4. **NodeKind completeness**: Every C++ construct in tree-sitter grammar MUST map to NodeKind entry
5. **Source immutability**: Source struct fields MUST NOT be modified after construction (treat as immutable)
6. **UTF-8 encoding**: All source content MUST be UTF-8 (encoding field documents actual encoding)

### Architectural Patterns

- **Pipeline architecture**: FilesCollector → Preprocessor → Parser → SourceExtractor
- **Tree visitor**: SourceNode hierarchy enables recursive tree traversal
- **Struct-based data**: Plain structs (Source, SourceNode) for data transfer to docsgen

---

## Architectural Constraints

### Dependency Rules

**Allowed:**

- pieces (Result<T, E>, containers, string utilities)
- fmt (formatting only)
- tree-sitter (C API)
- tree-sitter-cpp (C++ grammar)
- STL (filesystem, string, vector, map, etc.)

**Forbidden:**

- ❌ saturn headers — codex is tooling, not engine-coupled
- ❌ nlohmann-json — JSON serialization happens in docsgen, not codex
- ❌ GUI libraries — codex is CLI/library only
- ❌ Network libraries — codex operates on local filesystem

### Layering

- codex depends on pieces (utilities)
- docsgen depends on codex (consumes SourceNode output)
- saturn does NOT depend on codex (unidirectional)

### Threading Model

- **Single-threaded by design**: Parser operates on one source file at a time
- **Thread-safe**: No mutable state → safe to run multiple Parser instances in parallel (caller-managed)

### Lifetime & Ownership

- **Source**: Owned by caller, Parser takes const reference
- **SourceNode**: Shared ownership via std::shared_ptr (tree structure)
- **TSParser**: Owned by Parser class, destroyed on Parser destruction

### Platform Constraints

- Must support: Windows, Linux, macOS (tree-sitter cross-platform)
- Filesystem paths use std::filesystem (cross-platform)
- No platform-specific parsing logic

---

## Modification Rules

### Safe to Change

- Add new NodeKind entries for unsupported C++ constructs
- Extend SourceNode metadata (add fields for attributes, constexpr, noexcept, etc.)
- Improve preprocessor handling (#if/#ifdef logic)
- Add helper methods to SourceNode for common queries
- Optimize tree traversal algorithms

### Requires Coordination

- Changing SourceNode structure affects docsgen's consumption (verify docsgen still compiles)
- Modifying NodeKind enum requires docsgen updates (generator switch statements)
- Altering Source struct breaks docsgen's file handling
- Removing NodeKind entries breaks backward compatibility

### Almost Never Change

- **tree-sitter dependency** — switching parsers rewrites entire codebase
- **SourceNode shared_ptr ownership** — changing to unique_ptr breaks tree sharing
- **NodeKind enum values** — reordering breaks serialized data (if docsgen caches)

---

## Common Pitfalls

### Footguns

- ⚠️ **Tree-sitter API lifetimes**: TSTree MUST outlive all TSNode queries (keep tree alive)
- ⚠️ **Preprocessor incomplete**: #if/#ifdef not fully evaluated (codex doesn't resolve macros)
- ⚠️ **Template parsing**: Tree-sitter parses template syntax, but does NOT instantiate (syntax-only)
- ⚠️ **Comment extraction**: Comments MUST be adjacent to nodes (detached comments may be lost)
- ⚠️ **Error recovery**: Tree-sitter attempts error recovery (may produce incomplete AST on syntax errors)

### Performance Traps

- 🐌 **Large file parsing**: Tree-sitter is fast, but GBs of code still take time (batch files)
- 🐌 **Deep template nesting**: Heavily templated code produces large ASTs (consider depth limits)
- 🐌 **Recursive directory traversal**: FilesCollector walks entire tree (symlink cycles cause hangs)
- 🐌 **String allocations**: SourceNode stores strings (consider string interning if memory-constrained)

### Historical Mistakes (Do NOT repeat)

- **Attempting semantic analysis**: Tried to resolve types → removed (too complex for parser)
- **Adding JSON serialization to codex**: Moved to docsgen (separation of concerns)
- **Custom lexer for preprocessor**: Switched to tree-sitter's preprocessor support

---

## How Claude Should Help

### Expected Tasks

- Add support for new C++23/26 features (tree-sitter grammar updates)
- Improve documentation comment extraction (Doxygen, Javadoc style)
- Add NodeKind entries for missing constructs (friend declarations, concepts, modules)
- Extend SourceNode with additional metadata (noexcept, constexpr, consteval, etc.)
- Write tests for edge cases (nested namespaces, anonymous unions, etc.)
- Optimize parser performance (parallel file processing)
- Fix tree-sitter query bugs (incorrect node type matching)

### Conservative Approach Required

- **Changing SourceNode public API**: Verify docsgen compatibility before breaking changes
- **Modifying NodeKind enum**: Check docsgen's switch statements and generators
- **Altering tree-sitter query logic**: May break extraction of classes/functions (test thoroughly)
- **Removing NodeKind entries**: Breaks backward compatibility (deprecate instead)

### Before Making Changes

- [ ] Verify tree-sitter-cpp supports feature (check grammar version)
- [ ] Test on real saturn headers (saturn/include/saturn/\*_/_.hpp)
- [ ] Run codex unit tests (`codex/tests/`)
- [ ] Confirm docsgen still compiles and generates docs (`docsgen` tool)
- [ ] Check for memory leaks (tree-sitter owns memory, must free TSTree)

---

## Quick Reference

### Files

**Public API:**

- `include/codex/parser.hpp` — Parser class
- `include/codex/source.hpp` — Source struct
- `include/codex/nodes.hpp` — SourceNode, NodeKind, AccessSpecifier
- `include/codex/files_collector.hpp` — FilesCollector class
- `include/codex/preprocessor.hpp` — Preprocessor class
- `include/codex/source_extractor.hpp` — SourceExtractor class

**Internal:**

- `src/tree_sitter_cpp.hpp` — Tree-sitter C++ grammar bindings
- `src/*.cpp` — Implementations

**Tests:**

- `tests/main.cpp` — Unit tests

### Key Functions/Methods

- `Parser::parse(const vector<shared_ptr<Source>>&)` — Parse sources → SourceNode tree
- `FilesCollector::collect(const path&)` — Recursively find .hpp/.cpp files
- `Preprocessor::process(const Source&)` — Handle preprocessor directives
- `SourceExtractor::extract(const SourceNode&)` — Extract structured data

### Build Flags

- None (static library, standard C++ compilation)

---

## Status Notes

**Stable** — Core parsing is production-ready. New C++ features (modules, coroutines) may need grammar updates.
