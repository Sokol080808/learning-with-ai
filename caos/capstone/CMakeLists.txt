# Капстоун курса CAOS — Tiny VM (стековая виртуальная машина).
#
# Здесь два таргета:
#   1) caos_vm  — приёмочные тесты (как в обычном модуле, через add_exercise).
#   2) vm_demo  — маленькое демо-приложение на C, которое запускает зашитую
#                 программу через vm_run и печатает результат.
#
# Тебе НЕ нужно редактировать этот файл: пиши свою реализацию в src/vm.c.

# --- Приёмочные тесты (gtest) ---
add_exercise(
  NAME    caos_vm
  LABEL   capstone
  INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include
  SOURCES src/vm.c
  TESTS   tests/test_vm.cpp
)

# --- Демо-приложение на C ---
# Запускается руками: после сборки ищи бинарь в каталоге build (например
#   ./caos/build/capstone/vm_demo). Он печатает результат зашитой программы.
add_executable(vm_demo app/main.c src/vm.c)
target_include_directories(vm_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(NOT MSVC)
  target_compile_options(vm_demo PRIVATE -Wall -Wextra)
endif()
