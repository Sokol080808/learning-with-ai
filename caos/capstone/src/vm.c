// REFERENCE SOLUTION (ветка reference) — эталонная реализация Tiny VM.
//
// Стековая виртуальная машина: байткод-интерпретатор с классическим циклом
// «выборка — декодирование — исполнение» (fetch — decode — execute). Контракт —
// в include/vm.h, поведение зафиксировано в tests/test_vm.cpp.
//
// Состояние машины:
//   stack[VM_STACK_CAPACITY] — массив-стек целых чисел;
//   sp — индекс «следующей свободной ячейки» = текущее число элементов на стеке
//        (stack[sp-1] — вершина, если sp > 0);
//   pc — счётчик команд, индекс в массиве program.

#include "vm.h"

// Внутреннее состояние интерпретатора. Держим стек и его указатель вершины
// вместе, чтобы помощники snx/push работали над одной структурой.
typedef struct {
    int stack[VM_STACK_CAPACITY];
    size_t sp;  // количество значений на стеке (индекс следующей свободной ячейки)
} VmState;

// Положить значение на стек. Возвращает VM_OK либо VM_ERR_STACK_OVERFLOW,
// если стек уже заполнен (sp достиг вместимости).
static int vm_push(VmState* vm, int value) {
    if (vm->sp >= VM_STACK_CAPACITY) {
        return VM_ERR_STACK_OVERFLOW;
    }
    vm->stack[vm->sp++] = value;
    return VM_OK;
}

// Снять значение с вершины в *out. Возвращает VM_OK либо
// VM_ERR_STACK_UNDERFLOW, если стек пуст.
static int vm_pop(VmState* vm, int* out) {
    if (vm->sp == 0) {
        return VM_ERR_STACK_UNDERFLOW;
    }
    *out = vm->stack[--vm->sp];
    return VM_OK;
}

// Контракт см. в vm.h: выполнить программу, вернуть VM_OK и записать результат в
// *out_result при успехе, либо код ошибки из VmStatus.
int vm_run(const int* program, size_t len, int* out_result) {
    VmState vm = { .sp = 0 };
    size_t pc = 0;

    for (;;) {
        // ВЫБОРКА. Если pc вышел за пределы программы — мы либо дошли до конца
        // без HALT, либо ожидаем операнд, которого нет. И то и другое — ошибка.
        if (pc >= len) {
            return VM_ERR_PC_OUT_OF_RANGE;
        }

        // ДЕКОДИРОВАНИЕ.
        int opcode = program[pc];

        switch (opcode) {
            case OP_PUSH: {
                // Операнд лежит в следующей ячейке. Если её нет — pc за границей.
                if (pc + 1 >= len) {
                    return VM_ERR_PC_OUT_OF_RANGE;
                }
                int operand = program[pc + 1];
                int st = vm_push(&vm, operand);
                if (st != VM_OK) {
                    return st;
                }
                pc += 2;  // опкод + операнд
                break;
            }

            case OP_ADD:
            case OP_SUB:
            case OP_MUL:
            case OP_DIV: {
                // Снимаем два значения: b — то, что лежало сверху (снято первым),
                // a — то, что под ним. Контракт: SUB даёт a - b, DIV даёт a / b.
                int b, a;
                int st = vm_pop(&vm, &b);
                if (st != VM_OK) {
                    return st;
                }
                st = vm_pop(&vm, &a);
                if (st != VM_OK) {
                    return st;
                }

                int result;
                switch (opcode) {
                    case OP_ADD: result = a + b; break;
                    case OP_SUB: result = a - b; break;
                    case OP_MUL: result = a * b; break;
                    case OP_DIV:
                        if (b == 0) {
                            return VM_ERR_DIV_ZERO;
                        }
                        result = a / b;  // целочисленное деление, дробь отбрасывается
                        break;
                    default: result = 0; break;  // недостижимо
                }

                // После снятия двух гарантированно есть место — push не переполнит.
                st = vm_push(&vm, result);
                if (st != VM_OK) {
                    return st;
                }
                pc += 1;
                break;
            }

            case OP_NEG: {
                int x;
                int st = vm_pop(&vm, &x);
                if (st != VM_OK) {
                    return st;
                }
                st = vm_push(&vm, -x);
                if (st != VM_OK) {
                    return st;
                }
                pc += 1;
                break;
            }

            case OP_DUP: {
                // Посмотреть вершину (не снимая) и положить её копию.
                if (vm.sp == 0) {
                    return VM_ERR_STACK_UNDERFLOW;
                }
                int top = vm.stack[vm.sp - 1];
                int st = vm_push(&vm, top);
                if (st != VM_OK) {
                    return st;  // DUP при полном стеке — overflow
                }
                pc += 1;
                break;
            }

            case OP_HALT: {
                // Результат — текущая вершина. Пустой стек — возвращать нечего.
                if (vm.sp == 0) {
                    return VM_ERR_EMPTY_RESULT;
                }
                *out_result = vm.stack[vm.sp - 1];
                return VM_OK;
            }

            default:
                // Неизвестный опкод — машина сигналит об ошибке, а не падает.
                return VM_ERR_BAD_OPCODE;
        }
    }
}
