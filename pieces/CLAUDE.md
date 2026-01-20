# Package: pieces

**Location:** `pieces/`
**Type:** Header-only library
**Dependencies:** None (utf8cpp only for string utilities)

---

## Purpose & Responsibility

### Owns

- Type-erased containers (SparseSet, TypelessVector, Bitset, CircularBuffer, ConstexprMap, SPMC snapshot buffer)
- Memory allocators (PoolAllocator, ContiguousAllocator, ProxyAllocator, BaseAllocator interface)
- Railway-Oriented Programming via Result<T, E> and RefResult<T, E>
- C++20 coroutine utilities (Task<T>, Promise types)
- String utilities (UTF-8 conversion, string_view helpers)
- SIMD intrinsics wrappers
- Cache-line alignment utilities
- Enum flag operations

### Does NOT Own

- Any saturn-specific abstractions (ECS, rendering, platform, input, etc.)
- Concrete implementations with .cpp files
- External dependencies beyond utf8cpp
- Non-header-only code

---

## Key Abstractions & Invariants

### Core Types

- **`Result<T, E>`** (`core/result.hpp`) — Fallible operation result with monadic composition (.andThen, .map, .mapErr)
- **`RefResult<T, E>`** (`core/result.hpp`) — Result holding reference_wrapper for non-copyable types
- **`SparseSet<K, T, PageSize, AggressiveReclaim>`** (`containers/sparse_set.hpp`) — O(1) insert/delete/lookup with page-based sparse storage
- **`PoolAllocator<T, Policy>`** (`memory/pool_allocator.hpp`) — Fixed-capacity object pool with BitSet tracking
- **`ContiguousAllocator<T>`** (`memory/contiguous_allocator.hpp`) — Contiguous memory allocator with linear growth
- **`BitSet`** (`containers/bitset.hpp`) — Dynamic bitset with efficient page management
- **`CircularBuffer<T>`** (`containers/circular_buffer.hpp`) — Lock-free SPSC ring buffer
- **`ConstexprMap<K, V, N>`** (`containers/constexpr_map.hpp`) — Compile-time associative array
- **`SPMCSnapshotBuffer<T>`** (`containers/spmc_snapshot_buffer.hpp`) — Single-producer, multi-consumer lock-free buffer
- **`Task<T>`** (`utils/coroutines.hpp`) — C++20 coroutine wrapper for async operations

### Invariants (NEVER violate)

1. **Header-only constraint**: NEVER add .cpp files to pieces (must remain interface-only)
2. **Zero saturn dependencies**: NEVER include saturn headers or depend on saturn types
3. **Trivial destruction**: PoolAllocator NEVER supports non-trivially-destructible types (performance-critical)
4. **Unsigned integral keys**: SparseSet keys MUST be unsigned integral types (K constraint)
5. **Page alignment**: SparseSet PageSize MUST be > 0 (compile-time requirement)
6. **Result invariant**: Result<T, E> union MUST maintain isError flag consistency with active variant
7. **Allocator capacity**: PoolAllocator capacity MUST be non-zero (constructor validation)
8. **Lock-free guarantee**: CircularBuffer and SPMCSnapshotBuffer MUST remain lock-free (no mutex)

### Architectural Patterns

- **Railway-Oriented Programming**: Result<T, E> composition via .andThen/.map/.mapErr
- **CRTP**: NonCopyable<T>, NonMovable<T> for static polymorphism
- **Type-erased containers**: SparseSet + TypelessVector enable runtime type handling
- **Page-based storage**: SparseSet uses page allocations to amortize sparse memory cost
- **Bitset presence tracking**: Efficient empty page detection via std::bitset

---

## Architectural Constraints

### Dependency Rules

**Allowed:**

- C++ standard library (C++23)
- utf8cpp (for string.hpp only)

**Forbidden:**

- ❌ saturn headers — pieces is foundational, no upward dependencies
- ❌ External libraries beyond utf8cpp — keep zero-dependency promise
- ❌ Platform-specific code — must be cross-platform

### Layering

- pieces is bottom layer → saturn depends on pieces, NEVER reverse
- Internal headers (`internal/`) MUST NOT be included by users directly

### Threading Model

- **CircularBuffer**: Single-producer, single-consumer (lock-free)
- **SPMCSnapshotBuffer**: Single-producer, multi-consumer (lock-free)
- **Result/allocators**: Thread-compatible (not thread-safe by design - users handle synchronization)
- **Coroutines**: No thread safety guarantees (caller-managed)

### Lifetime & Ownership

- **PoolAllocator**: Owns buffer, users MUST manually deallocate objects before destruction
- **Result<T, E>**: Owns T or E (union storage), destructor handles cleanup
- **SparseSet**: Owns dense keys/values, pages managed via unique_ptr
- **Allocators**: RAII for memory, but do NOT construct/destruct user objects automatically

### Platform Constraints

- Must compile on: Windows (MSVC), Linux (GCC/Clang), Android (Clang), Emscripten (Clang)
- SIMD intrinsics (simd.hpp) MUST gracefully degrade on unsupported platforms

---

## Modification Rules

### Safe to Change

- Add new containers following existing patterns (template header-only)
- Extend Result<T, E> with new monadic operations (.orElse, .inspect, etc.)
- Add new allocator types inheriting BaseAllocator
- Improve SIMD intrinsic coverage
- Add string utility functions

### Requires Coordination

