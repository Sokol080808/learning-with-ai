add_exercise(
  NAME    m08_stl
  LABEL   08
  INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include
  SOURCES src/task08.cpp
  TESTS   tests/test_task08.cpp
)

add_exercise(
  NAME    m08_wordfreq
  LABEL   08
  INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/project/include
  SOURCES project/src/word_freq.cpp
  TESTS   project/tests/test_word_freq.cpp
)

add_executable(m08_wordfreq_app project/src/main.cpp project/src/word_freq.cpp)
target_include_directories(m08_wordfreq_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/project/include)
