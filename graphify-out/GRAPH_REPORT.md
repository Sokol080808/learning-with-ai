# Graph Report - .  (2026-06-19)

## Corpus Check
- Corpus is ~17,970 words - fits in a single context window. You may not need a graph.

## Summary
- 522 nodes · 571 edges · 66 communities (43 shown, 23 thin omitted)
- Extraction: 90% EXTRACTED · 10% INFERRED · 0% AMBIGUOUS · INFERRED: 59 edges (avg confidence: 0.8)
- Token cost: 132,192 input · 0 output

## Community Hubs (Navigation)
- [[_COMMUNITY_Build, Functions & Class Basics|Build, Functions & Class Basics]]
- [[_COMMUNITY_Types, Pointers & CopyMove|Types, Pointers & Copy/Move]]
- [[_COMMUNITY_STL & Modern Idioms|STL & Modern Idioms]]
- [[_COMMUNITY_Capstone REPL & Commands|Capstone REPL & Commands]]
- [[_COMMUNITY_Templates Exercise|Templates Exercise]]
- [[_COMMUNITY_Modern Idioms Exercise|Modern Idioms Exercise]]
- [[_COMMUNITY_Fraction Class Implementation|Fraction Class Implementation]]
- [[_COMMUNITY_Error Handling Exercise|Error Handling Exercise]]
- [[_COMMUNITY_Smart Pointers Exercise|Smart Pointers Exercise]]
- [[_COMMUNITY_Word Frequency Project|Word Frequency Project]]
- [[_COMMUNITY_STL Algorithms Exercise|STL Algorithms Exercise]]
- [[_COMMUNITY_Data Structures Exercise|Data Structures Exercise]]
- [[_COMMUNITY_Tooling Bug-Fix Exercise|Tooling Bug-Fix Exercise]]
- [[_COMMUNITY_Control Flow Exercise|Control Flow Exercise]]
- [[_COMMUNITY_Basics Exercise|Basics Exercise]]
- [[_COMMUNITY_Pointers Exercise|Pointers Exercise]]
- [[_COMMUNITY_Capstone KvStore Interface|Capstone KvStore Interface]]
- [[_COMMUNITY_Tooling & Concurrency Concepts|Tooling & Concurrency Concepts]]
- [[_COMMUNITY_Concurrency Exercise|Concurrency Exercise]]
- [[_COMMUNITY_IntVector CopyMove Tests|IntVector Copy/Move Tests]]
- [[_COMMUNITY_Capstone KvStore Impl|Capstone KvStore Impl]]
- [[_COMMUNITY_CLI Calculator Project|CLI Calculator Project]]
- [[_COMMUNITY_Stack Template|Stack Template]]
- [[_COMMUNITY_Fraction Header|Fraction Header]]
- [[_COMMUNITY_IntList (smart-pointer list)|IntList (smart-pointer list)]]
- [[_COMMUNITY_Warmup Exercise|Warmup Exercise]]
- [[_COMMUNITY_IntVector Header|IntVector Header]]
- [[_COMMUNITY_Error Handling Concepts|Error Handling Concepts]]
- [[_COMMUNITY_IntVector Impl|IntVector Impl]]
- [[_COMMUNITY_Geometry Library Tests|Geometry Library Tests]]
- [[_COMMUNITY_Polygon Perimeter|Polygon Perimeter]]
- [[_COMMUNITY_Point Struct Header|Point Struct Header]]
- [[_COMMUNITY_Distance Function|Distance Function]]
- [[_COMMUNITY_run.sh Course Runner|run.sh Course Runner]]
- [[_COMMUNITY_stdatomicT|std::atomic<T>]]
- [[_COMMUNITY_Big-O|Big-O]]
- [[_COMMUNITY_Binary Search|Binary Search]]
- [[_COMMUNITY_constexpr|constexpr]]
- [[_COMMUNITY_STL Container|STL Container]]
- [[_COMMUNITY_Dangling Pointer|Dangling Pointer]]
- [[_COMMUNITY_Data Race|Data Race]]
- [[_COMMUNITY_Exception|Exception]]
- [[_COMMUNITY_Hash Table|Hash Table]]
- [[_COMMUNITY_Iterator|Iterator]]
- [[_COMMUNITY_Lambda|Lambda]]
- [[_COMMUNITY_stdmutex  stdlock_guard|std::mutex / std::lock_guard]]
- [[_COMMUNITY_Pointer (T)|Pointer (T*)]]
- [[_COMMUNITY_Ranges (C++20)|Ranges (C++20)]]
- [[_COMMUNITY_Sanitizer (ASanUBSan)|Sanitizer (ASan/UBSan)]]
- [[_COMMUNITY_STL Algorithm|STL Algorithm]]
- [[_COMMUNITY_Structured Bindings|Structured Bindings]]
- [[_COMMUNITY_stdthread|std::thread]]
- [[_COMMUNITY_stdvariantA,B|std::variant<A,B>]]
- [[_COMMUNITY_Consolidation Projects (no skeleton)|Consolidation Projects (no skeleton)]]
- [[_COMMUNITY_Junior C++ Interview Prep|Junior C++ Interview Prep]]

