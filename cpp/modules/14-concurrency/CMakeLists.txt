# Многопоточность требует линковки с системной библиотекой потоков.
find_package(Threads REQUIRED)

add_executable(m14_concurrency src/task14.cpp tests/test_task14.cpp)
target_include_directories(m14_concurrency PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(m14_concurrency PRIVATE GTest::gtest_main Threads::Threads)
if(NOT MSVC)
  target_compile_options(m14_concurrency PRIVATE -Wall -Wextra)
endif()
gtest_discover_tests(m14_concurrency DISCOVERY_MODE PRE_TEST PROPERTIES LABELS "14")
