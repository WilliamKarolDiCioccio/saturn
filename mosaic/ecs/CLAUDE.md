# Package: saturn/ecs

**Location:** `saturn/include/saturn/ecs/`, `saturn/src/ecs/` (header-only)
**Type:** Header-only subsystem within saturn library
**Dependencies:** pieces (SparseSet, BitSet)

---

## Purpose & Responsibility

### Owns

- Entity lifecycle management (creation, destruction, component addition/removal)
- Archetype-based entity storage (entities grouped by component signature)
- Component registry (runtime component type registration)
- Entity-component queries (viewSubset, forEach iteration)
- Generational entity IDs (detect use-after-free)
- Type-erased component storage (TypelessSparseSet, TypelessVector)
- Cache-friendly iteration (archetype tables with tightly packed components)

### Does NOT Own

- Systems that operate on entities (user code or scene/ package)
- Component type definitions (user-defined POD structs)
- Multi-threaded iteration (exec/ package for parallel iteration)
- Serialization of entities/components (future feature)
- Scene graph hierarchy (scene/ package wraps ECS)

---

## Key Abstractions & Invariants

### Core Types

- **`EntityRegistry`** (`entity_registry.hpp:36`) — Main facade for entity/component management
- **`Archetype`** (`archetype.hpp:22`) — Groups entities with identical component signatures
- **`ComponentSignature`** (`component.hpp:14`) — Alias for pieces::BitSet (256-bit bitset)
- **`ComponentRegistry`** (`component_registry.hpp`) — Maps component types to IDs, tracks metadata
- **`EntityID`** (`entity.hpp`) — Alias for size_t, uniquely identifies entity (index into generations)
- **`EntityGen`** (`entity.hpp`) — Alias for uint32_t, generation count for EntityID
- **`EntityMeta`** (`entity.hpp`) — Pair of {EntityID, EntityGen} for generational indices
- **`TypelessSparseSet`** (`typeless_sparse_set.hpp`) — Type-erased sparse set wrapping pieces::SparseSet
- **`TypelessVector`** (`typeless_vector.hpp`) — Type-erased dense vector for archetype storage
- **`EntityView<Ts...>`** (`entity_registry.hpp:90`) — Non-owning view for querying entities with components Ts...

### Invariants (NEVER violate)

1. **Component concept**: Components MUST be trivially copyable, trivially destructible, not pointer/reference/const/volatile, standard layout (Component concept). Note: trivially copyable types CAN have non-trivial constructors.
2. **Archetype storage layout**: Each archetype row MUST be `[EntityMeta | Component1 | Component2 | ... | ComponentN]` (metadata first)
3. **Signature uniqueness**: No two archetypes MUST have identical ComponentSignature (registry deduplicates)
4. **Generational indices**: EntityGen MUST increment on entity destruction (never reuse same {ID, gen} pair)
5. **Type-unawareness**: EntityRegistry MUST NOT be templated on component types (runtime registration only)
6. **Cache-friendly packing**: Components within archetype MUST be contiguous in memory (TypelessSparseSet stride-based)
7. **O(1) guarantees**: insert(), remove(), get() MUST be O(1) via sparse set (page-based SparseSet)
8. **Swap-and-pop semantics**: remove() MUST swap-and-pop last entity in archetype (invalidates iteration order)
9. **ComponentID stability**: Once registered, ComponentID for type T MUST NEVER change (persistent across registry lifetime)
10. **No dynamic components**: Components cannot be added/removed during forEach iteration (undefined behavior)

### Architectural Patterns

- **Archetypal ECS**: Entities grouped by component signature, not tag-based
- **Type erasure**: TypelessSparseSet + TypelessVector enable runtime component registration
- **Generational indices**: EntityMeta = {EntityID, EntityGen} prevents dangling entity references
- **Swap-and-pop deletion**: remove() swaps with last entity, pops back (O(1) but reorders)
- **Cache-line optimization**: Components packed contiguously for data locality
- **Sparse set indexing**: O(1) entity lookup via page-based SparseSet (from pieces)

### Component Construction

Components can be constructed in two ways:

1. **Default construction** (no arguments):

   ```cpp
   auto entity = registry->createEntity<Position, Velocity>();
   // Components are default-initialized (indeterminate values)
   ```

2. **Constructor arguments** (via std::tuple):
   ```cpp
   auto entity = registry->createEntity<Position, Velocity>(
       std::make_tuple(10.0f, 20.0f, 30.0f),  // Position(x, y, z)
       std::make_tuple(1.0f, 2.0f, 3.0f)      // Velocity(dx, dy, dz)
   );
   ```

**Key points:**