## God Nodes (most connected - your core abstractions)
1. `TEST()` - 12 edges
2. `TEST()` - 12 edges
3. `KvStore` - 11 edges
4. `TEST()` - 11 edges
5. `TEST()` - 11 edges
6. `TEST()` - 11 edges
7. `TEST()` - 11 edges
8. `TEST()` - 11 edges
9. `TEST()` - 10 edges
10. `TEST()` - 10 edges

## Surprising Connections (you probably didn't know these)
- `C++ Course (Setup to Junior)` --references--> `RAII (Resource Acquisition Is Initialization)`  [EXTRACTED]
  README.md → GLOSSARY.md
- `C++ Course (Setup to Junior)` --references--> `Translation Unit`  [EXTRACTED]
  README.md → GLOSSARY.md
- `C++ Course (Setup to Junior)` --references--> `Post-Course Junior Plan`  [EXTRACTED]
  README.md → NEXT_STEPS.md
- `C++ Course (Setup to Junior)` --references--> `Course Progress Tracker`  [EXTRACTED]
  README.md → PROGRESS.md
- `Compilation Pipeline (preprocess/compile/link)` --conceptually_related_to--> `Translation Unit`  [EXTRACTED]
  modules/00-setup/README.md → GLOSSARY.md

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **RAII Ownership and Lifetime Chain** — 04_classes_raii, 05_copy_move_big_five, 06_smart_pointers_unique_ptr, glossary_raii [INFERRED 0.85]
- **C++ Build Pipeline and Toolchain** — 00_setup_compilation_pipeline, glossary_linking, glossary_cmake, glossary_googletest, cmakelists_root_build [INFERRED 0.85]
- **Capstone Integrates Prior Modules** — capstone_kvstore_class, 04_classes_module, 05_copy_move_module, 02_control_flow_module, 07_templates_module [EXTRACTED 1.00]
- **STL Algorithm Workflow: Containers + Iterators + Lambdas + Algorithms** — 08_stl_containers, 08_stl_iterators, 08_stl_lambdas, 08_stl_algorithms [EXTRACTED 1.00]
- **Data Race Mitigation Strategies (mutex, atomic, work division)** — 14_concurrency_data_race, 14_concurrency_mutex, 14_concurrency_atomic, 14_concurrency_async [EXTRACTED 1.00]
- **Error Handling Toolset: Exceptions, optional, variant for different error kinds** — 09_error_handling_exceptions, 09_error_handling_optional, 09_error_handling_variant [EXTRACTED 1.00]

## Communities (66 total, 23 thin omitted)

