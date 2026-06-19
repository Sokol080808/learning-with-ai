# Корневой CMake курса. Здесь решаются три задачи:
#   1) включить C++20 для всего проекта;
#   2) подтянуть GoogleTest (фреймворк, которым проверяются задания);
#   3) автоматически подключить все модули из modules/ и капстоун.
#
# Тебе НЕ нужно редактировать этот файл во время прохождения курса.
cmake_minimum_required(VERSION 3.20)
project(cpp_course LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

# Экспорт compile_commands.json — пригодится для clang-tidy / редактора (модуль 12).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Потоки (модуль 14 и др. используют std::thread; на Linux нужен -pthread, на macOS — нет).
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# --- GoogleTest через FetchContent (пин на конкретный релиз ради воспроизводимости) ---
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.15.2
)
# На Windows не подменять рантайм-настройки родителя.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

# Наш помощник add_exercise(...).
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(Helpers)

# --- Автоподключение всех модулей, у которых есть CMakeLists.txt ---
file(GLOB module_dirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/modules
     ${CMAKE_CURRENT_SOURCE_DIR}/modules/*)
list(SORT module_dirs)
foreach(d ${module_dirs})
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/modules/${d}/CMakeLists.txt)
    add_subdirectory(modules/${d})
  endif()
endforeach()

# Капстоун (если уже начат).
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/capstone/CMakeLists.txt)
  add_subdirectory(capstone)
endif()
