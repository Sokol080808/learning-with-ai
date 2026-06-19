#pragma once
// include/warmup.h — интерфейс модуля 00 на C.
// Обёртка extern "C" нужна, чтобы C++-тест (test_warmup.cpp) увидел эти
// функции под их «настоящими» C-именами, а не под искажёнными C++-именами.

#ifdef __cplusplus
extern "C" {
#endif

/* Возвращает сумму a и b. */
int add(int a, int b);

/* Возвращает число секунд в указанном количестве часов.
   Тип long — потому что секунд может быть много (часы * 3600). */
long seconds_in(int hours);

#ifdef __cplusplus
}
#endif
