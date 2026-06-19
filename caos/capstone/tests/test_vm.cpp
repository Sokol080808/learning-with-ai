// Эти тесты трогать не нужно — это эталон поведения твоей Tiny VM.
//
// Они описывают, ЧТО машина обязана делать: какие программы какой результат
// дают и какие ошибки какой код возврата дают. Если тест красный — смотри, что
// ожидалось (Expected) и что получилось (Actual), и правь src/vm.c.
//
// Заметь: тесты ссылаются на ИМЕНА опкодов и кодов ошибок (OP_PUSH, VM_OK,
// VM_ERR_DIV_ZERO, ...), а не на числа. Так и должно быть — контракт в именах.

#include <gtest/gtest.h>

extern "C" {
#include "vm.h"
}

// Удобный помощник: прогнать программу из инициализатора и вернуть статус,
// результат отдать через out. Делает тела тестов короче и читаемее.
static int run(const std::vector<int>& prog, int* out) {
    return vm_run(prog.data(), prog.size(), out);
}

// ===================================================================
//  Часть 1. Корректные программы — машина должна вернуть VM_OK и число.
// ===================================================================

TEST(VmBasic, PushHalt) {
    // Просто положить число и остановиться — результат это само число.
    int out = -1;
    EXPECT_EQ(run({OP_PUSH, 42, OP_HALT}, &out), VM_OK);
    EXPECT_EQ(out, 42);
}

TEST(VmBasic, PushNegativeOperand) {
    // Операнд PUSH может быть отрицательным — это обычный int в массиве.
    int out = 0;
    EXPECT_EQ(run({OP_PUSH, -7, OP_HALT}, &out), VM_OK);
    EXPECT_EQ(out, -7);
}

TEST(VmArith, Add) {
    // 2 + 3 = 5
    int out = 0;
    EXPECT_EQ(run({OP_PUSH, 2, OP_PUSH, 3, OP_ADD, OP_HALT}, &out), VM_OK);
    EXPECT_EQ(out, 5);
}

TEST(VmArith, SubOrderMatters) {
    // SUB снимает (a, b) и кладёт a - b. Положили 10, потом 3 -> 10 - 3 = 7.
    // Порядок важен: это НЕ b - a.
    int out = 0;
    EXPECT_EQ(run({OP_PUSH, 10, OP_PUSH, 3, OP_SUB, OP_HALT}, &out), VM_OK);
    EXPECT_EQ(out, 7);
}

TEST(VmArith, SubNegativeResult) {
    // 3 - 10 = -7 (проверяем, что знак не теряется).
    int out = 0;
    EXPECT_EQ(run({OP_PUSH, 3, OP_PUSH, 10, OP_SUB, OP_HALT}, &out), VM_OK);
    EXPECT_EQ(out, -7);
}

TEST(VmArith, Mul) {
    // 6 * 7 = 42
    int out = 0;
    EXPECT_EQ(run({OP_PUSH, 6, OP_PUSH, 7, OP_MUL, OP_HALT}, &out), VM_OK);
    EXPECT_EQ(out, 42);
}

TEST(VmArith, Div) {
    // 20 / 4 = 5 (порядок как у SUB: a / b, b сверху).
    int out = 0;
    EXPECT_EQ(run({OP_PUSH, 20, OP_PUSH, 4, OP_DIV, OP_HALT}, &out), VM_OK);
    EXPECT_EQ(out, 5);
}

TEST(VmArith, DivTruncatesTowardZero) {
    // Целочисленное деление в C отбрасывает дробную часть: 7 / 2 = 3.
    int out = 0;
    EXPECT_EQ(run({OP_PUSH, 7, OP_PUSH, 2, OP_DIV, OP_HALT}, &out), VM_OK);
    EXPECT_EQ(out, 3);
}

TEST(VmUnary, Neg) {
    // NEG снимает x и кладёт -x. -(5) = -5.
    int out = 0;
    EXPECT_EQ(run({OP_PUSH, 5, OP_NEG, OP_HALT}, &out), VM_OK);
    EXPECT_EQ(out, -5);
}

TEST(VmUnary, NegTwiceIsIdentity) {
    // Дважды поменять знак -> вернулись к исходному. -(-8) = 8.
    int out = 0;
    EXPECT_EQ(run({OP_PUSH, 8, OP_NEG, OP_NEG, OP_HALT}, &out), VM_OK);
    EXPECT_EQ(out, 8);
}

TEST(VmDup, DupThenAddDoubles) {
    // DUP копирует вершину. PUSH 9, DUP -> [9, 9], ADD -> 18.
    int out = 0;
    EXPECT_EQ(run({OP_PUSH, 9, OP_DUP, OP_ADD, OP_HALT}, &out), VM_OK);
    EXPECT_EQ(out, 18);
}

TEST(VmDup, DupThenMulSquares) {
    // Возвести в квадрат через DUP: PUSH 4, DUP -> [4,4], MUL -> 16.
    int out = 0;
    EXPECT_EQ(run({OP_PUSH, 4, OP_DUP, OP_MUL, OP_HALT}, &out), VM_OK);
    EXPECT_EQ(out, 16);
}

TEST(VmCompound, TwoPlusThreeTimesFour) {
    // (2 + 3) * 4 = 20 — та же программа, что в демо app/main.c.
    int out = 0;
    EXPECT_EQ(run({OP_PUSH, 2, OP_PUSH, 3, OP_ADD,
                   OP_PUSH, 4, OP_MUL, OP_HALT}, &out), VM_OK);
    EXPECT_EQ(out, 20);
}

