#pragma once

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
