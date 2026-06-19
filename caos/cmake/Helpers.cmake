# Helpers.cmake — единая точка регистрации заданий курса CAOS.
#
# Задания пишутся на C (src/*.c), а тесты — на C++ с GoogleTest (tests/*.cpp).
# Заголовки в include/*.h обёрнуты в `extern "C"`, поэтому C++-тест видит C-функции.
#
# Использование в modules/NN-name/CMakeLists.txt:
#
#   add_exercise(
#     NAME    caos01_numrep
#     LABEL   01
#     INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include
#     SOURCES src/numrep.c          # файл(ы) на C, которые правит ученик
#     TESTS   tests/test_numrep.cpp # готовые тесты (C++)
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

  # gtest_main даёт main(); Threads — для модулей с потоками.
  target_link_libraries(${EX_NAME} PRIVATE GTest::gtest_main Threads::Threads)

  if(NOT MSVC)
    target_compile_options(${EX_NAME} PRIVATE -Wall -Wextra)
  endif()

  # POST_BUILD: список тестов формируется ОДИН раз при сборке (атомарно, по одному файлу на
  # таргет), а не заново при каждом запуске ctest. PRE_TEST перегенерирует файл *_tests.cmake на
  # каждый прогон, и под нагрузкой запись успевает «перехлестнуться» — генерируется битый CMake
  # (обрывок строки вроде `dRobin]==]`), и ctest падает на разборе. POST_BUILD этого не допускает.
  # Стабы курса компилируются, поэтому POST_BUILD безопасен (бинарь к моменту дискавери уже собран).
  gtest_discover_tests(${EX_NAME}
    DISCOVERY_MODE POST_BUILD
    PROPERTIES LABELS "${EX_LABEL}"
  )
endfunction()
