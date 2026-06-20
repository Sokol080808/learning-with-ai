// ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 12 стали зелёными.
//
// Подключаем СВОЙ заголовок — так компилятор сверит, что твои реализации
// совпадают по сигнатуре с объявлениями в include/paging.h.
#include "paging.h"

// Контракт: номер страницы для адреса addr при размере страницы page_size.
// page_size — степень двойки. Номер страницы = addr / page_size.
size_t page_number(size_t addr, size_t page_size) {
    // Номер страницы — это сколько целых страниц «помещается» до адреса.
    return addr / page_size;
}

// Контракт: смещение адреса addr внутри его страницы. Смещение = addr % page_size.
size_t page_offset(size_t addr, size_t page_size) {
    // Смещение — это остаток адреса после отбрасывания целых страниц.
    return addr % page_size;
}

// Контракт: число промахов при FIFO-замещении для строки обращений refs.
// Память пуста в начале (холодные промахи считаются). Жертва — самая «старая»
// по времени ЗАГРУЗКИ страница.
int fifo_page_faults(const int *refs, int n, int frames) {
    if (frames <= 0) {
        // Без единого кадра каждое обращение — промах.
        return n > 0 ? n : 0;
    }

    int mem[frames];   // что сейчас загружено в кадрах
    int count = 0;     // сколько кадров реально занято
    int next = 0;      // указатель на следующую жертву (кольцевой)
    int faults = 0;

    for (int i = 0; i < n; ++i) {
        int page = refs[i];

        // Уже в памяти? Тогда попадание — порядок загрузки не трогаем.
        int hit = 0;
        for (int j = 0; j < count; ++j) {
            if (mem[j] == page) {
                hit = 1;
                break;
            }
        }
        if (hit) {
            continue;
        }

        // Промах.
        ++faults;
        if (count < frames) {
            // Есть свободный кадр — просто занимаем его.
            mem[count++] = page;
        } else {
            // Память полна — выселяем самую «старую» по загрузке (FIFO)
            // и кладём новую на её место. next движется по кругу.
            mem[next] = page;
            next = (next + 1) % frames;
        }
    }

    return faults;
}

// Контракт: число промахов при LRU-замещении. Жертва — страница, к которой
// дольше всего НЕ обращались. Любое обращение (включая hit) обновляет «свежесть».
int lru_page_faults(const int *refs, int n, int frames) {
    if (frames <= 0) {
        // Без единого кадра каждое обращение — промах.
        return n > 0 ? n : 0;
    }

    int mem[frames];    // что сейчас загружено в кадрах
    int last[frames];   // номер шага последнего обращения к странице в кадре j
    int count = 0;      // сколько кадров реально занято
    int faults = 0;

    for (int i = 0; i < n; ++i) {
        int page = refs[i];

        // Ищем страницу среди занятых кадров.
        int idx = -1;
        for (int j = 0; j < count; ++j) {
            if (mem[j] == page) {
                idx = j;
                break;
            }
        }

        if (idx >= 0) {
            // Попадание: обновляем «свежесть» — это ключевое отличие от FIFO.
            last[idx] = i;
            continue;
        }

        // Промах.
        ++faults;
        if (count < frames) {
            // Занимаем свободный кадр.
            mem[count] = page;
            last[count] = i;
            ++count;
        } else {
            // Память полна — выселяем least-recently-used: минимальный last.
            int victim = 0;
            for (int j = 1; j < count; ++j) {
                if (last[j] < last[victim]) {
                    victim = j;
                }
            }
            mem[victim] = page;
            last[victim] = i;
        }
    }

    return faults;
}
