#pragma once
#include <string>

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
