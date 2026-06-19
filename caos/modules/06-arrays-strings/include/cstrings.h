#pragma once
#include <stddef.h>   // size_t

#ifdef __cplusplus
extern "C" {
#endif

/* Длина си-строки s (без учёта завершающего '\0'). */
size_t my_strlen(const char* s);

/* Скопировать src в буфер dst размером cap байт.
 * Копирует не более cap-1 символов, ВСЕГДА завершает результат '\0'
 * (если cap > 0). Возвращает длину src (как my_strlen(src)) —
 * это позволяет узнать, было ли усечение: усечение, если результат >= cap. */
size_t safe_copy(char* dst, size_t cap, const char* src);

/* Развернуть строку s на месте (in-place), '\0' остаётся в конце. */
void reverse_string(char* s);

/* Сравнить две си-строки лексикографически, как strcmp:
 * <0, если a идёт раньше b; 0, если равны; >0, если a идёт позже b. */
int my_strcmp(const char* a, const char* b);

#ifdef __cplusplus
}
#endif
