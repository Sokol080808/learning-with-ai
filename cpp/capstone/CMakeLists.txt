# CMakeLists.txt капстоуна — написан ЯВНО (без помощника add_exercise), потому что капстоун
# имитирует настоящий проект из модуля 11: библиотека + приложение + тесты. ПРОЧИТАЙ его —
# здесь нет магии, всё на «современном CMake» через таргеты и их свойства.

# Многопоточность (модуль 14) требует линковки с системной библиотекой потоков.
find_package(Threads REQUIRED)

# --- 1) Библиотека minidb_lib: вся логика из src/*.cpp ---
# Статическая библиотека, которую переиспользуют и приложение, и тесты (см. модуль 11).
add_library(minidb_lib STATIC
  src/value.cpp
  src/database.cpp
  src/command.cpp
)
# PUBLIC include: каталог заголовков виден всем, кто линкуется с minidb_lib.
target_include_directories(minidb_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
# Threads — PUBLIC: Database хранит mutex, потоки нужны и пользователям библиотеки.
target_link_libraries(minidb_lib PUBLIC Threads::Threads)
if(NOT MSVC)
  target_compile_options(minidb_lib PRIVATE -Wall -Wextra)
endif()

# --- 2) Приложение minidb_app: REPL ---
add_executable(minidb_app app/main.cpp)
target_link_libraries(minidb_app PRIVATE minidb_lib)
if(NOT MSVC)
  target_compile_options(minidb_app PRIVATE -Wall -Wextra)
endif()

# --- 3) Тесты minidb_tests: все файлы из tests/ в одном таргете ---
add_executable(minidb_tests
  tests/test_value.cpp
  tests/test_database.cpp
  tests/test_command.cpp
  tests/test_concurrency.cpp
  tests/test_cache.cpp        # СТРЕТЧ: header-only LRU Cache<K,V> (include/cache.hpp)
)
target_link_libraries(minidb_tests PRIVATE
  minidb_lib            # тестируемый код
  GTest::gtest_main     # фреймворк + main()
  Threads::Threads      # тесты сами поднимают std::thread
)
if(NOT MSVC)
  target_compile_options(minidb_tests PRIVATE -Wall -Wextra)
endif()

# Метка "capstone" — чтобы можно было гонять только капстоун: ctest -L capstone (и ./cpp/run.sh capstone).
# POST_BUILD — как в Helpers.cmake: список тестов формируется один раз при сборке (стабильно под нагрузкой).
gtest_discover_tests(minidb_tests
  DISCOVERY_MODE POST_BUILD
  PROPERTIES LABELS "capstone"
)
