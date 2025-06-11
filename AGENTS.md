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

