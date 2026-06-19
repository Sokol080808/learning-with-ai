#pragma once

// Ядро калькулятора: применяет операцию op к a и b.
// Гарантии для тестов: op — один из '+', '-', '*', '/'; при op == '/' делитель b != 0.
double calc(double a, char op, double b);
