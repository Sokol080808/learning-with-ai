add_exercise(
  NAME    caos13_syncbuf
  LABEL   13
  INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include
  SOURCES src/syncbuf.c
  TESTS   tests/test_syncbuf.cpp
)
