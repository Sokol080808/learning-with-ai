# C++: C++ Capstone — MiniDB (Redis-lite in-memory

> 19 nodes

## Key Concepts

- **C++ Capstone — MiniDB (Redis-lite in-memory DB)** (9 connections) — `cpp/capstone/README.md`
- **Python Capstone — mini SQL database** (6 connections) — `python/capstone/README.md`
- **Rust Capstone — concurrent multi-type store (minidb)** (5 connections) — `rust/capstone/README.md`
- **Thread-safe access via std::mutex/lock_guard (module 14)** (3 connections) — `cpp/capstone/README.md`
- **Value via std::variant (string/list/hash)** (2 connections) — `cpp/capstone/README.md`
- **cpp MiniDB build (lib+app+tests, Threads, POST_BUILD)** (2 connections) — `cpp/capstone/CMakeLists.txt`
- **Value enum { Str, List, Hash } with type_name** (2 connections) — `rust/capstone/README.md`
- **SharedDb = Arc<Mutex<Database>> (Send+Sync, fearless concurrency)** (2 connections) — `rust/capstone/README.md`
- **Key TTL with lazy expiration and injected clock** (1 connections) — `cpp/capstone/README.md`
- **Snapshot persistence (save/load text format)** (1 connections) — `cpp/capstone/README.md`
- **Text command protocol execute() (Redis-like)** (1 connections) — `cpp/capstone/README.md`
- **MiniDB replaces simpler kvstore — integrates far more of the course** (1 connections) — `cpp/capstone/README.md`
- **Tokenizer -> parser -> engine three-layer pipeline** (1 connections) — `python/capstone/README.md`
- **In-memory tables with coerce typing** (1 connections) — `python/capstone/README.md`
- **CREATE/INSERT/SELECT(projection/WHERE/AND/ORDER BY)/DELETE** (1 connections) — `python/capstone/README.md`
- **QueryError — single user-facing error type (syntax vs semantics)** (1 connections) — `python/capstone/README.md`
- **Mini SQL DB replaces simpler todo capstone — integrates more of the course** (1 connections) — `python/capstone/README.md`
- **Result-returning typed ops (WRONGTYPE errors)** (1 connections) — `rust/capstone/README.md`
- **Rust minidb replaces simpler kvstore — integrates far more of the course** (1 connections) — `rust/capstone/README.md`

## Relationships

- No strong cross-community connections detected

## Source Files

- `cpp/capstone/CMakeLists.txt`
- `cpp/capstone/README.md`
- `python/capstone/README.md`
- `rust/capstone/README.md`

## Audit Trail

- EXTRACTED: 32 (76%)
- INFERRED: 10 (24%)
- AMBIGUOUS: 0 (0%)

---

*Part of the graphify knowledge wiki. See [[index]] to navigate.*