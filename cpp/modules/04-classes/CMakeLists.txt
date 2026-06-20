add_exercise(
  NAME    m04_fraction
  LABEL   04
  INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include
  SOURCES src/fraction.cpp
          src/shapes.cpp
  TESTS   tests/test_fraction.cpp
          tests/test_shapes.cpp
)
