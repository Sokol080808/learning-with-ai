# Header-only модуль: исходников (.cpp) нет, весь код шаблонов — в include/task21.hpp.
add_exercise(
  NAME    m21_implement_stl
  LABEL   21
  INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include
  TESTS   tests/test_task21.cpp
)
