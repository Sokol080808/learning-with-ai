// ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 06 стали зелёными.
//
// Сейчас все функции — ЗАГЛУШКИ: они компилируются, но дают неверный результат,
// поэтому тесты падают. Это нормально. Твоя задача — заменить заглушки на настоящую
// логику. Готовых решений тут нет — только подсказки в README.
//
// Помни главное правило этого модуля: си-строка — это массив char, оканчивающийся
// нулевым байтом '\0'. Ходи по ней, пока не встретишь '\0'. И НИКОГДА не пиши за
// границы буфера.

#include "cstrings.h"

// Длина строки: число символов до завершающего '\0' (сам '\0' не считается).
size_t my_strlen(const char* s) {
    size_t len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}

// Безопасное копирование. dst — буфер на cap байт. Скопируй не более cap-1
// символов из src, затем обязательно поставь '\0' (если cap > 0).
// Верни длину src (сколько символов в src на самом деле).
size_t safe_copy(char* dst, size_t cap, const char* src) {
    size_t src_len = my_strlen(src);
    if (cap == 0) {
        return src_len;
    }
    size_t written = 0;
    while (src[written] != '\0' && written < cap - 1) {
        dst[written] = src[written];
        written++;
    }
    dst[written] = '\0';
    return src_len;
}

// Развернуть строку на месте: "abc" -> "cba". Завершающий '\0' не трогаем.
void reverse_string(char* s) {
    size_t n = my_strlen(s);
    if (n == 0) {
        return;
    }
    size_t i = 0;
    size_t j = n - 1;
    while (i < j) {
        char tmp = s[i];
        s[i] = s[j];
        s[j] = tmp;
        i++;
        j--;
    }
}

// Сравнение строк, как strcmp: отрицательное / 0 / положительное.
int my_strcmp(const char* a, const char* b) {
    size_t i = 0;
    while (a[i] != '\0' && a[i] == b[i]) {
        i++;
    }
    return (int)(unsigned char)a[i] - (int)(unsigned char)b[i];
}
