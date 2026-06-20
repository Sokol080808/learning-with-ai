// ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 12 стали зелёными.
//
// Подключаем СВОЙ заголовок — так компилятор сверит, что твои реализации
// совпадают по сигнатуре с объявлениями в include/paging.h.
#include "paging.h"
#include <stdlib.h>   // calloc/malloc/free для ленивых L2-таблиц

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

// ============================================================
//  Двухуровневая таблица страниц (симулятор трансляции)
// ============================================================
//
// Раскладка vaddr (32 бита):  [ L1: 10 | L2: 10 | offset: 12 ].
// L1 — директория из PT_ENTRIES указателей на L2-таблицы.
// L2 — таблица из PT_ENTRIES записей; запись хранит номер кадра + present.
// L2-таблицы создаются лениво (только под используемые куски памяти).

// Одна запись L2: present == 0 -> страница не отображена.
typedef struct {
    uint32_t frame;    // номер физического кадра
    int      present;  // 1, если страница отображена
} L2Entry;

// Вложенная таблица второго уровня: PT_ENTRIES записей.
typedef struct {
    L2Entry entries[PT_ENTRIES];
} L2Table;

// Сама таблица страниц: директория L1 + счётчик розданных кадров.
struct PageTable {
    L2Table *l1[PT_ENTRIES];  // l1[i] == NULL, пока кусок не использован
    uint32_t next_frame;      // следующий свободный кадр для ленивой раздачи
};

// --- разбор виртуального адреса на три поля ---
static inline uint32_t pt_l1_index(uint32_t vaddr) {
    return (vaddr >> (PT_INDEX_BITS + PT_PAGE_BITS)) & (PT_ENTRIES - 1);
}
static inline uint32_t pt_l2_index(uint32_t vaddr) {
    return (vaddr >> PT_PAGE_BITS) & (PT_ENTRIES - 1);
}
static inline uint32_t pt_offset(uint32_t vaddr) {
    return vaddr & (PT_PAGE_SIZE - 1);
}

PageTable *pt_create(void) {
    // calloc обнуляет: все l1[] == NULL, next_frame == 0.
    return (PageTable *)calloc(1, sizeof(PageTable));
}

void pt_destroy(PageTable *pt) {
    if (pt == NULL) {
        return;
    }
    for (uint32_t i = 0; i < PT_ENTRIES; ++i) {
        free(pt->l1[i]);  // free(NULL) безопасен
    }
    free(pt);
}

// Возвращает L2-таблицу для l1-индекса, создавая её при create != 0.
// При create == 0 и отсутствии таблицы возвращает NULL.
static L2Table *pt_get_l2(PageTable *pt, uint32_t l1, int create) {
    if (pt->l1[l1] != NULL) {
        return pt->l1[l1];
    }
    if (!create) {
        return NULL;
    }
    // calloc -> все записи present == 0 (страницы не отображены).
    L2Table *t = (L2Table *)calloc(1, sizeof(L2Table));
    pt->l1[l1] = t;  // при неудаче останется NULL — вызывающий проверит
    return t;
}

int64_t pt_walk(PageTable *pt, uint32_t vaddr, int alloc) {
    if (pt == NULL) {
        return -1;
    }
    uint32_t l1  = pt_l1_index(vaddr);
    uint32_t l2  = pt_l2_index(vaddr);
    uint32_t off = pt_offset(vaddr);

    L2Table *table = pt_get_l2(pt, l1, alloc);
    if (table == NULL) {
        return -1;  // нет L2-таблицы и не просили аллоцировать -> page fault
    }

    L2Entry *e = &table->entries[l2];
    if (!e->present) {
        if (!alloc) {
            return -1;  // страница не отображена -> page fault
        }
        // ОС «обработала промах»: раздаём очередной свежий кадр.
        e->frame   = pt->next_frame++;
        e->present = 1;
    }

    // Физический адрес: кадр в старших битах, смещение — как есть.
    return ((int64_t)e->frame << PT_PAGE_BITS) | off;
}

int pt_map(PageTable *pt, uint32_t vaddr, uint32_t paddr) {
    if (pt == NULL) {
        return -1;
    }
    uint32_t l1 = pt_l1_index(vaddr);
    uint32_t l2 = pt_l2_index(vaddr);

    L2Table *table = pt_get_l2(pt, l1, 1);
    if (table == NULL) {
        return -1;  // не удалось выделить L2-таблицу
    }

    // Отображаем СТРАНИЦУ: берём только номер кадра paddr, смещение игнорим.
    L2Entry *e = &table->entries[l2];
    e->frame   = paddr >> PT_PAGE_BITS;
    e->present = 1;
    return 0;
}
