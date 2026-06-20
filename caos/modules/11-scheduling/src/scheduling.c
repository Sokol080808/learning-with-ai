// ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 11 стали зелёными.
//
// Подключаем СВОЙ заголовок — так компилятор сверит, что твои реализации
// совпадают по сигнатуре с объявлениями в include/scheduling.h.
#include "scheduling.h"
#include <stdlib.h>

// Контракт: среднее время ожидания при FCFS.
// burst[i] — длительность процесса i; все пришли в момент 0, считаются по порядку.
// Время ожидания процесса i = сумма burst[0..i-1]. Верни среднее по n процессам.
double fcfs_avg_waiting(const int *burst, int n) {
    long long total_waiting = 0;
    long long current_time = 0;
    for (int i = 0; i < n; i++) {
        // Время ожидания процесса i — это текущий момент до его запуска
        total_waiting += current_time;
        current_time += burst[i];
    }
    return (double)total_waiting / n;
}

// Контракт: порядок исполнения процессов при Round-Robin с квантом quantum.
// Заполни order_out последовательностью id (по одному id за каждый выданный квант)
// и запиши длину этой последовательности в *order_len.
void round_robin_order(const int *burst, int n, int quantum,
                       int *order_out, int *order_len) {
    // Массив остатков работы для каждого процесса
    int *rem = (int *)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) {
        rem[i] = burst[i];
    }

    // Очередь реализована как массив с индексами head/tail
    // Максимальная длина очереди — n (одновременно в очереди не более n процессов)
    int *queue = (int *)malloc(n * sizeof(int));
    int head = 0, tail = 0;

    // Изначально все процессы в очереди по порядку
    for (int i = 0; i < n; i++) {
        queue[tail++] = i;
    }

    int out_idx = 0;

    while (head < tail) {
        int pid = queue[head++];

        // Записываем этот квант в порядок выдачи
        order_out[out_idx++] = pid;

        // Тратим один квант на этот процесс
        int used = (rem[pid] < quantum) ? rem[pid] : quantum;
        rem[pid] -= used;

        // Если осталась работа — ставим в конец очереди
        if (rem[pid] > 0) {
            queue[tail++] = pid;
        }
    }

    *order_len = out_idx;

    free(rem);
    free(queue);
}
