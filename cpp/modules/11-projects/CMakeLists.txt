# Этот CMake написан ЯВНО (без нашего хелпера add_exercise), потому что модуль 11 —
# как раз про то, как устроена сборка. Прочитай его построчно.

# 1) Библиотека geometry из двух исходников. STATIC — статическая библиотека (.a).
add_library(m11_geometry STATIC
  src/shapes.cpp
  src/polygon.cpp
)
# PUBLIC: заголовки из include/ видны и самой библиотеке, и всем, кто с ней слинкуется.
target_include_directories(m11_geometry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(NOT MSVC)
  target_compile_options(m11_geometry PRIVATE -Wall -Wextra)
endif()

# 2) Приложение, использующее библиотеку.
add_executable(geometry_app app/main.cpp)
target_link_libraries(geometry_app PRIVATE m11_geometry)

# 3) Тесты: линкуются и с библиотекой, и с GoogleTest.
add_executable(m11_geometry_tests tests/test_geometry.cpp)
target_link_libraries(m11_geometry_tests PRIVATE m11_geometry GTest::gtest_main)
gtest_discover_tests(m11_geometry_tests DISCOVERY_MODE PRE_TEST PROPERTIES LABELS "11")
