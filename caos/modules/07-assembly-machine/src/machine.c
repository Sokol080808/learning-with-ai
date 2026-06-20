// ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 07 стали зелёными.
//
// В README есть три листинга ассемблера x86-64 (синтаксис Intel) — A, B и C.
// Прочитай разбор каждого построчно и перепиши то же поведение на C. Это и есть
// упражнение «от ассемблера к смыслу»: понять, ЧТО делают регистры и инструкции,
// и выразить это нормальным кодом.
//
// Сейчас все функции — заглушки (return 0), поэтому тесты падают. Это нормально:
// падающий тест — твоё задание.

#include "machine.h"

// Контракт: вернуть x*x + 1.
// (Листинг A: imul умножает x на себя, потом add прибавляет 1.)
int asm_a(int x) {
    return x * x + 1;
}

// Контракт: вернуть максимум из a и b.
// (Листинг B: cmp сравнивает a и b, условный переход выбирает большее.)
int asm_b(int a, int b) {
    int result = a;          // mov eax, edi: оптимистично считаем максимумом a
    if (b > a) {             // cmp edi, esi / jge .done: прыжок, если a >= b
        result = b;          // mov eax, esi: иначе ответ — b
    }
    return result;
}

// Контракт: вернуть сумму 1 + 2 + ... + n. Для n <= 0 вернуть 0.
// (Листинг C: цикл с аккумулятором — на каждом витке add прибавляет счётчик.)
int asm_c(int n) {
    int sum = 0;                     // mov eax, 0: накопитель суммы
    for (int i = 1; i <= n; i++) {   // mov ecx, 1 / cmp ecx, edi / jg .done
        sum += i;                    // add eax, ecx
    }                                // add ecx, 1 / jmp .loop
    return sum;
}

/* ---------- Задание 4: чтение локальных переменных из кадра стека ----------
 *
 * Модель памяти кадра:
 *
 *   Высокие адреса (rbp) = frame + frame_size
 *   Низкие адреса  (rsp) = frame
 *
 * Слот с offset = -8 лежит по адресу rbp + offset = (frame + frame_size) - 8.
 * Так как offset отрицателен, итоговый указатель внутри буфера:
 *   ptr = frame + frame_size + offset   (offset < 0 → адрес левее rbp)
 *
 * Знаковое расширение выполняется автоматически при присвоении значения
 * знакового типа (int8_t / int16_t / int32_t / int64_t) в int64_t.
 */
#include <string.h>   /* memcpy */

int64_t frame_read_local(const FrameLayout *layout, const uint8_t *frame, int slot) {
    const SlotInfo *s = &layout->slots[slot];
    /* Адрес начала слота внутри снимка: rbp находится в конце буфера */
    const uint8_t *ptr = frame + layout->frame_size + s->offset;

    int64_t result = 0;
    switch (s->size) {
        case 1: {
            int8_t v;
            memcpy(&v, ptr, 1);
            result = (int64_t)v;
            break;
        }
        case 2: {
            int16_t v;
            memcpy(&v, ptr, 2);
            result = (int64_t)v;
            break;
        }
        case 4: {
            int32_t v;
            memcpy(&v, ptr, 4);
            result = (int64_t)v;
            break;
        }
        case 8: {
            int64_t v;
            memcpy(&v, ptr, 8);
            result = v;
            break;
        }
        default:
            result = 0;
            break;
    }
    return result;
}
