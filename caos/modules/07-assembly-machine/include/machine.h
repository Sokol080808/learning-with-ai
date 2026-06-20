#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* asm_a: вычисляет x*x + 1. */
int asm_a(int x);

/* asm_b: возвращает максимум из a и b. */
int asm_b(int a, int b);

/* asm_c: сумма целых от 1 до n (для n <= 0 сумма равна 0). */
int asm_c(int n);

/* ---------- Задание 4: чтение локальных переменных из кадра стека ---------- */

/*
 * Описание одного слота локальной переменной в кадре стека.
 *
 *  offset — смещение от базы кадра (rbp). Отрицательное: например, -8 означает
 *            [rbp - 8]. Используется для нахождения нужных байт в снимке.
 *  size   — размер переменной в байтах: 1, 2, 4 или 8.
 */
typedef struct {
    int    offset; /* смещение от rbp, например -8, -12, -20 */
    int    size;   /* размер в байтах: 1, 2, 4 или 8         */
} SlotInfo;

/*
 * Описание схемы кадра стека.
 *
 *  frame_size — полный размер снимка кадра в байтах (расстояние от rsp до rbp).
 *  num_slots  — количество слотов локальных переменных.
 *  slots      — массив SlotInfo длиной num_slots.
 */
typedef struct {
    int            frame_size; /* размер снимка в байтах          */
    int            num_slots;  /* количество слотов                */
    const SlotInfo *slots;     /* описание каждого слота           */
} FrameLayout;

/*
 * frame_read_local — прочитать значение локальной переменной из снимка кадра.
 *
 * layout — схема кадра стека.
 * frame  — указатель на начало снимка (соответствует rsp, наименьший адрес).
 *           Размер буфера равен layout->frame_size байт.
 * slot   — индекс слота в layout->slots (0-based).
 *
 * Возвращает значение переменной как int64_t со знаковым расширением.
 * Поведение не определено, если slot >= layout->num_slots или size не из {1,2,4,8}.
 */
int64_t frame_read_local(const FrameLayout *layout, const uint8_t *frame, int slot);

#ifdef __cplusplus
}
#endif
