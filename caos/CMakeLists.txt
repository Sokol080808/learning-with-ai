# Корневой CMake курса CAOS (Computer Architecture & Operating Systems).
#   1) включить C11 (для заданий) и C++20 (для тестов GoogleTest);
#   2) подтянуть GoogleTest;
#   3) подключить pthread (модули про потоки/синхронизацию);
#   4) автоподключить модули из modules/ и капстоун.
# Тебе НЕ нужно редактировать этот файл во время прохождения курса.
cmake_minimum_required(VERSION 3.20)
project(caos_course LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Потоки (модули 10, 13).
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# --- GoogleTest через FetchContent (пин на релиз) ---
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.15.2
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(Helpers)

# --- Автоподключение модулей ---
file(GLOB module_dirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/modules
     ${CMAKE_CURRENT_SOURCE_DIR}/modules/*)
list(SORT module_dirs)
foreach(d ${module_dirs})
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/modules/${d}/CMakeLists.txt)
    add_subdirectory(modules/${d})
  endif()
endforeach()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/capstone/CMakeLists.txt)
  add_subdirectory(capstone)
endif()
