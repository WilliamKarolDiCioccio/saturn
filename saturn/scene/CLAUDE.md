# Package: saturn/scene

**Location:** `saturn/include/saturn/scene/`, `saturn/src/scene/`
**Type:** Part of saturn shared/static library (IN DEVELOPMENT - STUB FILES)
**Dependencies:** pieces (Result), ecs/ (EntityRegistry), core/ (System) - planned

---

## Purpose & Responsibility

### Owns (PLANNED - NOT YET IMPLEMENTED)

- Scene graph abstraction (hierarchical entity organization)
- Parent-child entity relationships (transform hierarchy)
- Scene management (create, load, save, switch scenes)
- Built-in components for scene graph (Transform, Name, Parent, Children - components deleted/in flux)
- Scene serialization/deserialization (JSON, binary)
- Entity prefab system (reusable entity templates)
- Scene lifecycle (activate, deactivate scenes)

### Does NOT Own

- ECS entity storage (ecs/ package owns EntityRegistry)
- Rendering scene (graphics/ handles rendering)
- Physics simulation (future physics/ package)
- Asset loading beyond scene files (future asset/ package)

---

## Key Abstractions & Invariants

### Core Types (PLANNED - NOT YET IMPLEMENTED)

- **`Scene`** (`scene.hpp`) — **STUB FILE** — Scene instance with EntityRegistry
- **`SceneSystem`** (`scene_system.hpp`) — **STUB FILE** — EngineSystem for scene management
- **Built-in components** — **DELETED** (`builtin_components.hpp` was deleted, needs redesign)

### Invariants (NEVER violate - FUTURE DESIGN)

1. **Scene owns EntityRegistry**: Each Scene MUST own its own EntityRegistry (scene-local entities)
2. **Hierarchy integrity**: Parent-child relationships MUST be acyclic (no circular hierarchies)
3. **Transform propagation**: Child transforms MUST be relative to parent (world transform computed)
4. **Scene isolation**: Entities MUST NOT reference entities in other scenes (scene boundaries)
5. **Prefab immutability**: Prefab templates MUST be read-only (instances can be modified)

### Architectural Patterns (PLANNED)

- **Scene graph**: Hierarchical entity organization via Parent/Children components
- **ECS integration**: Scene wraps EntityRegistry, components define hierarchy
- **Prefab system**: Reusable entity templates (similar to Unity prefabs)
- **Scene stacking**: Multiple scenes active simultaneously (main + overlay scenes)

---

## Architectural Constraints

### Dependency Rules

**Allowed (when implemented):**

- pieces (Result)
- ecs/ (EntityRegistry, components)
- core/ (System lifecycle)
- nlohmann-json (scene serialization)

**Forbidden:**

- ❌ graphics/ — Scene does NOT depend on rendering (graphics queries Scene for entities)
- ❌ Circular dependency with ECS — Scene uses EntityRegistry, ECS does NOT use Scene

### Layering (PLANNED)

- Scene wraps EntityRegistry (higher-level abstraction)
- SceneSystem manages active scenes (EngineSystem)
- User code creates entities in Scene via Scene::createEntity()

### Threading Model (PLANNED)

- **SceneSystem**: Single-threaded (main thread)
- **Scene**: Single-threaded (not thread-safe)
- **Serialization**: Can be offloaded to background threads (load/save async)

### Lifetime & Ownership (PLANNED)

- **SceneSystem**: Owned by Application (EngineSystem)
- **Scene**: Owned by SceneSystem
- **EntityRegistry**: Owned by Scene (one registry per scene)
- **Entities**: Owned by Scene's EntityRegistry

### Platform Constraints

- Platform-agnostic (header + .cpp implementation)

---

## Modification Rules

### Safe to Change

- **EVERYTHING** — Scene is greenfield (stub files only)
- Design Scene/SceneSystem API from scratch
- Design built-in components (Transform, Name, Parent, Children)
- Implement scene serialization format (JSON, MessagePack, binary)
- Add prefab system

### Requires Coordination

- Scene API design affects user code (coordinate with game developers)
- Built-in component design affects ECS (Transform, hierarchy components)
- SceneSystem integration affects Application lifecycle

### Almost Never Change (ONCE IMPLEMENTED)

