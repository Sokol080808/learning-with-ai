# Header-only модуль: исходников (.cpp) нет, весь код шаблонов — в include/task18.hpp.
add_exercise(
  NAME    m18_type_erasure
  LABEL   18
  INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include
  TESTS   tests/test_task18.cpp
)
