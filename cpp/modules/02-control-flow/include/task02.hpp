#pragma once
#include <string>
#include <vector>

// n! (факториал). factorial(0) == 1.
int factorial(int n);

// n-е число Фибоначчи: fib(0)=0, fib(1)=1.
int fib(int n);

// Наибольший общий делитель (алгоритм Евклида).
int gcd(int a, int b);

// Классический FizzBuzz для одного числа: "Fizz" / "Buzz" / "FizzBuzz" / само число.
std::string fizzbuzz(int n);

// Перегрузка: площадь квадрата и площадь прямоугольника.
int area(int side);
int area(int w, int h);

// --- Дополнительные (более сложные) задания ---

// Наименьшее общее кратное. lcm(0, x) == 0. Гарантируется a >= 0, b >= 0.
int lcm(int a, int b);

// Решето Эратосфена: все простые числа от 2 до n включительно, по возрастанию.
// При n < 2 — пустой вектор.
std::vector<int> sieve(int n);

// Бинарный поиск в отсортированном по возрастанию векторе.
// Возвращает индекс элемента, равного target, или -1, если его нет.
int binary_search(const std::vector<int>& v, int target);

// --- Задание 9: перегрузка + значения по умолчанию ---

// Прямоугольник w×h из символа fill; строки разделены '\n'.
// Пример: print_box(3, 2) == "***\n***"
std::string print_box(int w, int h, char fill = '*');

// Квадрат side×side — перегрузка с одним целым параметром.
// Пример: print_box(2) == "**\n**"
std::string print_box(int side, char fill = '*');