- **Scene ownership of EntityRegistry** — Core design decision
- **Hierarchy via ECS components** — Switching to non-ECS hierarchy breaks design
- **Scene isolation** — Cross-scene entity references break scene independence

---

## Common Pitfalls

### Footguns (FUTURE)

- ⚠️ **Circular hierarchies**: Parent A → Child B → Child A (detect and reject)
- ⚠️ **Cross-scene entity references**: Entity in Scene A references entity in Scene B (breaks on scene unload)
- ⚠️ **Modifying prefab templates**: Prefabs should be read-only (instance overrides instead)
- ⚠️ **Destroying parent without children**: Orphaned entities (destroy children first or reparent)

### Performance Traps (FUTURE)

- 🐌 **Deep hierarchies**: Transform propagation is O(depth) (keep hierarchies shallow)
- 🐌 **Frequent reparenting**: Triggers transform recalculation (batch reparenting)
- 🐌 **Large scene serialization**: Synchronous save blocks frame (save async)

### Historical Mistakes (Do NOT repeat)

- **builtin_components.hpp deleted**: Previous built-in component design scrapped (needs redesign)

---

## How Claude Should Help

### Expected Tasks

- **Design Scene and SceneSystem API** (greenfield - start from scratch)
- **Implement built-in components** (Transform, Name, Parent, Children, Active)
- **Implement scene serialization** (JSON export/import of entities + components)
- **Design prefab system** (template entities, instance overrides)
- **Implement scene stacking** (multiple active scenes: main + UI overlay)
- **Add scene lifecycle callbacks** (onActivate, onDeactivate)
- **Implement transform hierarchy** (local transform, world transform computation)
- **Write tests** for scene graph (parent-child relationships, transform propagation)
- **Add scene editor integration** (future - scene editing in editor)

### Conservative Approach Required

- **Everything is greenfield** — No existing code to break
- **Coordinate with ECS design** — Scene uses EntityRegistry, don't break ECS abstractions
- **Align with Application lifecycle** — SceneSystem is EngineSystem, follow lifecycle pattern

### Before Making Changes

- [ ] **Design API first** — Scene is empty, start with clear API design
- [ ] **Coordinate with user code** — Scene API affects game developers
- [ ] **Design built-in components** — Transform, hierarchy components need careful design
- [ ] **Plan serialization format** — JSON vs binary vs MessagePack
- [ ] **Test hierarchy integrity** — Detect circular hierarchies, validate parent-child links
- [ ] **Benchmark transform propagation** — Ensure O(depth) acceptable for deep hierarchies

---

## Quick Reference

### Files

**Public API (STUB FILES):**

- `include/saturn/scene/scene.hpp` — **EMPTY (1 line stub)**
- `include/saturn/scene/scene_system.hpp` — **EMPTY (1 line stub)**
- `include/saturn/scene/builtin_components.hpp` — **DELETED (redesign needed)**

**Internal (STUB FILES):**

- `src/scene/scene.cpp` — **EMPTY (1 line stub)**
- `src/scene/scene_system.cpp` — **EMPTY (1 line stub)**

**Tests:**

- None (tests needed once implementation starts)

### Key Functions/Methods (PLANNED - NOT YET IMPLEMENTED)

- `Scene::createEntity<Ts...>(components...)` → EntityMeta — Create entity in scene
- `Scene::destroyEntity(entity)` — Destroy entity and children (if hierarchical)
- `Scene::save(filePath)` → Result<void, string> — Serialize scene to file
- `Scene::load(filePath)` → Result<Scene, string> — Deserialize scene from file
- `SceneSystem::createScene(name)` → Scene\* — Create new scene
- `SceneSystem::loadScene(filePath)` → Result<Scene\*, string> — Load scene from file
- `SceneSystem::setActiveScene(scene)` — Switch active scene

### Build Flags

- None

---

## Status Notes

**STUB FILES - IN DEVELOPMENT** — Scene graph is greenfield. Only stub files exist (1 line each, likely just `#pragma once`). builtin_components.hpp was deleted (previous design scrapped). Requires full implementation: API design, built-in components, serialization, prefab system, hierarchy. **Claude should be EXTREMELY conservative** - ask questions, clarify design, coordinate with user before implementing anything.