- Changing Result<T, E> API affects all saturn error handling (verify usages)
- Modifying SparseSet interface impacts saturn ECS (TypelessSparseSet wraps this)
- Changing PoolAllocator capacity semantics affects ECS archetype allocations
- Altering coroutine Task<T> affects saturn exec layer

### Almost Never Change

- **Result<T, E> core API** (isOk, isErr, value, error) — load-bearing across entire codebase
- **SparseSet O(1) guarantee** — ECS performance depends on this
- **PoolAllocator trivial-destruction constraint** — removing this breaks ECS component storage
- **Header-only nature** — switching to compiled library breaks saturn's build model

---

## Common Pitfalls

### Footguns

- ⚠️ **PoolAllocator manual deallocation**: Users MUST call deallocate() before destructor or leak slots
- ⚠️ **Result unwrapping without check**: Calling .value() on Err or .error() on Ok throws/UB
- ⚠️ **SparseSet key reuse**: Erasing key K, then inserting K again reuses dense index (swap-and-pop)
- ⚠️ **CircularBuffer overflow**: Pushing to full buffer overwrites oldest data silently
- ⚠️ **Non-trivial types in PoolAllocator**: Compile error if T has non-trivial destructor

### Performance Traps

- 🐌 **Sparse SparseSet**: Very sparse keys (e.g., {0, 1000000}) allocate many empty pages unless AggressiveReclaim=true
- 🐌 **Result chaining overhead**: Deep .andThen chains may hinder inlining (keep chains shallow)
- 🐌 **BitSet dynamic growth**: Growing BitSet reallocates pages (reserve capacity upfront)
- 🐌 **Coroutine heap allocation**: Task<T> may allocate on heap (use std::pmr or custom allocators if needed)

### Historical Mistakes (Do NOT repeat)

- **Attempting to add .cpp files**: pieces must remain header-only for dependency management
- **Introducing saturn dependencies**: Broke pieces' reusability as standalone library
- **Adding mutex to lock-free containers**: Defeated performance benefits, removed

---

## How Claude Should Help

### Expected Tasks

- Add new container types (stack, queue, intrusive list, etc.)
- Extend Result<T, E> monadic operations following Rust/F# patterns
- Implement new allocator strategies (buddy allocator, slab allocator)
- Add utility functions to string.hpp (parsing, formatting)
- Write unit tests for new abstractions
- Optimize existing containers (SIMD, cache-line alignment)
- Add C++23 features (std::expected integration, ranges support)

### Conservative Approach Required

- **Changing Result<T, E> core API**: Verify all 50+ usages across saturn before modifying
- **Modifying SparseSet internals**: ECS depends on O(1) guarantees and swap-and-pop semantics
- **Breaking header-only constraint**: Discuss architectural impact before adding .cpp files
- **Introducing dependencies**: Must justify any new external dependency (default: reject)

### Before Making Changes

- [ ] Verify header-only constraint maintained (no .cpp files)
- [ ] Confirm no saturn dependencies introduced (include scan)
- [ ] Check cross-platform compatibility (Windows/Linux/Android/Emscripten)
- [ ] Run pieces unit tests (`pieces/tests/unit/`)
- [ ] Run pieces benchmarks if performance-critical (`pieces/bench/`)
- [ ] Validate C++23 concepts/constraints compile correctly

---

## Quick Reference

### Files

**Public API:**

- `core/result.hpp` — Result<T, E>, RefResult<T, E>
- `core/templates.hpp` — NonCopyable, NonMovable, CRTP helpers
- `containers/sparse_set.hpp` — SparseSet<K, T, PageSize, AggressiveReclaim>
- `containers/bitset.hpp` — Dynamic BitSet
- `containers/circular_buffer.hpp` — Lock-free SPSC CircularBuffer
- `containers/constexpr_map.hpp` — Compile-time ConstexprMap
- `containers/spmc_snapshot_buffer.hpp` — Lock-free SPMC buffer
- `memory/pool_allocator.hpp` — PoolAllocator<T, Policy>
- `memory/contiguous_allocator.hpp` — ContiguousAllocator<T>
- `memory/proxy_allocator.hpp` — ProxyAllocator wrapper
- `memory/base_allocator.hpp` — BaseAllocator interface
- `utils/coroutines.hpp` — Task<T>, Promise types
- `utils/string.hpp` — UTF-8 conversion, string utilities
- `utils/enum_flags.hpp` — Enum bitwise operators
- `intrinsics/simd.hpp` — SIMD wrappers (SSE, AVX, NEON)

**Internal:**

- `internal/error_codes.hpp` — Error code definitions

**Tests:**

- `tests/unit/allocators_test.cpp`
- `tests/unit/bitset_test.cpp`
- `tests/unit/coroutines_test.cpp`
- `tests/unit/result_test.cpp`
- `tests/unit/sparse_set_test.cpp`
- `tests/unit/spmc_snapshot_buffer_test.cpp`

**Benchmarks:**

- `bench/sparse_vs_map_bench.cpp`

### Key Functions/Methods

- `Result<T, E>::andThen(F)` — Monadic bind (railway chaining)
- `SparseSet<K, T>::insert(K, T)` — O(1) insertion, updates if exists
- `SparseSet<K, T>::erase(K)` — O(1) deletion via swap-and-pop
- `PoolAllocator<T>::allocate(index?)` — Allocates slot, returns T\*
- `PoolAllocator<T>::deallocate(index)` — Frees slot for reuse

### Build Flags

- None (header-only library)

---

## Status Notes

**Stable** — Core abstractions are production-ready and widely used across saturn. API changes require careful coordination.
