// Тесты для составных типов модуля 01: кортеж (minmax) и массив (first_n_squares).
// Трогать не нужно — это эталон поведения. На заглушке `todo!()` они красные
// (паника), на правильной реализации — зелёные.
//
// Внизу файла — рандомизированные property-тесты на детерминированном ГПСЧ
// (xorshift64*) без внешних крейтов: сид ФИКСИРОВАННЫЙ, прогон воспроизводим.

use m01_basics::*;

// ===========================================================================
// minmax — детерминированные примеры
// ===========================================================================

#[test]
fn minmax_orders_pair() {
    assert_eq!(minmax(3, 7), (3, 7));
    assert_eq!(minmax(7, 3), (3, 7)); // порядок аргументов не важен
}

#[test]
fn minmax_equal_arguments() {
    assert_eq!(minmax(5, 5), (5, 5));
    assert_eq!(minmax(0, 0), (0, 0));
}

#[test]
fn minmax_negatives() {
    assert_eq!(minmax(-1, -7), (-7, -1));
    assert_eq!(minmax(-5, 5), (-5, 5));
}

#[test]
fn minmax_tuple_field_access() {
    // Доступ к полям кортежа через .0 и .1 (не [0]/[1]).
    let pair = minmax(10, 2);
    assert_eq!(pair.0, 2);
    assert_eq!(pair.1, 10);
}

#[test]
fn minmax_destructuring() {
    // Канонический способ разобрать кортеж: let (lo, hi) = ...
    let (lo, hi) = minmax(8, 3);
    assert_eq!(lo, 3);
    assert_eq!(hi, 8);
}

// ===========================================================================
// first_n_squares — детерминированные примеры
// ===========================================================================

#[test]
fn first_n_squares_partial_fill() {
    assert_eq!(first_n_squares(3), [1, 4, 9, 0, 0, 0, 0, 0]);
}

#[test]
fn first_n_squares_zero_is_all_zeros() {
    assert_eq!(first_n_squares(0), [0; 8]);
}

#[test]
fn first_n_squares_full_fill() {
    assert_eq!(first_n_squares(8), [1, 4, 9, 16, 25, 36, 49, 64]);
}

#[test]
fn first_n_squares_length_is_always_eight() {
    // Длина — часть типа [i64; 8]; проверяем явно для разных n.
    for n in 0..=8 {
        assert_eq!(first_n_squares(n).len(), 8);
    }
}

#[test]
fn first_n_squares_one_element() {
    assert_eq!(first_n_squares(1), [1, 0, 0, 0, 0, 0, 0, 0]);
}

// ===========================================================================
// Детерминированный ГПСЧ (xorshift64*) — без внешних крейтов.
// Сид ФИКСИРОВАННЫЙ, прогон детерминирован.
// ===========================================================================

/// маленький детерминированный ГПСЧ (xorshift64*), воспроизводимо.
fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}

/// целое в диапазоне [lo, hi] включительно (диапазон держим скромным, чтобы
/// оракул `a + b` не переполнял i64).
fn in_range(state: &mut u64, lo: i64, hi: i64) -> i64 {
    let span = (hi - lo + 1) as u64;
    lo + (next_u64(state) % span) as i64
}

// ===========================================================================
// Property-тесты для minmax
// ===========================================================================

/// Свойство: первая позиция <= второй (min <= max) для любых аргументов.
#[test]
fn prop_minmax_first_le_second() {
    let mut rng: u64 = 0x01BA_5105_DEAD_C0DE;
    for _ in 0..500 {
        let a = in_range(&mut rng, -10_000, 10_000);
        let b = in_range(&mut rng, -10_000, 10_000);
        let (lo, hi) = minmax(a, b);
        assert!(lo <= hi, "minmax({a}, {b}) = ({lo}, {hi}): ожидали lo <= hi");
    }
}

/// Свойство: симметрия — minmax(a, b) == minmax(b, a).
#[test]
fn prop_minmax_symmetric() {
    let mut rng: u64 = 0x5117_5117_5117_5117;
    for _ in 0..500 {
        let a = in_range(&mut rng, -10_000, 10_000);
        let b = in_range(&mut rng, -10_000, 10_000);
        assert_eq!(
            minmax(a, b),
            minmax(b, a),
            "minmax не симметрична на ({a}, {b})"
        );
    }
}

/// Свойство (инвариант суммы): .0 + .1 == a + b — переставили, ничего не потеряли.
/// Диапазон скромный, чтобы a + b не переполнял i64.
#[test]
fn prop_minmax_sum_invariant() {
    let mut rng: u64 = 0xA11C_E55_5022_DEAD;
    for _ in 0..500 {
        let a = in_range(&mut rng, -10_000, 10_000);
        let b = in_range(&mut rng, -10_000, 10_000);
        let (lo, hi) = minmax(a, b);
        assert_eq!(lo + hi, a + b, "сумма пары для ({a}, {b}) не совпала с a + b");
    }
}

/// Свойство: пара minmax — это в точности {a, b} как множество
/// (lo — один из аргументов, hi — другой; их множество совпадает с {a, b}).
#[test]
fn prop_minmax_is_a_permutation_of_inputs() {
    let mut rng: u64 = 0x7E57_0F75_0117_0001;
    for _ in 0..500 {
        let a = in_range(&mut rng, -10_000, 10_000);
        let b = in_range(&mut rng, -10_000, 10_000);
        let (lo, hi) = minmax(a, b);
        // lo и hi — это {a, b} (в каком-то порядке)
        let mut got = [lo, hi];
        let mut want = [a, b];
        got.sort();
        want.sort();
        assert_eq!(got, want, "minmax({a}, {b}) = ({lo}, {hi}) — не перестановка входов");
    }
}

// ===========================================================================
// Property-тесты для first_n_squares
// ===========================================================================

/// Свойство: первые n позиций — точные квадраты (i+1)², остальные — нули,
/// длина всегда 8.
#[test]
fn prop_first_n_squares_shape() {
    let mut rng: u64 = 0x0F1A_2B3C_4D5E_6F70;
    for _ in 0..500 {
        let n = (next_u64(&mut rng) % 9) as usize; // n in 0..=8
        let arr = first_n_squares(n);
        assert_eq!(arr.len(), 8, "длина массива всегда 8");
        for (i, &val) in arr.iter().enumerate() {
            if i < n {
                let k = i as i64 + 1;
                assert_eq!(val, k * k, "позиция {i} при n={n}: ожидали квадрат {}", k * k);
            } else {
                assert_eq!(val, 0, "позиция {i} при n={n}: ожидали 0 (хвост)");
            }
        }
    }
}

/// Свойство: ровно n ненулевых позиций (квадраты строго положительны),
/// значит число нулей == 8 - n.
#[test]
fn prop_first_n_squares_count_nonzero() {
    let mut rng: u64 = 0xC0C0_1234_5678_9ABC;
    for _ in 0..500 {
        let n = (next_u64(&mut rng) % 9) as usize; // n in 0..=8
        let arr = first_n_squares(n);
        let nonzero = arr.iter().filter(|&&x| x != 0).count();
        assert_eq!(nonzero, n, "ожидали ровно {n} ненулевых элементов при n={n}");
    }
}
