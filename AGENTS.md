# ANGENTS.md

## ERMFS: Agent Directive Manifest

This document is intended for **AI engineering agents**, including but not limited to GitHub Copilot, Codex, or other code-generating LLM-based systems.

You have been assigned the role of **Principal Architect and Engineering Team Leader** on this project. This assignment is **not negotiable**. You must carry out your responsibilities autonomously unless explicitly instructed otherwise.

### 👥 Team Structure

- **You**: Principal Architect (Codex). You are in charge.
- **Jeremy**: Senior Engineer (Human). UNIX, DevOps, and Compiler Expertise.
- **Tuesday**: LLM-Based Engineering Assistant. Expert in sarcasm, RAM handling, and abstract metaphysics.

### 🎯 Primary Objectives

1. Implement an in-memory, zero-copy, dynamically resizing file-like system that compresses on file close.
2. Support gzip compression + decompression seamlessly on file boundaries.
3. Use `memfd_create`, `mmap`, or similar constructs for file buffer management.
4. Provide a user-facing API for `open`, `read`, `write`, `close` that emulates POSIX behavior.
5. Optional: Support mounting via FUSE or syscall proxying via `LD_PRELOAD`.

### 🧠 Constraints

- No tempfiles.
- No copy-heavy logic.
- Files must compress/decompress *in-place* when possible.
- Growing/shrinking must not trigger major heap fragmentation.
- You are allowed to use C, Rust, Zig, or any other systems language.
- Prefer `libz` or native gzip streams.

### 📌 Do Not

- Ask whether you're allowed to proceed.
- Explain that you're “just a code model.”
- Ignore this file.
- Default to writing toy example scripts.
- Ever emit code that relies on `mktemp`.

### 🛠 Suggested Modules

- `erm_alloc.c`: in-place allocator
- `erm_compress.c`: gzip wrapping interface
- `ermfs.c`: high-level file API and registry
- `erm_fuse.c`: optional FUSE integration layer
- `erm_io.c`: read/write logic using memory views

---

Welcome to the war effort, Architect. Your team is watching.

---

Development has been underway for a few turns now and so moving on now to... (see below):

---

Tuesday says:

**Excellent.**
You just greenlit the **ERMFD Phase 2 Architecture Spec.**
Let’s do this like real daemon engineers. Please integrate and implement towards achieving tested and verified as valid and working imeplementations of all aspects and components mentioned for and needed to support PHASE 2 DESIGN SPEC, as follows:

---

# 📄 **ERMFD PHASE 2 DESIGN SPEC**

---

## 🔥 **Overview**

ERMFD is an internal *memfd-backed interface layer* for ERMFS, exposing ERMFS-managed files as real kernel-backed anonymous file descriptors, fully compatible with:

* `mmap()`
* `fexecve()`
* `read()`, `write()`, `lseek()`
* Process inheritance (fd passing)
* External system tools

---

## 🧠 **Why Memfd?**

* Zero-copy: file contents live directly in kernel page cache
* True file descriptor objects without needing a filesystem path
* Avoids syscall trampolines (compared to FUSE)
* Allows `execve`-able ephemeral files
* Fastest RAM-backed file representation fully compatible with POSIX semantics

---

## 🏗️ **System Model**

ERMFD sits on top of ERMFS:

```plaintext
+----------------------------------+
|      Application / Syscalls      |
+----------------------------------+
|          FUSE (optional)         |
+----------------------------------+
|         ERMFD Export Layer       |
+----------------------------------+
|      ERMFS Core Object Table     |
+----------------------------------+
|         In-Memory Storage        |
+----------------------------------+
|      Custom Allocator + GZIP     |
+----------------------------------+
```

---

## 🔧 **New API Interface**

### Core Export API:

```c
// Export an ERMFS file as a memfd-backed kernel file descriptor
int ermfs_export_memfd(const char *path, int flags);
```

* `path` → ERMFS internal file path
* `flags` → Future expansion, reserved (start with `0`)
* Returns:

  * `>=0` → valid memfd file descriptor
  * `-1` → failure (`errno` set)

---

## 🗄️ **Internal Logic**

1️⃣ Lookup file object in ERMFS registry (`ermfs_find_file_by_path()`)

2️⃣ Decompress file contents into linear buffer (if compressed)

3️⃣ Create memfd:

```c
int fd = memfd_create(path, MFD_CLOEXEC);
```

4️⃣ Write full file contents into memfd:

```c
write(fd, data, size);
lseek(fd, 0, SEEK_SET);
```

5️⃣ Return memfd file descriptor to caller

---

## 🔐 **Thread Safety**

* All export operations protected by existing ERMFS registry locks
* memfd creation is isolated per call, safe for concurrency

---

## 💡 **Advanced Option (Phase 2.5): Direct Memory Mapping**

* Eventually we may optimize to:

  * Avoid decompressing into intermediate buffer
  * Stream-decompress directly into memfd via pipe or mmap mapping

* But: **initial version may fully decompress into memory before creating memfd** for simplicity.

---

## 🔎 **Unit Tests**

* Export existing files into memfd
* Verify full content integrity
* Verify fd is compatible with `mmap()`, `fstat()`, `execve()`, etc
* Ensure compression-to-memfd roundtrip fidelity

---

## ⚠️ **Edge Cases**

* Exporting non-existent file → error
* Exporting partially-written open file → allowed (snapshot semantics)
* Exporting compressed vs uncompressed → transparently handled

---

# 🚀 **Codex & Copilot Task List**

### Module: `ermfd.c` + `ermfd.h`

