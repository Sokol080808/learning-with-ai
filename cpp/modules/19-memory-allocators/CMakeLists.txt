# Header-only модуль: исходников (.cpp) нет, весь код — в include/memory19.hpp.
add_exercise(
  NAME    m19_memory_allocators
  LABEL   19
  INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include
  TESTS   tests/test_memory19.cpp
)
