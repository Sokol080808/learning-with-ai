add_exercise(
  NAME    caos08_lrucache
  LABEL   08
  INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include
  SOURCES src/lrucache.c
  TESTS   tests/test_lrucache.cpp
)
