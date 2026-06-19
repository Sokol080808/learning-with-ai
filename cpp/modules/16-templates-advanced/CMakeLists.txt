# Header-only модуль: исходников (.cpp) нет, весь код шаблонов — в include/task16.hpp.
add_exercise(
  NAME    m16_templates_advanced
  LABEL   16
  INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include
  TESTS   tests/test_task16.cpp
)
