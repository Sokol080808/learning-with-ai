#include "warmup.hpp"

// Это эталонный ответ (answer key) — справочная ветка, не выдаётся учащимся.

int add(int a, int b) {
    return a + b;
}

long seconds_in(int hours) {
    return static_cast<long>(hours) * 60 * 60;
}

// Задание 2. ОПРЕДЕЛЕНИЕ функции, объявленной в warmup.hpp.
// Сигнатура обязана совпадать с объявлением до буквы, иначе линковщик
// не свяжет вызов с этим телом.
long abs_diff(int a, int b) {
    long diff = static_cast<long>(a) - static_cast<long>(b);
    return diff < 0 ? -diff : diff;
}
