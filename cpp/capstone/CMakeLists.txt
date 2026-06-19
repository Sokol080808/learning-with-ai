# Капстоун собран как настоящий проект: библиотека + приложение + тесты (модуль 11).

add_library(kvstore_lib STATIC
  src/kvstore.cpp
  src/command.cpp
)
target_include_directories(kvstore_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(NOT MSVC)
  target_compile_options(kvstore_lib PRIVATE -Wall -Wextra)
endif()

# REPL-приложение.
add_executable(kvstore_app app/main.cpp)
target_link_libraries(kvstore_app PRIVATE kvstore_lib)

# Приёмочные тесты (метка capstone).
add_executable(kvstore_tests tests/test_kvstore.cpp tests/test_command.cpp)
target_link_libraries(kvstore_tests PRIVATE kvstore_lib GTest::gtest_main)
gtest_discover_tests(kvstore_tests DISCOVERY_MODE PRE_TEST PROPERTIES LABELS "capstone")
