# Helpers.cmake — единая точка регистрации заданий курса.
#
# Зачем это нужно: чтобы каждый модуль не повторял один и тот же CMake-бойлерплейт,
# а просто говорил "вот задание: вот его исходники, вот его тесты, вот его метка".
# Так ты концентрируешься на C++, а не на сборке.
#
# Использование в modules/NN-name/CMakeLists.txt:
#
#   add_exercise(
#     NAME    m01_temperature      # уникальное имя таргета
#     LABEL   01                   # метка для ctest -L 01
#     INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include
#     SOURCES src/temperature.cpp  # файл(ы), которые правит ученик (можно опустить для header-only)
#     TESTS   tests/test_temperature.cpp
#   )

function(add_exercise)
  cmake_parse_arguments(EX "" "NAME;LABEL" "SOURCES;TESTS;INCLUDE" ${ARGN})

  if(NOT EX_NAME)
    message(FATAL_ERROR "add_exercise: не задан NAME")
  endif()
  if(NOT EX_TESTS)
    message(FATAL_ERROR "add_exercise(${EX_NAME}): не заданы TESTS")
  endif()

  add_executable(${EX_NAME} ${EX_SOURCES} ${EX_TESTS})

  if(EX_INCLUDE)
    target_include_directories(${EX_NAME} PRIVATE ${EX_INCLUDE})
  endif()

  target_link_libraries(${EX_NAME} PRIVATE GTest::gtest_main)

  # Чуть строже к качеству кода — junior должен привыкать к предупреждениям.
  if(NOT MSVC)
    target_compile_options(${EX_NAME} PRIVATE -Wall -Wextra)
  endif()

  # Обнаружение тестов откладываем до момента запуска (PRE_TEST), чтобы недописанные
  # стабы не мешали самой сборке проекта.
  gtest_discover_tests(${EX_NAME}
    DISCOVERY_MODE PRE_TEST
    PROPERTIES LABELS "${EX_LABEL}"
  )
endfunction()
