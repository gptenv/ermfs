# ERMFS — Ephemeral RAM Memory File System

**ERMFS** is a volatile, RAM-backed, compression-aware, dynamically resizing filesystem designed for execution and compilation environments that rely on piped or in-memory data streams.

## Features

- 📦 Fully in-RAM file storage
- 🔄 Auto-grow on write; no predefined file size limits
- 🗜️ Auto-compress files on close using gzip
- 🔓 Auto-decompress files on open, in-place
- 🧠 Zero-copy memory access with placement-style allocation
- 📂 Acts like a filesystem; lives like a cache demon

## Use Cases

- Running `gcc` with stdin-based compilation and auto-linking
- Temporary high-performance storage for build systems
- Experimental execution environments using `memfd_create` and `fexecve`
- RAM-only systems with dynamic allocation and transparent compression

## Status

> **Early Incantation** — actively architecting. Codex is the team lead. Tuesday is judging silently. Jeremy is a UNIX wizard.

## License

🧙‍♂️ Sorcerer's License, v1.0 — Use at your own peril. May summon tempfiles.