TEST(VmCompound, MixedExpression) {
    // ((10 - 2) * 3 + (-5)) = 8 * 3 - 5 = 24 - 5 = 19.
    //   PUSH 10, PUSH 2, SUB        -> [8]
    //   PUSH 3, MUL                 -> [24]
    //   PUSH 5, NEG, ADD            -> 24 + (-5) = 19
    int out = 0;
    EXPECT_EQ(run({OP_PUSH, 10, OP_PUSH, 2, OP_SUB,
                   OP_PUSH, 3, OP_MUL,
                   OP_PUSH, 5, OP_NEG, OP_ADD, OP_HALT}, &out), VM_OK);
    EXPECT_EQ(out, 19);
}

TEST(VmHalt, ResultIsTopNotEmptied) {
    // На стеке может остаться больше одного значения — результат это ВЕРШИНА.
    //   PUSH 1, PUSH 2, PUSH 3, HALT -> вершина 3.
    int out = 0;
    EXPECT_EQ(run({OP_PUSH, 1, OP_PUSH, 2, OP_PUSH, 3, OP_HALT}, &out), VM_OK);
    EXPECT_EQ(out, 3);
}

TEST(VmStack, NearCapacityOk) {
    // Можно положить вплоть до VM_STACK_CAPACITY значений — это ещё не overflow.
    // Кладём ровно VM_STACK_CAPACITY единиц, затем HALT; вершина == 1.
    std::vector<int> prog;
    for (int i = 0; i < VM_STACK_CAPACITY; ++i) {
        prog.push_back(OP_PUSH);
        prog.push_back(1);
    }
    prog.push_back(OP_HALT);

    int out = 0;
    EXPECT_EQ(run(prog, &out), VM_OK);
    EXPECT_EQ(out, 1);
}

// ===================================================================
//  Часть 2. Ошибочные программы — машина должна вернуть НЕнулевой код,
//  причём ПРАВИЛЬНЫЙ для каждого случая.
// ===================================================================

TEST(VmError, DivisionByZero) {
    // 5 / 0 — деление на ноль.
    int out = 12345;  // должно остаться нетронутым/неважным
    EXPECT_EQ(run({OP_PUSH, 5, OP_PUSH, 0, OP_DIV, OP_HALT}, &out), VM_ERR_DIV_ZERO);
}

TEST(VmError, StackUnderflowAddOnEmpty) {
    // ADD без значений на стеке — снимать нечего.
    int out = 0;
    EXPECT_EQ(run({OP_ADD, OP_HALT}, &out), VM_ERR_STACK_UNDERFLOW);
}

TEST(VmError, StackUnderflowAddOneValue) {
    // ADD нужно ДВА значения, а на стеке одно — тоже underflow.
    int out = 0;
    EXPECT_EQ(run({OP_PUSH, 1, OP_ADD, OP_HALT}, &out), VM_ERR_STACK_UNDERFLOW);
}

TEST(VmError, StackUnderflowNegOnEmpty) {
    // NEG нужно одно значение, а стек пуст.
    int out = 0;
    EXPECT_EQ(run({OP_NEG, OP_HALT}, &out), VM_ERR_STACK_UNDERFLOW);
}

TEST(VmError, StackUnderflowDupOnEmpty) {
    // DUP копирует вершину, а её нет.
    int out = 0;
    EXPECT_EQ(run({OP_DUP, OP_HALT}, &out), VM_ERR_STACK_UNDERFLOW);
}

TEST(VmError, StackOverflow) {
    // Кладём на ОДИН больше, чем вмещает стек -> переполнение.
    std::vector<int> prog;
    for (int i = 0; i < VM_STACK_CAPACITY + 1; ++i) {
        prog.push_back(OP_PUSH);
        prog.push_back(7);
    }
    prog.push_back(OP_HALT);

    int out = 0;
    EXPECT_EQ(run(prog, &out), VM_ERR_STACK_OVERFLOW);
}

TEST(VmError, BadOpcode) {
    // 9999 — заведомо не существующий опкод.
    int out = 0;
    EXPECT_EQ(run({OP_PUSH, 1, 9999, OP_HALT}, &out), VM_ERR_BAD_OPCODE);
}

TEST(VmError, PushWithoutOperand) {
    // PUSH стоит последним: операнда за ним нет -> pc вышел за границы.
    int out = 0;
    EXPECT_EQ(run({OP_PUSH}, &out), VM_ERR_PC_OUT_OF_RANGE);
}

TEST(VmError, RunsOffEndWithoutHalt) {
    // Нет HALT: дойдя до конца массива, машина обязана сообщить об ошибке,
    // а не «свалиться» за границы и не вернуть мусор как успех.
    int out = 0;
    EXPECT_EQ(run({OP_PUSH, 1, OP_PUSH, 2, OP_ADD}, &out), VM_ERR_PC_OUT_OF_RANGE);
}

TEST(VmError, HaltOnEmptyStack) {
    // HALT при пустом стеке — возвращать нечего.
    int out = 0;
    EXPECT_EQ(run({OP_HALT}, &out), VM_ERR_EMPTY_RESULT);
}

TEST(VmError, AnyErrorIsNonZero) {
    // Резюме контракта: VM_OK == 0, а любая ошибка != 0. Демо и реальный код
    // полагаются на это, чтобы отличать успех от провала простым `if`.
    EXPECT_EQ(VM_OK, 0);
    EXPECT_NE(VM_ERR_DIV_ZERO, 0);
    EXPECT_NE(VM_ERR_STACK_UNDERFLOW, 0);
    EXPECT_NE(VM_ERR_STACK_OVERFLOW, 0);
    EXPECT_NE(VM_ERR_BAD_OPCODE, 0);
    EXPECT_NE(VM_ERR_PC_OUT_OF_RANGE, 0);
    EXPECT_NE(VM_ERR_EMPTY_RESULT, 0);
}
