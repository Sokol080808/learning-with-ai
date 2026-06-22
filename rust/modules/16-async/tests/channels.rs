// Тесты для заданий 4–5: async-канал (channel_sum) и конкурентный map-reduce (run_workers).
//
// Файл — отдельный интеграционный крейт; импортирует только публичный API.
// channel_sum асинхронна — гоняем через block_on; run_workers синхронна — зовём напрямую.

use futures::executor::block_on;
use m16_async::{channel_sum, run_workers};

// ===========================================================================
// Задание 4 — channel_sum
// ===========================================================================

#[test]
fn channel_sum_basic() {
    assert_eq!(block_on(channel_sum(vec![1, 2, 3, 4, 5])), 15);
}

#[test]
fn channel_sum_empty_is_zero() {
    // Пустой вход: ни одного значения, отправитель сразу закрывается, Stream пуст → 0.
    // Важно: функция ОБЯЗАНА завершиться (не зависнуть) — block_on вернёт управление.
    assert_eq!(block_on(channel_sum(vec![])), 0);
}

#[test]
fn channel_sum_cancels_out() {
    assert_eq!(block_on(channel_sum(vec![-3, 3])), 0);
}

#[test]
fn channel_sum_single_value() {
    assert_eq!(block_on(channel_sum(vec![42])), 42);
}

#[test]
fn channel_sum_all_negative() {
    assert_eq!(block_on(channel_sum(vec![-1, -2, -3, -4])), -10);
}

#[test]
fn channel_sum_does_not_hang_on_large_input() {
    // Если отправитель не закрыт, Stream не завершится и тест зависнет. То, что мы сюда
    // вообще доходим с ответом, доказывает: отправитель закрывается корректно.
    let v: Vec<i64> = (1..=100).collect();
    assert_eq!(block_on(channel_sum(v)), 5050);
}

// ===========================================================================
// Задание 5 — run_workers (конкурентный map-reduce)
// ===========================================================================

#[test]
fn run_workers_basic() {
    // 1+2+3 + 4+5 + 6 = 21
    assert_eq!(run_workers(vec![vec![1, 2, 3], vec![4, 5], vec![6]]), 21);
}

#[test]
fn run_workers_no_chunks_is_zero() {
    assert_eq!(run_workers(vec![]), 0);
}

#[test]
fn run_workers_single_empty_chunk_is_zero() {
    assert_eq!(run_workers(vec![vec![]]), 0);
}

#[test]
fn run_workers_single_chunk_equals_its_sum() {
    assert_eq!(run_workers(vec![vec![10, 20, 30]]), 60);
}

#[test]
fn run_workers_empty_chunks_contribute_zero() {
    // Пустые чанки вносят 0; итог = сумма непустых.
    assert_eq!(run_workers(vec![vec![], vec![5], vec![], vec![5]]), 10);
}

#[test]
fn run_workers_order_independent() {
    // Перестановка чанков не меняет итог (сумма коммутативна).
    let a = run_workers(vec![vec![1, 2], vec![3, 4], vec![5]]);
    let b = run_workers(vec![vec![5], vec![3, 4], vec![1, 2]]);
    assert_eq!(a, b);
    assert_eq!(a, 15);
}

#[test]
fn run_workers_with_negatives() {
    assert_eq!(run_workers(vec![vec![-10, 5], vec![5], vec![0]]), 0);
}

#[test]
fn run_workers_many_chunks() {
    // 50 чанков по одному значению k: сумма 0+1+...+49 = 1225.
    let chunks: Vec<Vec<i64>> = (0..50).map(|k| vec![k]).collect();
    assert_eq!(run_workers(chunks), 1225);
}
