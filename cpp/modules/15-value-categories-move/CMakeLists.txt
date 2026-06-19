# Header-only модуль: исходников (.cpp) нет, весь код — в include/value_move.hpp.
add_exercise(
  NAME    m15_value_categories_move
  LABEL   15
  INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include
  TESTS   tests/test_value_move.cpp
)
