# Package: saturn/exec

**Location:** `saturn/include/saturn/exec/`, `saturn/src/exec/`
**Type:** Part of saturn shared/static library
**Dependencies:** pieces (Result, coroutines), moodycamel queues (concurrentqueue)

---

## Purpose & Responsibility

### Owns

- Multi-threaded task execution (ThreadPool, ThreadWorker)
- Work-stealing task scheduler (global queue + per-worker queues)
- Async task futures with cancellation (TaskFuture<T>, TaskPromise<T>)
- Shared state synchronization (SharedState<T> with spin-then-wait)
- Worker affinity control (CPU core pinning)
- Worker sharing modes (exclusive, shared, steal policies)
- Future exception handling (FutureException, FutureErrorCode)
- Task graph execution (TaskGraph, TaskScheduler - **in development, stub files**)

### Does NOT Own

- Application lifecycle (core/ package)
- System registry (core/ package)
- ECS parallelization (users parallelize forEach externally)
- Platform-specific threading primitives (uses std::thread)
- Coroutine runtime (pieces/utils/coroutines.hpp provides Task<T>)

---

## Key Abstractions & Invariants

### Core Types

- **`ThreadPool`** (`thread_pool.hpp:71`) — Manages worker threads, global queue, task assignment (Pimpl, singleton)
- **`ThreadWorker`** (`thread_pool.hpp:60`) — Single worker thread with local queue (forward-declared)
- **`WorkerSharingMode`** (`thread_pool.hpp:26`) — Enum flags: allow_steal, accept_direct, accept_indirect, global_consumer
- **`TaskFuture<T>`** (`task_future.hpp`) — Future half of async task, supports cancellation, blocking wait
- **`TaskPromise<T>`** (`task_future.hpp`) — Promise half, sets value/exception
- **`SharedState<T>`** (`task_future.hpp:88`) — Shared promise/future state, spin-then-wait optimization
- **`FutureStatus`** (`task_future.hpp:24`) — Enum: pending, ready, executing, error, cancelled, consumed
- **`FutureErrorCode`** (`task_future.hpp:37`) — Error codes: no_state, promise_already_satisfied, broken_promise, etc.
- **`TaskGraph`** (`task_graph.hpp`) — **STUB FILE (in development)** — DAG-based task dependencies
- **`TaskScheduler`** (`task_scheduler.hpp`) — **STUB FILE (in development)** — Dependency-based execution

### Invariants (NEVER violate)

1. **One worker per logical CPU core**: ThreadPool MUST create workers equal to CPUInfo::logicalCoreCount (no over-subscription)
2. **Work-stealing semantics**: Workers with allow_steal flag MUST allow other workers to steal tasks (lock-free deque)
3. **Spin-then-wait**: SharedState MUST spin k_spinCount (100) times before blocking on condition variable (reduce context switching)
4. **Cache-line alignment**: SharedState mutex/cv MUST be cache-line aligned (SATURN_CACHE_LINE_SIZE) to avoid false sharing
5. **Future single-retrieve**: TaskFuture MUST be retrieved exactly once from TaskPromise (tryRetrieveFuture atomic CAS)
6. **Promise single-satisfy**: TaskPromise MUST set value/exception exactly once (throws promise_already_satisfied)
7. **Cancellation idempotency**: Calling cancel() multiple times MUST be safe (atomic flag)
8. **Status transitions**: FutureStatus MUST transition: pending → executing → ready/error/cancelled → consumed
9. **Exception safety**: TaskFuture::get() MUST rethrow exception_ptr if promise set exception
10. **Singleton ThreadPool**: ONLY one ThreadPool instance MUST exist (g_instance static member)

### Architectural Patterns

- **Work-stealing**: Workers steal from other workers' local queues when idle (Cilk-style)
- **Pimpl**: ThreadPool hides Impl details (workers, queues, affinity masks)
- **Promise/Future**: TaskPromise sets value, TaskFuture retrieves value (std::promise/std::future analog)
- **Spin-then-wait**: Optimize for low-latency by spinning before blocking (reduce syscall overhead)
- **Cache-line alignment**: Avoid false sharing between CPU cores (alignas(SATURN_CACHE_LINE_SIZE))
- **Singleton**: ThreadPool::g_instance for global task submission

