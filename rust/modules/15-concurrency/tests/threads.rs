// Эти тесты трогать не нужно — это эталон поведения.
// Они описывают, как ДОЛЖНЫ вести себя `parallel_sum` и `pipeline_collect` из src/lib.rs.
// Сейчас они падают (внутри `todo!()` — паника), но компилируются. Реализуй стаб —
// и красное станет зелёным.
//
// Здесь же — рандомизированные property-тесты на детерминированном ГПСЧ (xorshift64*),
// без внешних крейтов: прогон всегда воспроизводим.

use m15_concurrency::{parallel_sum, pipeline_collect};

// ---------------------------------------------------------------------------
// Детерминированный ГПСЧ (xorshift64*) — без внешних крейтов.
// Сид ФИКСИРОВАННЫЙ (hardcode), прогон детерминирован.
// ---------------------------------------------------------------------------

/// маленький детерминированный ГПСЧ (xorshift64*).
fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}

/// целое в диапазоне [lo, hi] включительно (диапазон скромный — оракул не переполняет i64).
fn in_range(state: &mut u64, lo: i64, hi: i64) -> i64 {
    let span = (hi - lo + 1) as u64;
    lo + (next_u64(state) % span) as i64
}

/// случайный Vec<i64> заданной длины.
fn random_vec(state: &mut u64, len: usize, lo: i64, hi: i64) -> Vec<i64> {
    (0..len).map(|_| in_range(state, lo, hi)).collect()
}

// ===========================================================================
// Задание 1 — parallel_sum: детерминированные кейсы
// ===========================================================================

#[test]
fn parallel_sum_basic() {
    assert_eq!(parallel_sum(vec![1, 2, 3, 4, 5], 2), 15);
}

#[test]
fn parallel_sum_empty_is_zero() {
    assert_eq!(parallel_sum(vec![], 4), 0);
}

#[test]
fn parallel_sum_single_chunk_matches_plain_sum() {
    let data = vec![10, -3, 7, 100, -50];
    let expected: i64 = data.iter().sum();
    assert_eq!(parallel_sum(data, 1), expected);
}

#[test]
fn parallel_sum_zero_chunks_does_not_panic() {
    // chunks == 0 трактуется как 1 — не должно паниковать и должно дать верную сумму.
    let data = vec![4, 8, 15, 16, 23, 42];
    let expected: i64 = data.iter().sum();
    assert_eq!(parallel_sum(data, 0), expected);
}

#[test]
fn parallel_sum_more_chunks_than_elements() {
    // Частей больше, чем элементов — лишние части просто пустые.
    assert_eq!(parallel_sum(vec![1, 2, 3], 10), 6);
}

#[test]
fn parallel_sum_independent_of_chunk_count() {
    // Один и тот же вход при любом числе частей даёт один результат.
    let data = vec![5, -1, 9, 12, -7, 3, 0, 8, -2, 11];
    let expected: i64 = data.iter().sum();
    for chunks in 1..=12 {
        assert_eq!(
            parallel_sum(data.clone(), chunks),
            expected,
            "результат должен совпадать при chunks = {chunks}"
        );
    }
}

/// Оракул: parallel_sum(data, chunks) == data.iter().sum() при любом числе частей.
#[test]
fn prop_parallel_sum_matches_oracle() {
    let mut rng: u64 = 0xDEAD_BEEF_1234_5678;
    for _ in 0..500 {
        let len = in_range(&mut rng, 0, 200) as usize;
        let data = random_vec(&mut rng, len, -1_000, 1_000);
        let chunks = in_range(&mut rng, 1, 16) as usize;
        let expected: i64 = data.iter().sum();
        assert_eq!(
            parallel_sum(data.clone(), chunks),
            expected,
            "несовпадение при chunks = {chunks}, len = {len}"
        );
    }
}

/// Свойство: результат НЕ зависит от числа частей (для одного и того же входа).
#[test]
fn prop_parallel_sum_chunk_invariant() {
    let mut rng: u64 = 0xCAFE_BABE_0000_0001;
    for _ in 0..300 {
        let len = in_range(&mut rng, 0, 150) as usize;
        let data = random_vec(&mut rng, len, -1_000, 1_000);
        let base = parallel_sum(data.clone(), 1);
        let c = in_range(&mut rng, 1, 16) as usize;
        assert_eq!(
            parallel_sum(data.clone(), c),
            base,
            "результат при chunks={c} разошёлся с chunks=1"
        );
    }
}

// ===========================================================================
// Задание 2 — pipeline_collect: детерминированные кейсы
// ===========================================================================

/// Хелпер: отсортированная копия (порядок результата не гарантирован).
fn sorted(mut v: Vec<i64>) -> Vec<i64> {
    v.sort_unstable();
    v
}

#[test]
fn pipeline_collect_squares_basic() {
    assert_eq!(sorted(pipeline_collect(vec![1, 2, 3], 2)), vec![1, 4, 9]);
}

#[test]
fn pipeline_collect_empty_is_empty() {
    assert_eq!(pipeline_collect(vec![], 4), Vec::<i64>::new());
}

#[test]
fn pipeline_collect_preserves_length() {
    let inputs = vec![5, 6, 7, 8, 9, 10, 11];
    let out = pipeline_collect(inputs.clone(), 3);
    assert_eq!(out.len(), inputs.len());
}

#[test]
fn pipeline_collect_zero_workers_does_not_hang() {
    // workers == 0 трактуется как 1 — не виснет и даёт верный мультимножество.
    let out = pipeline_collect(vec![2, 3, 4], 0);
    assert_eq!(sorted(out), vec![4, 9, 16]);
}

#[test]
fn pipeline_collect_negatives_square_positive() {
    let out = pipeline_collect(vec![-2, -3, 4], 2);
    assert_eq!(sorted(out), vec![4, 9, 16]);
}

#[test]
fn pipeline_collect_more_workers_than_inputs() {
    // Потоков больше, чем входов — часть потоков ничего не отправит, но всё сойдётся.
    let out = pipeline_collect(vec![3, 5], 8);
    assert_eq!(sorted(out), vec![9, 25]);
}

/// Оракул: мультимножество результата == { x*x } независимо от числа воркеров.
#[test]
fn prop_pipeline_collect_matches_oracle() {
    let mut rng: u64 = 0x1111_2222_3333_4444;
    for _ in 0..500 {
        let len = in_range(&mut rng, 0, 100) as usize;
        let inputs = random_vec(&mut rng, len, -1_000, 1_000);
        let workers = in_range(&mut rng, 1, 8) as usize;

        let mut expected: Vec<i64> = inputs.iter().map(|&x| x * x).collect();
        expected.sort_unstable();

        let got = sorted(pipeline_collect(inputs.clone(), workers));
        assert_eq!(
            got, expected,
            "несовпадение мультимножества при workers={workers}, len={len}"
        );
    }
}

/// Свойство: длина результата всегда равна длине входа.
#[test]
fn prop_pipeline_collect_length_preserved() {
    let mut rng: u64 = 0x5555_6666_7777_8888;
    for _ in 0..300 {
        let len = in_range(&mut rng, 0, 100) as usize;
        let inputs = random_vec(&mut rng, len, -500, 500);
        let workers = in_range(&mut rng, 1, 8) as usize;
        assert_eq!(pipeline_collect(inputs.clone(), workers).len(), inputs.len());
    }
}
