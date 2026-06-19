# Header-only модуль: исходников (.cpp) нет, весь код — в include/concepts_ranges.hpp.
add_exercise(
  NAME    m17_concepts_ranges
  LABEL   17
  INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include
  TESTS   tests/test_concepts_ranges.cpp
)
