#include "calc.hpp"

// ЭТАЛОННОЕ РЕШЕНИЕ (ветка reference) — на main здесь стаб с // TODO.
double calc(double a, char op, double b) {
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return a / b;
        default:  return 0.0;
    }
}
