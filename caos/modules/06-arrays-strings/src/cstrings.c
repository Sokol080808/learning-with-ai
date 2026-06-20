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

/* ------------------------------------------------------------------ *
 *  StrBuf — ограниченный строковый буфер фиксированной ёмкости.      *
 * ------------------------------------------------------------------ */

void strbuf_init(StrBuf* sb, size_t cap) {
    if (cap == 0) {
        sb->buf = NULL;
        sb->len = 0;
        sb->cap = 0;
        return;
    }
    sb->buf = (char*)malloc(cap);
    if (sb->buf == NULL) {
        sb->len = 0;
        sb->cap = 0;
        return;
    }
    sb->buf[0] = '\0';
    sb->len = 0;
    sb->cap = cap;
}

size_t strbuf_append(StrBuf* sb, const char* s) {
    if (sb->buf == NULL || sb->cap == 0) {
        return 0;
    }
    /* Сколько байт ещё влезает (не считая завершающий '\0'). */
    size_t free_space = (sb->cap - 1 > sb->len) ? (sb->cap - 1 - sb->len) : 0;
    if (free_space == 0) {
        return 0;
    }
    size_t copied = 0;
    while (s[copied] != '\0' && copied < free_space) {
        sb->buf[sb->len + copied] = s[copied];
        copied++;
    }
    sb->len += copied;
    sb->buf[sb->len] = '\0';
    return copied;
}

void strbuf_clear(StrBuf* sb) {
    if (sb->buf != NULL) {
        sb->buf[0] = '\0';
    }
    sb->len = 0;
}

void strbuf_free(StrBuf* sb) {
    free(sb->buf);
    sb->buf = NULL;
    sb->len = 0;
    sb->cap = 0;
}