---

## Architectural Constraints

### Dependency Rules

**Allowed:**

- pieces (Result, coroutines Task<T>)
- moodycamel (concurrentqueue for lock-free queues)
- STL (thread, mutex, condition_variable, atomic, variant)
- core/sys_info (CPUInfo for logical core count)
- tools/logger (logging)

**Forbidden:**

- ❌ core/application — ThreadPool independent of Application (Application may use ThreadPool)
- ❌ ecs/ — ECS does NOT depend on exec (users parallelize manually)
- ❌ graphics/ — Rendering does NOT automatically use ThreadPool
- ❌ Platform-specific threading — Use std::thread, not Win32/pthread directly

### Layering

- ThreadPool is utility layer → Application/systems may use it
- No upward dependencies on core/Application

### Threading Model

- **ThreadPool**: Thread-safe (multiple threads can enqueueToGlobal concurrently)
- **TaskFuture**: Thread-safe (multiple threads can wait(), check status)
- **TaskPromise**: Thread-safe (setValue/setException use mutex)
- **SharedState**: Thread-safe (atomic status, mutex-protected storage)
- **WorkerSharingMode**: Defines worker concurrency policy (exclusive vs shared)

### Lifetime & Ownership

- **ThreadPool**: Singleton (g_instance), initialized in application, shutdown before exit
- **TaskFuture**: Owned by caller, holds shared_ptr<SharedState<T>>
- **TaskPromise**: Owned by task executor, holds shared_ptr<SharedState<T>>
- **SharedState**: Shared ownership via shared_ptr (future + promise both hold reference)
- **ThreadWorker**: Owned by ThreadPool::Impl via unique_ptr

### Platform Constraints

- Cross-platform (uses std::thread, std::mutex, std::condition_variable)
- CPU affinity requires platform-specific calls (Win32 SetThreadAffinityMask, Linux pthread_setaffinity_np)
- Cache-line size platform-dependent (SATURN_CACHE_LINE_SIZE macro)

---

## Modification Rules

### Safe to Change

- Add new WorkerSharingMode flags (e.g., priority levels)
- Extend TaskFuture with continuation support (.then(), .andThen())
- Implement TaskGraph/TaskScheduler (currently stub files)
- Optimize work-stealing algorithm (chase-lev deque, bounded stealing)
- Add worker statistics (tasks executed, steal count)
- Improve spin-then-wait heuristics (adaptive spin count)

### Requires Coordination

- Changing SharedState layout affects TaskFuture/TaskPromise ABI
- Modifying WorkerSharingMode enum breaks existing presets (shared, exclusive, shared_no_steal)
- Altering FutureStatus transitions breaks user code checking status
- Removing singleton pattern requires Application to manage ThreadPool lifetime

### Almost Never Change

- **Spin-then-wait optimization** — removing spin increases latency (core performance feature)
- **Cache-line alignment** — removing causes false sharing (catastrophic perf regression)
- **Work-stealing semantics** — switching to non-stealing breaks load balancing
- **Promise/Future separation** — merging into single type breaks async pattern
- **One worker per core** — over-subscription degrades performance

---

## Common Pitfalls

### Footguns

- ⚠️ **Calling get() on cancelled future**: Throws FutureException (check status first)
- ⚠️ **Not checking enqueueToGlobal() return**: Returns nullopt if pool shutdown (always check optional)
- ⚠️ **Multiple get() calls**: get() consumes value, second call throws future_already_consumed
- ⚠️ **Destroying TaskPromise without satisfying**: Throws broken_promise on TaskFuture::get()
- ⚠️ **Infinite wait()**: If promise never satisfied, wait() blocks forever (use wait_for with timeout)
- ⚠️ **Recursive task submission in worker**: May deadlock if all workers block waiting (use continuation instead)

### Performance Traps

