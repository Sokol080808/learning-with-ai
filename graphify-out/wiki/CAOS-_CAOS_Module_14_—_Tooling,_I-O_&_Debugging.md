# CAOS: CAOS Module 14 — Tooling, I/O & Debugging

> 18 nodes · cohesion 0.12

## Key Concepts

- **CAOS Module 14 — Tooling, I/O & Debugging** (9 connections) — `caos/modules/14-tooling/README.md`
- **File descriptors (fd: 0 stdin, 1 stdout, 2 stderr)** (3 connections) — `caos/modules/14-tooling/README.md`
- **Sanitizers (AddressSanitizer, -fsanitize=address)** (3 connections) — `caos/modules/14-tooling/README.md`
- **System calls (open/read/write)** (3 connections) — `caos/modules/14-tooling/README.md`
- **Rationale: CAOS add_exercise — each module a CMake target (C11 source + C++20 GoogleTest via extern "C")** (3 connections) — `caos/CMakeLists.txt`
- **Buggy exercise: fix 4 bugs in src/buggy.c (3 logical, 1 memory)** (2 connections) — `caos/modules/14-tooling/README.md`
- **Git for junior (status/diff/add/commit/log)** (2 connections) — `caos/modules/14-tooling/README.md`
- **CAOS capstone Tiny VM (caos_vm gtest target + vm_demo C executable)** (2 connections) — `caos/capstone/CMakeLists.txt`
- **CAOS build structure (root CMake: C11+C++20, GoogleTest FetchContent, pthread, auto-include modules)** (2 connections) — `caos/CMakeLists.txt`
- **Rationale: ask the tool, not your eyes — debug from facts** (1 connections) — `caos/modules/14-tooling/README.md`
- **gdb debugger (break/run/print/next/backtrace)** (1 connections) — `caos/modules/14-tooling/README.md`
- **Rationale: git as a time machine — experiment fearlessly with undo** (1 connections) — `caos/modules/14-tooling/README.md`
- **I/O buffering (printf buffers; stderr/fflush flush)** (1 connections) — `caos/modules/14-tooling/README.md`
- **perf profiler (perf stat/record/report)** (1 connections) — `caos/modules/14-tooling/README.md`
- **Platform note: Linux tools vs macOS (lldb, dtruss); sanitizers portable** (1 connections) — `caos/modules/14-tooling/README.md`
- **/proc/<pid>/ virtual filesystem (fd, maps, status)** (1 connections) — `caos/modules/14-tooling/README.md`
- **strace / ltrace (trace syscalls and library calls)** (1 connections) — `caos/modules/14-tooling/README.md`
- **valgrind (memory checking without recompiling)** (1 connections) — `caos/modules/14-tooling/README.md`

## Relationships

- No strong cross-community connections detected

## Source Files

- `caos/CMakeLists.txt`
- `caos/capstone/CMakeLists.txt`
- `caos/modules/14-tooling/README.md`

## Audit Trail

- EXTRACTED: 32 (84%)
- INFERRED: 6 (16%)
- AMBIGUOUS: 0 (0%)

---

*Part of the graphify knowledge wiki. See [[index]] to navigate.*