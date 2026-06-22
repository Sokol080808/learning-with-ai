// Тесты для заданий 1–3: async-функции, `.await`, `join!`, `join_all`.
//
// Файл — отдельный интеграционный крейт; импортирует только публичный API модуля.
// Никаких proc-макро-атрибутов (#[tokio::test]/#[futures_test]) — каждый async
// прогоняется через `futures::executor::block_on(...)` ВНУТРИ обычной `#[test]` функции.
// Так двигатель (executor) виден явно — ровно то, чему учит модуль.

use futures::executor::block_on;
use m16_async::{add, double, double_then_add, sum_all, sum_two};

// ===========================================================================
// Задание 1 — double / add / double_then_add
// ===========================================================================

#[test]
fn double_basic() {
    assert_eq!(block_on(double(21)), 42);
}

#[test]
fn double_zero() {
    assert_eq!(block_on(double(0)), 0);
}

#[test]
fn double_negative() {
    assert_eq!(block_on(double(-5)), -10);
}

#[test]
fn add_basic() {
    assert_eq!(block_on(add(40, 2)), 42);
}

#[test]
fn add_with_negatives() {
    assert_eq!(block_on(add(-7, 7)), 0);
    assert_eq!(block_on(add(-3, -4)), -7);
}

#[test]
fn double_then_add_basic() {
    // double(10) == 20; + 5 == 25
    assert_eq!(block_on(double_then_add(10, 5)), 25);
}

#[test]
fn double_then_add_uses_await_order() {
    // 2*x + y для нескольких пар — проверяем формулу, а не случайное совпадение.
    assert_eq!(block_on(double_then_add(0, 0)), 0);
    assert_eq!(block_on(double_then_add(-4, 1)), -7);
    assert_eq!(block_on(double_then_add(100, -50)), 150);
}

// ===========================================================================
// Задание 2 — sum_two через join!
// ===========================================================================

#[test]
fn sum_two_of_two_doubles() {
    // double(3)=6, double(4)=8 → 14
    assert_eq!(block_on(sum_two(double(3), double(4))), 14);
}

#[test]
fn sum_two_of_async_blocks() {
    // sum_two принимает любые Future<Output = i64> — здесь это голые async-блоки.
    assert_eq!(block_on(sum_two(async { 100 }, async { -1 })), 99);
}

#[test]
fn sum_two_with_zeros() {
    assert_eq!(block_on(sum_two(async { 0 }, async { 0 })), 0);
}

#[test]
fn sum_two_mixed_future_kinds() {
    // Один аргумент — future из async fn, другой — async-блок. Типы разные, но оба
    // Future<Output = i64>, и дженерик-сигнатура это принимает.
    assert_eq!(block_on(sum_two(double(7), async { 8 })), 22);
}

// ===========================================================================
// Задание 3 — sum_all через join_all
// ===========================================================================

#[test]
fn sum_all_three_doubles() {
    // double(1)+double(2)+double(3) = 2+4+6 = 12
    assert_eq!(block_on(sum_all(vec![double(1), double(2), double(3)])), 12);
}

#[test]
fn sum_all_empty_is_zero() {
    let empty: Vec<std::pin::Pin<Box<dyn std::future::Future<Output = i64>>>> = vec![];
    assert_eq!(block_on(sum_all(empty)), 0);
}

#[test]
fn sum_all_one_to_ten_doubled() {
    // sum of 2*k for k in 1..=10 = 2 * 55 = 110
    let futs: Vec<_> = (1..=10).map(double).collect();
    assert_eq!(block_on(sum_all(futs)), 110);
}

#[test]
fn sum_all_single_element() {
    assert_eq!(block_on(sum_all(vec![double(21)])), 42);
}

#[test]
fn sum_all_with_async_blocks() {
    // Подвох: КАЖДЫЙ async-блок имеет СВОЙ уникальный тип, поэтому
    // `vec![async {10}, async {-3}]` не компилируется — у элементов Vec разные типы.
    // Лекарство — сделать future одинакового типа: `async move { x }` по одной переменной.
    let futs: Vec<_> = [10, -3, 5].into_iter().map(|x| async move { x }).collect();
    assert_eq!(block_on(sum_all(futs)), 12);
}