- 🐌 **Small tasks**: Task submission overhead dominates execution time (batch tasks or use inline execution)
- 🐌 **Frequent wait()**: Blocking on futures serializes execution (prefer fire-and-forget or continuation)
- 🐌 **Deep task graphs**: Stack overflow risk if tasks recursively submit sub-tasks (use iterative decomposition)
- 🐌 **Contended global queue**: All workers pushing to global queue causes lock contention (use worker-local queues)
- 🐌 **No work-stealing**: Exclusive workers may idle while others are overloaded (use shared presets)

### Historical Mistakes (Do NOT repeat)

- **Using std::future**: Switched to custom TaskFuture for cancellation support
- **No spin-then-wait**: Added spinning to reduce context switch overhead
- **Global queue only**: Added per-worker queues for work-stealing (reduced contention)
- **No cache-line alignment**: Added alignment to avoid false sharing (massive perf gain)

---

## How Claude Should Help

### Expected Tasks

- Implement TaskGraph and TaskScheduler (DAG-based task execution)
- Add continuation support to TaskFuture (.then(), .andThen(), .onError())
- Optimize work-stealing algorithm (chase-lev deque, randomized victim selection)
- Add worker statistics and profiling (task count, steal count, idle time)
- Implement task priority queues (high/medium/low priority lanes)
- Write unit tests for TaskFuture cancellation edge cases
- Add adaptive spin count heuristics (measure contention, adjust k_spinCount)
- Integrate with pieces/utils/coroutines.hpp (co_await TaskFuture<T>)

### Conservative Approach Required

- **Changing SharedState synchronization**: Risk of data races or deadlocks (use ThreadSanitizer)
- **Modifying work-stealing algorithm**: May break load balancing (benchmark before/after)
- **Altering FutureStatus state machine**: Breaks user code checking status
- **Removing spin-then-wait**: Increases latency (measure impact with benchmarks)
- **Changing worker count**: May over-subscribe or under-utilize cores

### Before Making Changes

- [ ] Run exec unit tests (`saturn/tests/unit/thread_pool_test.cpp`)
- [ ] Benchmark with varying task sizes (saturn/bench/ or create new bench)
- [ ] Test with ThreadSanitizer (detect data races)
- [ ] Verify CPU affinity works on Windows and Linux (platform-specific code)
- [ ] Check cancellation edge cases (cancel before/during/after execution)
- [ ] Measure spin-then-wait overhead vs latency (k_spinCount tuning)
- [ ] Test under heavy contention (all workers submitting tasks)

---

## Quick Reference

### Files

**Public API:**

- `include/saturn/exec/thread_pool.hpp` — ThreadPool, ThreadWorker, WorkerSharingMode
- `include/saturn/exec/task_future.hpp` — TaskFuture, TaskPromise, SharedState, FutureStatus
- `include/saturn/exec/task_graph.hpp` — **STUB (in development)**
- `include/saturn/exec/task_scheduler.hpp` — **STUB (in development)**

**Internal:**

- `src/exec/thread_pool.cpp` — ThreadPool::Impl implementation

**Tests:**

- `saturn/tests/unit/thread_pool_test.cpp` — ThreadPool, work-stealing, cancellation tests

**Benchmarks:**

- None currently (benchmarks needed for task submission overhead, work-stealing efficiency)

### Key Functions/Methods

- `ThreadPool::initialize(CPUInfo)` → RefResult<ThreadPool, string> — Create workers (one per core)
- `ThreadPool::enqueueToGlobal(fn, args...)` → optional<TaskFuture<T>> — Submit task to global queue
- `ThreadPool::shutdown()` — Stop workers, drain queues
- `TaskFuture::get()` → T — Blocks until ready, returns value (or throws exception)
- `TaskFuture::wait()` — Blocks until ready (no value retrieval)
- `TaskFuture::wait_for(duration)` → bool — Timed wait, returns true if ready
- `TaskFuture::cancel()` — Request cancellation (task checks via isCancellationRequested())
- `TaskPromise::setValue(val)` — Satisfy promise with value
- `TaskPromise::setException(ptr)` — Satisfy promise with exception

### Build Flags

- `SATURN_CACHE_LINE_SIZE` — Cache-line size for alignment (default: 64 bytes)

---

## Status Notes

**Stable with stubs** — ThreadPool and TaskFuture are production-ready. TaskGraph and TaskScheduler are stub files (in development - placeholders for DAG-based execution).
