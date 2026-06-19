# Header-only модуль: исходников (.cpp) нет, весь код — в include/task22.hpp.
add_exercise(
  NAME    m22_exception_safety_constexpr
  LABEL   22
  INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include
  TESTS   tests/test_task22.cpp
)