- Constructor arguments are passed via `std::make_tuple` for each component
- Number of tuples MUST match number of component types (enforced at compile time via `requires` clause)
- Bulk operations use **shared arguments** for all entities (performance over flexibility)
- Components are constructed in EntityRegistry via placement new before memcpy to type-erased storage
- Type-erased storage (TypelessSparseSet/TypelessVector) remains memcpy-only (no constructor invocation)
- Backwards compatible: existing code without args continues to work

**Bulk construction with shared args:**

```cpp
// All 100 entities get Position(0, 0, 0)
auto entities = registry->createEntityBulk<Position>(100,
    std::make_tuple(0.0f, 0.0f, 0.0f)
);
```

**Adding components with args during archetype migration:**

```cpp
registry->addComponents<Velocity>(entityID,
    std::make_tuple(5.0f, 10.0f, 15.0f)
);
// Existing components preserved, new Velocity constructed with args
```

---

## Architectural Constraints

### Dependency Rules

**Allowed:**

- pieces (SparseSet, BitSet, Result)
- STL (vector, unordered_map, bitset, functional)

**Forbidden:**

- ❌ saturn core/ — ECS is lower-level than Application/System
- ❌ saturn exec/ — ECS does NOT handle threading (users parallelize forEach externally)
- ❌ saturn scene/ — Scene depends on ECS, not reverse
- ❌ External serialization libraries — ECS focuses on runtime storage

### Layering

- scene/ (if needed) wraps EntityRegistry for hierarchy
- User code creates components, queries entities via viewSubset<Ts...>
- ECS is foundation layer → no upward dependencies

### Threading Model

- **EntityRegistry**: NOT thread-safe (users serialize access)
- **forEach iteration**: Single-threaded (users parallelize via exec/ThreadPool if needed)
- **Component registration**: NOT thread-safe (register components during initialization only)
- **Archetype modification**: NOT thread-safe (no concurrent createEntity/destroyEntity)

### Lifetime & Ownership

- **EntityRegistry**: Owns all archetypes, component registry, entity allocator
- **Archetypes**: Owned by EntityRegistry via unique_ptr
- **Components**: Owned by archetype storage (TypelessSparseSet), copied on insertion
- **EntityView**: Non-owning view (holds Archetype\* pointers, invalidated on archetype modification)

### Platform Constraints

- Platform-agnostic (header-only, pure C++)
- No platform-specific component types

---

## Modification Rules

### Safe to Change

- Add new query methods (e.g., viewAll(), viewExcluding<Ts...>())
- Optimize TypelessSparseSet allocation strategy
- Add entity tagging system (bitset of tags)
- Extend ComponentMeta with additional fields (version, serialization hints)
- Add EntityView filtering predicates

### Requires Coordination

- Changing Component concept affects all user-defined components (verify all component types)
- Modifying Archetype storage layout breaks ECS iteration (retest all forEach loops)
- Altering ComponentSignature (switching from BitSet) invalidates archetype hashing
- Changing EntityMeta structure affects serialization (if added in future)

### Almost Never Change

- **Component concept** (trivially copyable/destructible) — removing constraint breaks type-erasure
- **Archetype storage layout** — [EntityMeta | Components...] order is load-bearing
- **Swap-and-pop semantics** — changing to stable deletion breaks O(1) guarantee
- **EntityID = size_t, EntityGen = uint32_t** — changing sizes breaks ABI compatibility
- **O(1) insert/remove guarantee** — switching from sparse set breaks performance contract

---

## Common Pitfalls

### Footguns

- ⚠️ **Destroying entity during forEach**: Invalidates archetype iterators (collect EntityIDs, destroy after iteration)
- ⚠️ **Adding components during forEach**: May trigger archetype migration mid-iteration (undefined behavior)
- ⚠️ **Dangling EntityMeta**: Destroyed entity's {ID, gen} is invalid (check isAlive() before use)
- ⚠️ **Non-trivial component types**: Components with destructors/copy constructors violate Component concept (compile error)
- ⚠️ **Pointer components**: Storing raw pointers breaks lifetime safety (use EntityID references instead)
- ⚠️ **Forgetting EntityMeta in archetype storage**: Archetype MUST include EntityMeta at offset 0 (layout invariant)

### Performance Traps

- 🐌 **Frequent component addition/removal**: Triggers archetype migration (copy all components, swap-and-pop old archetype)
- 🐌 **Large component types**: Large components reduce cache hits (split into smaller components)
- 🐌 **Deep component hierarchies**: Many archetypes fragment memory (prefer flat component sets)
- 🐌 **viewSubset with many component types**: Searches all archetypes for signature match (O(archetypes))
- 🐌 **Random entity access**: get(EntityID) is O(1) but cache-unfriendly (prefer forEach iteration)

### Historical Mistakes (Do NOT repeat)

