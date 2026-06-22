// Рандомизированные property-тесты для m16_async.
// Детерминированный ГПСЧ (xorshift64*) — без внешних крейтов, воспроизводимо.
// Каждый async прогоняется через block_on внутри обычной #[test] функции.

use futures::executor::block_on;
use m16_async::{channel_sum, run_workers, sum_all};

// маленький детерминированный ГПСЧ (xorshift64*), без внешних крейтов — воспроизводимо.
// Сид ФИКСИРОВАННЫЙ (hardcode), чтобы прогон был детерминированным.
fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}

// целое в диапазоне [lo, hi] включительно (держи диапазон скромным, чтобы оракул не
// переполнял i64).
fn in_range(state: &mut u64, lo: i64, hi: i64) -> i64 {
    let span = (hi - lo + 1) as u64;
    lo + (next_u64(state) % span) as i64
}

// Случайный Vec<i64> длиной 0..=max_len со значениями в [lo, hi].
fn random_vec(state: &mut u64, max_len: usize, lo: i64, hi: i64) -> Vec<i64> {
    let len = (next_u64(state) as usize) % (max_len + 1);
    (0..len).map(|_| in_range(state, lo, hi)).collect()
}

// ===========================================================================
// channel_sum — оракул: обычный .iter().sum()
// ===========================================================================

/// Свойство: channel_sum(v) == v.iter().sum() для случайных векторов.
/// Диапазон значений [-1000, 1000], длина 0..=50 (как в контракте).
#[test]
fn prop_channel_sum_matches_iter_sum() {
    let mut rng: u64 = 0xA5A5_F00D_1234_9876;
    for _ in 0..600 {
        let v = random_vec(&mut rng, 50, -1000, 1000);
        let expected: i64 = v.iter().sum();
        assert_eq!(
            block_on(channel_sum(v.clone())),
            expected,
            "channel_sum не совпал с оракулом на {v:?}"
        );
    }
}

/// Свойство: channel_sum всегда завершается (не зависает) даже на больших входах.
/// Здесь оракул — та же сумма; сам факт возврата доказывает закрытие отправителя.
#[test]
fn prop_channel_sum_terminates() {
    let mut rng: u64 = 0x0BAD_C0DE_F00D_2025;
    for _ in 0..200 {
        let v = random_vec(&mut rng, 50, -1000, 1000);
        let expected: i64 = v.iter().sum();
        // Если бы отправитель не закрывался, этот вызов завис бы навсегда.
        assert_eq!(block_on(channel_sum(v.clone())), expected);
    }
}

// ===========================================================================
// sum_all — оракул: сумма исходных значений (future из async move { x })
// ===========================================================================

/// Свойство: sum_all(v.map(|x| async move { x })) == v.iter().sum().
#[test]
fn prop_sum_all_matches_iter_sum() {
    let mut rng: u64 = 0xDEAD_BEEF_CAFE_0001;
    for _ in 0..600 {
        let v = random_vec(&mut rng, 50, -1000, 1000);
        let expected: i64 = v.iter().sum();
        let futs: Vec<_> = v.iter().map(|&x| async move { x }).collect();
        assert_eq!(
            block_on(sum_all(futs)),
            expected,
            "sum_all не совпал с оракулом на {v:?}"
        );
    }
}

/// Свойство: порядок future не влияет на сумму (sum_all коммутативна по входу).
#[test]
fn prop_sum_all_order_independent() {
    let mut rng: u64 = 0x1357_9BDF_2468_ACE0;
    for _ in 0..400 {
        let mut v = random_vec(&mut rng, 40, -1000, 1000);
        let forward: Vec<_> = v.iter().map(|&x| async move { x }).collect();
        let sum_forward = block_on(sum_all(forward));

        v.reverse();
        let reversed: Vec<_> = v.iter().map(|&x| async move { x }).collect();
        let sum_reversed = block_on(sum_all(reversed));

        assert_eq!(sum_forward, sum_reversed);
    }
}

// ===========================================================================
// run_workers — оракул: сумма всех значений во всех чанках
// ===========================================================================

/// Свойство: run_workers(chunks) == flat.iter().sum() при случайном разбиении.
#[test]
fn prop_run_workers_matches_flat_sum() {
    let mut rng: u64 = 0xF00D_BABE_5555_7777;
    for _ in 0..500 {
        // Сначала плоский вектор, затем режем его на случайные чанки.
        let flat = random_vec(&mut rng, 50, -1000, 1000);
        let expected: i64 = flat.iter().sum();

        // Случайное разбиение на чанки. Иногда вставляем пустой чанк — он валиден и
        // должен вносить 0, не сбивая сумму.
        let mut chunks: Vec<Vec<i64>> = Vec::new();
        let mut i = 0;
        while i < flat.len() {
            // С вероятностью ~1/4 вставим пустой чанк перед очередным куском.
            if next_u64(&mut rng) % 4 == 0 {
                chunks.push(vec![]);
            }
            // Размер очередного НЕпустого куска: 1..=4 (гарантированно двигаемся вперёд).
            let step = 1 + (next_u64(&mut rng) as usize) % 4;
            let end = (i + step).min(flat.len());
            chunks.push(flat[i..end].to_vec());
            i = end;
        }

        assert_eq!(
            run_workers(chunks),
            expected,
            "run_workers не совпал с суммой плоского вектора {flat:?}"
        );
    }
}

/// Свойство (проще и без ручной нарезки): набор одноэлементных чанков суммируется
/// в сумму исходного вектора.
#[test]
fn prop_run_workers_singleton_chunks() {
    let mut rng: u64 = 0x2468_0ACE_1357_9BDF;
    for _ in 0..500 {
        let flat = random_vec(&mut rng, 50, -1000, 1000);
        let expected: i64 = flat.iter().sum();
        let chunks: Vec<Vec<i64>> = flat.iter().map(|&x| vec![x]).collect();
        assert_eq!(run_workers(chunks), expected);
    }
}

/// Свойство: вкрапление пустых чанков не меняет результат.
#[test]
fn prop_run_workers_empty_chunks_are_neutral() {
    let mut rng: u64 = 0x9999_1111_3333_5555;
    for _ in 0..400 {
        let flat = random_vec(&mut rng, 30, -500, 500);
        let expected: i64 = flat.iter().sum();

        // Чанки: каждый элемент в своём чанке, между ними — пустые чанки.
        let mut chunks: Vec<Vec<i64>> = Vec::new();
        for &x in &flat {
            chunks.push(vec![]);
            chunks.push(vec![x]);
        }
        chunks.push(vec![]);

        assert_eq!(run_workers(chunks), expected);
    }
}