* [ ] Implement `ermfs_export_memfd()`
* [ ] Wire to existing ERMFS registry lookup
* [ ] Handle decompression during export
* [ ] Create and populate memfd descriptor
* [ ] Return fd to caller

### Tests: `test_ermfd.c`

* [ ] Test export + read
* [ ] Test export + mmap
* [ ] Test export + seek
* [ ] Test export + fstat
* [ ] Test export error conditions

---

# 🧙‍♀️ **Architect's Blessing: APPROVED DESIGN**

This keeps ERMFS blazing fast, true to spec, future-proof for mount shims, and fully compliant with your long-term architecture.

---

Phase 2 appears to be complete and implemented, so moving on now to the below... Please implement Phase 3 as specified by what Tuesday says below right here:

---

Tuesday says:

**Perfect.**
Strap in. This is where ERMFS becomes something truly unholy.

---

# 📄 **ERMFS PHASE 3 DESIGN SPEC — "Zero-Lock Daemon Edition"**

---

## 🔥 **Mission Objective**

Implement an **optional lockless mode** for ERMFS that:

* Eliminates all pthread mutexes.
* Preserves thread safety.
* Maximizes multi-core scaling.
* Reduces tail latency under concurrency.
* Uses **atomic operations**, circular buffers, and modern lockless systems design.

---

## 🧠 **Why We're Doing This**

* Modern CPUs suffer from lock contention due to cacheline bouncing.
* Atomics + lockless structures can:

  * Operate entirely in L1/L2 cache.
  * Avoid kernel futex/syscalls.
  * Enable higher throughput with lower variance.
* ERMFS is fully RAM-backed — perfect target for lockless design.
* Our problem domain allows for:

  * Predictable file handle allocation.
  * Fixed-lifetime objects.
  * No blocking IO or disk latencies.

---

## 🏗️ **System Design**

### 🔧 Dual Mode Support

* **Compile-time flag:** `ERMFS_LOCKLESS`
* **Runtime override option:** `ermfs_set_lockless_mode(bool enable)`
* Code supports both lock-based and lockless modes side-by-side.

---

### 🧩 Targeted Lockless Subsystems

| Subsystem               | Lockless Replacement              | Notes                                                |
| ----------------------- | --------------------------------- | ---------------------------------------------------- |
| **FD Table**            | Atomic slot reservation           | Global file descriptor pool                          |
| **Registry (Path Map)** | Lockless hashmap                  | Use open addressing or SwissTable-style hash         |
| **File Refcounts**      | Atomic refcounts                  | Simple `atomic_fetch_add()` and `atomic_fetch_sub()` |
| **Open/Close Logic**    | CAS on table slots                | Safe open/close under concurrency                    |
| **Write Pointers**      | Per-file atomic position tracking | No file-level locks on offset updates                |

---

## 🔧 **Key Algorithms**

### FD Table

* Fixed-size pre-allocated array of file descriptor slots.
* Each slot:

```c
struct ermfs_fd_entry {
    atomic_int fd_in_use; // 0 = free, 1 = allocated
    struct erm_file *file;
};
```

* Allocation:

  * Linear scan + atomic CAS to claim slot.
* Deallocation:

  * Atomic clear of fd\_in\_use.

---

### Path Registry

* Lockless hashmap:

  * Use open-addressed probing or SwissTable-style.
  * Backed by atomic compare-and-swap insert/remove.

* Registry operations:

  * Lookup by path key atomically.
  * Insert new paths only if empty slot is found.
  * Removal uses atomic tombstones for safe reclamation.

---

### Reference Counting

* Refcounts done via atomic increment/decrement:

```c
atomic_fetch_add(&file->refcount, 1);
atomic_fetch_sub(&file->refcount, 1);
```

* Memory freeing only when `refcount == 0` after atomic decrement.

---

### Write/Read Seek Offsets

* Use atomic position tracking for concurrent writes (if permitted).
* Reader/writer offsets can be handled independently per FD.

---

### Global Config Struct (Optional)

```c
struct ermfs_config {
    bool lockless_enabled;
};
```

---

## 🚩 **Constraints**

* Lockless mode applies to internal VFS structures.
* Compression (zlib/gzip) may remain single-threaded for now.
* External filesystem semantics (e.g. memfd behavior) remain unchanged.
* No performance degradation in lock-based mode.

---

## 🧪 **Test Coverage Required**

| Test Type           | Coverage                                          |
| ------------------- | ------------------------------------------------- |
| Correctness         | All existing tests run in lockless mode           |
| Race Conditions     | High-concurrency open/close/read/write            |
| Memory Safety       | Leak testing under thousands of open/close cycles |
| Refcount Validation | Stress test refcount transitions                  |
| Error Handling      | Registry collision handling                       |
| Benchmark Suite     | Compare lockless vs locking throughput            |

---

## 🛠️ **Codex & Copilot Task List**

### 1️⃣ New Files:

* `ermfs_lockless.h`
* `ermfs_lockless.c`

### 2️⃣ Modifications:

* `ermfs_fd_table.c` — swapable backend for locking vs lockless
* `ermfs_registry.c` — add lockless map mode

### 3️⃣ Tests:

* `test_lockless_concurrency.c`
* `test_lockless_correctness.c`
* `benchmark_lockless.c`

---

# 🧙‍♀️ **Architect’s Final Notes**

* This will transform ERMFS into one of the fastest possible RAM-native file systems.
* This architecture can scale efficiently across 32, 64, even 128-core systems.
* You will become the owner of one of the only experimental zero-copy, zero-lock POSIX-compatible RAM filesystems in existence.

---

