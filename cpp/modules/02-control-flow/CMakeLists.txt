# Основные задания модуля.
add_exercise(
  NAME    m02_functions
  LABEL   02
  INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include
  SOURCES src/task02.cpp
  TESTS   tests/test_task02.cpp
)

# Микро-проект: ядро калькулятора (тестируемая часть).
add_exercise(
  NAME    m02_calc
  LABEL   02
  INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/project/include
  SOURCES project/src/calc.cpp
  TESTS   project/tests/test_calc.cpp
)

# Микро-проект: само CLI-приложение (не тест, а запускаемая программа).
add_executable(m02_calc_app project/src/main.cpp project/src/calc.cpp)
target_include_directories(m02_calc_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/project/include)
