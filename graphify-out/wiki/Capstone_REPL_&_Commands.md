# Capstone REPL & Commands

> 26 nodes · cohesion 0.09

## Key Concepts

- **TEST()** (10 connections) — `cpp/capstone/tests/test_command.cpp`
- **TEST()** (9 connections) — `cpp/capstone/tests/test_kvstore.cpp`
- **KvStore** (5 connections) — `cpp/capstone/src/command.cpp`
- **execute()** (5 connections) — `cpp/capstone/src/command.cpp`
- **Command** (4 connections) — `cpp/capstone/tests/test_command.cpp`
- **main.cpp** (3 connections) — `cpp/capstone/app/main.cpp`
- **test_command.cpp** (3 connections) — `cpp/capstone/tests/test_command.cpp`
- **main()** (2 connections) — `cpp/capstone/app/main.cpp`
- **command.cpp** (2 connections) — `cpp/capstone/src/command.cpp`
- **test_kvstore.cpp** (2 connections) — `cpp/capstone/tests/test_kvstore.cpp`
- **Count** (1 connections) — `cpp/capstone/tests/test_command.cpp`
- **string** (1 connections) — `cpp/capstone/src/command.cpp`
- **Del** (1 connections) — `cpp/capstone/tests/test_command.cpp`
- **Erase** (1 connections) — `cpp/capstone/tests/test_kvstore.cpp`
- **GetExistingAndMissing** (1 connections) — `cpp/capstone/tests/test_command.cpp`
- **Keys** (1 connections) — `cpp/capstone/tests/test_command.cpp`
- **KvStoreCore** (1 connections) — `cpp/capstone/tests/test_kvstore.cpp`
- **KvStorePersistence** (1 connections) — `cpp/capstone/tests/test_kvstore.cpp`
- **LoadMissingFileReturnsFalse** (1 connections) — `cpp/capstone/tests/test_kvstore.cpp`
- **OverwriteValue** (1 connections) — `cpp/capstone/tests/test_kvstore.cpp`
- **SaveThenLoad** (1 connections) — `cpp/capstone/tests/test_kvstore.cpp`
- **SetAndGet** (1 connections) — `cpp/capstone/tests/test_kvstore.cpp`
- **SetReturnsOk** (1 connections) — `cpp/capstone/tests/test_command.cpp`
- **SizeAndKeysSorted** (1 connections) — `cpp/capstone/tests/test_kvstore.cpp`
- **UnknownCommand** (1 connections) — `cpp/capstone/tests/test_command.cpp`
- *... and 1 more nodes in this community*

## Relationships

- [[Capstone KvStore Impl]] (1 shared connections)

## Source Files

- `cpp/capstone/app/main.cpp`
- `cpp/capstone/src/command.cpp`
- `cpp/capstone/tests/test_command.cpp`
- `cpp/capstone/tests/test_kvstore.cpp`

## Audit Trail

- EXTRACTED: 57 (93%)
- INFERRED: 4 (7%)
- AMBIGUOUS: 0 (0%)

---

*Part of the graphify knowledge wiki. See [[index]] to navigate.*