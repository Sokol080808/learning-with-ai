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

  # Threads::Threads — чтобы std::thread/std::mutex (модуль 14) линковались и на Linux (-pthread).
  target_link_libraries(${EX_NAME} PRIVATE GTest::gtest_main Threads::Threads)

  # Чуть строже к качеству кода — junior должен привыкать к предупреждениям.
  if(NOT MSVC)
    target_compile_options(${EX_NAME} PRIVATE -Wall -Wextra)
  endif()

  # POST_BUILD: список тестов формируется ОДИН раз при сборке (атомарно, по файлу на таргет).
  # PRE_TEST перегенерирует *_tests.cmake на каждый запуск ctest, и под нагрузкой запись успевает
  # «перехлестнуться» — получается битый CMake, и ctest падает на разборе. Стабы курса компилируются,
  # поэтому POST_BUILD безопасен (бинарь к моменту обнаружения тестов уже собран).
  gtest_discover_tests(${EX_NAME}
    DISCOVERY_MODE POST_BUILD
    PROPERTIES LABELS "${EX_LABEL}"
  )
endfunction()
