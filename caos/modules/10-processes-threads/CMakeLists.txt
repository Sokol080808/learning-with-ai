add_exercise(
  NAME    caos10_threads
  LABEL   10
  INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include
  SOURCES src/threads.c
  TESTS   tests/test_threads.cpp
)
