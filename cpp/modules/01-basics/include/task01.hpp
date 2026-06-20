#pragma once

#include <optional>
#include <string_view>

// Перевод температуры из градусов Цельсия в градусы Фаренгейта.
double to_fahrenheit(double c);

// true, если n чётное.
bool is_even(int n);

// Наименьшее из трёх чисел.
int min_of_three(int a, int b, int c);

// Утраивает значение через ссылку (меняет оригинал вызывающей стороны).
void triple(int& x);

// Среднее арифметическое трёх целых чисел (дробный результат).
double average3(int a, int b, int c);

// --- Задание 6 ---
// Сложение int с защитой от переполнения. Если a + b выходит за пределы int,
// возвращает std::nullopt; иначе — std::optional с суммой.
// ВАЖНО: переполнение знакового int — это UB, поэтому проверять его НУЖНО ДО сложения,
// а не после.
std::optional<int> safe_add(int a, int b);

// --- Задание 7: битовые операции ---
// Биты нумеруются с нуля, бит 0 — младший. pos в диапазоне [0, 31].
// Возвращает значение, у которого бит pos установлен в 1 (остальные как в value).
unsigned int set_bit(unsigned int value, int pos);
// Возвращает значение, у которого бит pos сброшен в 0.
unsigned int clear_bit(unsigned int value, int pos);
// Возвращает значение, у которого бит pos инвертирован.
unsigned int toggle_bit(unsigned int value, int pos);
// Возвращает true, если бит pos в value равен 1.
bool get_bit(unsigned int value, int pos);

// --- Задание 8 ---
// Меняет местами значения a и b через ссылки. БЕЗ std::swap.
void swap_ints(int& a, int& b);

// --- Задание 9 ---
// Возвращает вид на первое «слово» в строке s (всё до первого пробела ' ').
// Если строка пустая или состоит только из пробелов — возвращает пустой string_view.
// Результат указывает ВНУТРЬ исходной строки (zero-copy, без аллокации).
std::string_view first_word(std::string_view s);