- **Attempting non-POD components**: Broke type-erasure, reverted to trivially copyable constraint
- **Stable deletion without swap-and-pop**: Slowed remove() to O(n), switched back to swap-and-pop
- **Templating EntityRegistry on component types**: Lost runtime component registration, removed templates

---

## How Claude Should Help

### Expected Tasks

- Add new query methods (viewExcluding, viewOptional for nullable components)
- Optimize archetype search (cache component signature → archetype mapping)
- Implement entity serialization (JSON export/import of entities + components)
- Add parallel forEach via exec/ThreadPool integration
- Write benchmark tests for archetype migration performance
- Add entity tagging system (bitset tags for fast filtering)
- Implement component versioning (detect component changes)
- Add memory profiling (track archetype memory usage)

### Conservative Approach Required

- **Changing Component concept**: Verify ALL user components still compile (Position, Velocity, etc.)
- **Modifying Archetype storage layout**: Breaks existing forEach iteration (retest saturn/bench/ecs_bench.cpp)
- **Altering swap-and-pop semantics**: May break user code assuming entity destruction during iteration
- **Removing type-unawareness**: Breaks runtime component registration (plugins, scripting)

### Before Making Changes

- [ ] Verify Component concept constraints enforced at compile time (static_assert)
- [ ] Run ECS unit tests (`saturn/tests/unit/ecs_test.cpp`)
- [ ] Run ECS benchmarks (`saturn/bench/ecs_bench.cpp`) to detect performance regressions
- [ ] Test archetype migration (createEntity, addComponents, removeComponents)
- [ ] Verify swap-and-pop semantics preserved (remove() invalidates iteration order)
- [ ] Check generational index increment on entity destruction
- [ ] Confirm O(1) insert/remove/get guarantees maintained

---

## Quick Reference

### Files

**Public API (header-only):**

- `include/saturn/ecs/entity_registry.hpp` — EntityRegistry, EntityView, EntityAllocator
- `include/saturn/ecs/entity.hpp` — EntityID, EntityGen, EntityMeta
- `include/saturn/ecs/component.hpp` — ComponentID, ComponentSignature, Component concept, ComponentMeta
- `include/saturn/ecs/component_registry.hpp` — ComponentRegistry (runtime component registration)
- `include/saturn/ecs/archetype.hpp` — Archetype (component signature + storage)
- `include/saturn/ecs/typeless_sparse_set.hpp` — TypelessSparseSet (wraps pieces::SparseSet)
- `include/saturn/ecs/typeless_vector.hpp` — TypelessVector (dense type-erased storage)
- `include/saturn/ecs/helpers.hpp` — Utility functions

**Tests:**

- `saturn/tests/unit/ecs_test.cpp` — Entity lifecycle, component addition/removal
- `saturn/tests/unit/typeless_sparse_set_test.cpp` — TypelessSparseSet tests
- `saturn/tests/unit/typeless_vector_test.cpp` — TypelessVector tests

**Benchmarks:**

- `saturn/bench/ecs_bench.cpp` — Entity creation, component iteration, archetype migration

### Key Functions/Methods

- `EntityRegistry::createEntity<Ts...>()` → EntityMeta — Create entity with default-initialized components
- `EntityRegistry::createEntity<Ts...>(ArgTuples...)` → EntityMeta — Create entity with components initialized using constructor arguments (tuples)
- `EntityRegistry::createEntityBulk<Ts...>(count)` → vector<EntityMeta> — Create multiple entities with default-initialized components
- `EntityRegistry::createEntityBulk<Ts...>(count, ArgTuples...)` → vector<EntityMeta> — Create multiple entities with shared constructor arguments
- `EntityRegistry::destroyEntity(EntityID)` — Destroy entity, increment generation
- `EntityRegistry::addComponents<Ts...>(EntityID)` — Migrate entity to new archetype (default-initialized)
- `EntityRegistry::addComponents<Ts...>(EntityID, ArgTuples...)` — Migrate entity to new archetype with constructor arguments
- `EntityRegistry::removeComponents<Ts...>(EntityID)` — Migrate entity to archetype without components
- `EntityRegistry::getComponentsForEntity<Ts...>(EntityID)` → optional<tuple<Ts&...>> — Get component references
- `EntityRegistry::viewSubset<Ts...>()` → optional<EntityView<Ts...>> — Query entities with components
- `EntityView::forEach(fn)` — Iterate entities, call fn(EntityMeta, Ts&...)
- `ComponentRegistry::registerComponent<T>()` → ComponentID — Runtime component registration

### Build Flags

- None (header-only subsystem)

---

## Status Notes

**Stable** — Core ECS is production-ready. No .cpp files (header-only). Parallel iteration not implemented (users use exec/ThreadPool manually). Serialization not implemented.
