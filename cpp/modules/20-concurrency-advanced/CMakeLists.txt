# Header-only модуль: исходников (.cpp) нет, весь код — в include/task20.hpp.
add_exercise(
  NAME    m20_concurrency_advanced
  LABEL   20
  INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include
  TESTS   tests/test_task20.cpp
)
