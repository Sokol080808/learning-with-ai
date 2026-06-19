# Header-only модуль: исходников (.cpp) нет, весь код шаблонов — в include/task07.hpp.
add_exercise(
  NAME    m07_templates
  LABEL   07
  INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include
  TESTS   tests/test_task07.cpp
)