### Community 0 - "Build, Functions & Class Basics"
Cohesion: 0.05
Nodes (43): Compilation Pipeline (preprocess/compile/link), Compile vs Link Errors, Module 00 - Setup and Build Pipeline, CLI Calculator Micro-project, Declaration vs Definition (hpp/cpp split), GCD (Euclid's algorithm), Module 02 - Control Flow and Functions, Function Overloading (+35 more)

### Community 1 - "Types, Pointers & Copy/Move"
Cohesion: 0.08
Nodes (33): Integer Division Pitfall, Module 01 - Types, const, References, Pass by Value vs Reference, const-correctness with Pointers, Dangling Pointer/Reference, Module 03 - Pointers and Memory Model, nullptr, Pointer Arithmetic and Raw Arrays (+25 more)

### Community 2 - "STL & Modern Idioms"
Cohesion: 0.10
Nodes (29): STL Algorithms (sort, count_if, find, accumulate, transform, copy_if), STL Containers (vector, array, map, unordered_map, set, deque, list), Iterators (begin/end, [begin, end) range), Lambdas (capture lists, [=], [&]), Module 08 — STL: Containers, Iterators, Algorithms, Lambdas, Prefer Algorithms over Manual Loops, Standard Template Library (STL), Word Frequency Dictionary Micro-Project (+21 more)

### Community 3 - "Capstone REPL & Commands"
Cohesion: 0.09
Nodes (22): Command, Count, main(), string, Del, Erase, GetExistingAndMissing, Keys (+14 more)

### Community 4 - "Templates Exercise"
Cohesion: 0.14
Nodes (18): ClampValue, T, vector, First, clamp_value(), is_power_of_two(), my_max(), Pair (+10 more)

### Community 5 - "Modern Idioms Exercise"
Cohesion: 0.13
Nodes (17): Color, ColorName, map, pair, string, vector, EvensSquared, square_ce() (+9 more)

### Community 6 - "Fraction Class Implementation"
Cohesion: 0.13
Nodes (15): string, Add, Multiply, Equality, Fraction, MovesSignToNumerator, NormalizesOnConstruction, add() (+7 more)

### Community 7 - "Error Handling Exercise"
Cohesion: 0.14
Nodes (16): optional, string, vector, Describe, ElementAt, ErrorHandling, FirstEven, SafeDivide (+8 more)

### Community 8 - "Smart Pointers Exercise"
Cohesion: 0.11
Nodes (14): Contains, size_t, unique_ptr, vector, EmptyToVector, IntList, MakeUniqueInt, PushFrontOrder (+6 more)

### Community 9 - "Word Frequency Project"
Cohesion: 0.15
Nodes (14): CaseSensitive, Counts, main(), map, pair, string, vector, HandlesNewlinesAndSpaces (+6 more)

### Community 10 - "STL Algorithms Exercise"
Cohesion: 0.15
Nodes (15): CharFrequency, CountGreater, map, string, vector, Evens, SortedDesc, Squared (+7 more)

### Community 11 - "Data Structures Exercise"
Cohesion: 0.16
Nodes (14): Algo, BinarySearch, optional, pair, vector, InsertionSort, PushTopPopSize, binary_search_idx() (+6 more)

### Community 12 - "Tooling Bug-Fix Exercise"
Cohesion: 0.18
Nodes (14): Average, Buggy, string, vector, HasDuplicate, MaxIn, Repeat, average() (+6 more)

### Community 13 - "Control Flow Exercise"
Cohesion: 0.14
Nodes (11): AreaOverloads, ControlFlow, string, Factorial, Fibonacci, FizzBuzz, Gcd, area() (+3 more)

### Community 14 - "Basics Exercise"
Cohesion: 0.18
Nodes (11): Average3, Basics, IsEven, MinOfThree, is_even(), min_of_three(), to_fahrenheit(), triple() (+3 more)

### Community 15 - "Pointers Exercise"
Cohesion: 0.19
Nodes (12): CountValue, MaxElementPtr, Pointers, ReverseInPlace, count_value(), max_element_ptr(), reverse_in_place(), sum_array() (+4 more)

### Community 16 - "Capstone KvStore Interface"
Cohesion: 0.18
Nodes (11): map, string, KvStore, data_, erase, get, keys, load (+3 more)

### Community 17 - "Tooling & Concurrency Concepts"
Cohesion: 0.23
Nodes (12): Find-and-Fix Bugs Exercise (buggy.cpp), Debugger (gdb: break, run, backtrace), Git Basics for Junior, Module 12 — Tooling and Code Quality, Sanitizers (ASan, UBSan), Formatting and Static Analysis (clang-format, clang-tidy), Work Division and std::async / std::future, std::atomic<T> (indivisible operations) (+4 more)

### Community 18 - "Concurrency Exercise"
Cohesion: 0.20
Nodes (10): AtomicCounter, Concurrency, vector, MutexCounter, ParallelSum, ParallelSumUneven, atomic_increment(), concurrent_increment() (+2 more)

### Community 19 - "IntVector Copy/Move Tests"
Cohesion: 0.17
Nodes (11): CopyAssignment, DeepCopyIndependent, EmptyByDefault, FillConstructor, IndexIsWritable, IntVector, MoveAssignment, MoveConstructorStealsBuffer (+3 more)

### Community 20 - "Capstone KvStore Impl"
Cohesion: 0.24
Nodes (11): optional, size_t, string, vector, erase(), get(), keys(), load() (+3 more)

### Community 21 - "CLI Calculator Project"
Cohesion: 0.20
Nodes (6): Calc, Add, Multiply, Divide, Subtract, TEST()

### Community 22 - "Stack Template"
Cohesion: 0.24
Nodes (5): size_t, T, vector, Stack, data_

### Community 23 - "Fraction Header"
Cohesion: 0.20
Nodes (9): Fraction, add, den_, denominator, multiply, num_, numerator, operator== (+1 more)

### Community 24 - "IntList (smart-pointer list)"
Cohesion: 0.22
Nodes (8): unique_ptr, IntList, contains, head_, push_front, size, to_vector, Node

### Community 25 - "Warmup Exercise"
Cohesion: 0.32
Nodes (5): AddSumsNumbers, SecondsInHours, seconds_in(), TEST(), Warmup

### Community 26 - "IntVector Header"
Cohesion: 0.25
Nodes (7): size_t, IntVector, capacity, data_, empty, push_back, size

### Community 27 - "Error Handling Concepts"
Cohesion: 0.38
Nodes (7): Exception Safety via RAII, Choosing Exception vs optional, Exceptions (throw / try / catch, std::exception hierarchy), Module 09 — Error Handling, noexcept Specifier, std::optional<T> (value or nothing), std::variant<A,B,...> (type-safe union, std::visit)

### Community 28 - "IntVector Impl"
Cohesion: 0.38
Nodes (4): size_t, capacity(), IntVector(), size()

### Community 29 - "Geometry Library Tests"
Cohesion: 0.29
Nodes (6): Distance, Geometry, PerimeterDegenerate, PerimeterSquare, PerimeterTriangle, TEST()

### Community 30 - "Polygon Perimeter"
Cohesion: 0.50
Nodes (3): Point, vector, perimeter()

### Community 31 - "Point Struct Header"
Cohesion: 0.50
Nodes (3): Point, x, y

## Knowledge Gaps
- **234 isolated node(s):** `set`, `get`, `erase`, `size`, `keys` (+229 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **23 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `KvStore class (Milestone 1)` connect `Build, Functions & Class Basics` to `Types, Pointers & Copy/Move`?**
  _High betweenness centrality (0.008) - this node is a cross-community bridge._
- **Why does `Module 05 - Copy, Move, Rule of 0/3/5` connect `Types, Pointers & Copy/Move` to `Build, Functions & Class Basics`?**
  _High betweenness centrality (0.006) - this node is a cross-community bridge._
- **Why does `C++ Course (Setup to Junior)` connect `Build, Functions & Class Basics` to `Types, Pointers & Copy/Move`?**
  _High betweenness centrality (0.006) - this node is a cross-community bridge._
- **Are the 5 inferred relationships involving `TEST()` (e.g. with `count_value()` and `max_element_ptr()`) actually correct?**
  _`TEST()` has 5 INFERRED edges - model-reasoned connections that need verification._
- **Are the 5 inferred relationships involving `TEST()` (e.g. with `square_ce()` and `color_name()`) actually correct?**
  _`TEST()` has 5 INFERRED edges - model-reasoned connections that need verification._
- **Are the 4 inferred relationships involving `TEST()` (e.g. with `is_even()` and `min_of_three()`) actually correct?**
  _`TEST()` has 4 INFERRED edges - model-reasoned connections that need verification._
- **What connects `set`, `get`, `erase` to the rest of the system?**
  _239 weakly-connected nodes found - possible documentation gaps or missing edges._