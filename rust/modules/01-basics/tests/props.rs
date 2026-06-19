// Рандомизированные property-тесты для m01_basics.
// Не дублируют конкретные примеры из basics.rs — проверяют СВОЙСТВА
// для случайных входных значений.

use m01_basics::*;

// маленький детерминированный ГПСЧ (xorshift64*), без внешних крейтов — воспроизводимо.
// Используй ЭТОТ helper во всех property-тестах модуля. Сид ФИКСИРОВАННЫЙ (hardcode), чтобы прогон был детерминированным.
fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}
// целое в диапазоне [lo, hi] включительно (держи диапазон скромным, чтобы оракул не переполнял i64)
fn in_range(state: &mut u64, lo: i64, hi: i64) -> i64 {
    let span = (hi - lo + 1) as u64;
    lo + (next_u64(state) % span) as i64
}

// ── to_fahrenheit ─────────────────────────────────────────────────────────────

/// Свойство: to_fahrenheit линейна — oracle вычисляет ту же формулу независимо.
#[test]
fn prop_to_fahrenheit_matches_formula() {
    let mut rng: u64 = 0xDEAD_BEEF_CAFE_1234;
    for _ in 0..1000 {
        let c = in_range(&mut rng, -10_000, 10_000) as f64;
        let expected = c * 9.0 / 5.0 + 32.0;
        let got = to_fahrenheit(c);
        assert!(
            (got - expected).abs() < 1e-9,
            "to_fahrenheit({c}) = {got}, ожидали {expected}"
        );
    }
}

/// Свойство: преобразование обратимо — из Фаренгейта можно вернуться в Цельсий.
/// Формула обратного преобразования: C = (F - 32) * 5/9
#[test]
fn prop_to_fahrenheit_invertible() {
    let mut rng: u64 = 0x1234_5678_9ABC_DEF0;
    for _ in 0..1000 {
        let c = in_range(&mut rng, -10_000, 10_000) as f64;
        let f = to_fahrenheit(c);
        let c_back = (f - 32.0) * 5.0 / 9.0;
        assert!(
            (c - c_back).abs() < 1e-9,
            "round-trip failed: C={c} -> F={f} -> C={c_back}"
        );
    }
}

/// Свойство: to_fahrenheit строго монотонна — больший Цельсий → больший Фаренгейт.
#[test]
fn prop_to_fahrenheit_monotone() {
    let mut rng: u64 = 0xABCD_EF01_2345_6789;
    for _ in 0..500 {
        let a = in_range(&mut rng, -9_999, 0) as f64;
        let b = in_range(&mut rng, 1, 10_000) as f64;
        // a < b, значит to_fahrenheit(a) < to_fahrenheit(b)
        assert!(
            to_fahrenheit(a) < to_fahrenheit(b),
            "монотонность нарушена: f({a}) >= f({b})"
        );
    }
}

// ── is_even ───────────────────────────────────────────────────────────────────

/// Свойство: is_even совпадает с независимым оракулом `n % 2 == 0`.
#[test]
fn prop_is_even_matches_oracle() {
    let mut rng: u64 = 0xFEED_FACE_DEAD_BEEF;
    for _ in 0..1000 {
        let n = in_range(&mut rng, -10_000, 10_000);
        let oracle = n % 2 == 0;
        assert_eq!(
            is_even(n),
            oracle,
            "is_even({n}) = {}, oracle = {oracle}",
            is_even(n)
        );
    }
}

/// Свойство: чётность чередуется — n чётное тогда и только тогда, когда n+1 нечётное.
#[test]
fn prop_is_even_alternates() {
    let mut rng: u64 = 0x0102_0304_0506_0708;
    for _ in 0..1000 {
        let n = in_range(&mut rng, -9_999, 9_999);
        assert_ne!(
            is_even(n),
            is_even(n + 1),
            "n={n}: четность n и n+1 должна чередоваться"
        );
    }
}

/// Свойство: n*2 всегда чётное; n*2+1 всегда нечётное.
#[test]
fn prop_is_even_doubled_and_odd() {
    let mut rng: u64 = 0xCAFE_BABE_1337_4242;
    for _ in 0..1000 {
        let n = in_range(&mut rng, -5_000, 5_000);
        assert!(is_even(n * 2), "n*2={} должно быть чётным", n * 2);
        assert!(!is_even(n * 2 + 1), "n*2+1={} должно быть нечётным", n * 2 + 1);
    }
}

// ── min_of_three ──────────────────────────────────────────────────────────────

/// Свойство: результат не превышает ни одного из трёх аргументов.
#[test]
fn prop_min_of_three_le_all() {
    let mut rng: u64 = 0x1111_2222_3333_4444;
    for _ in 0..1000 {
        let a = in_range(&mut rng, -10_000, 10_000);
        let b = in_range(&mut rng, -10_000, 10_000);
        let c = in_range(&mut rng, -10_000, 10_000);
        let m = min_of_three(a, b, c);
        assert!(m <= a, "min_of_three({a},{b},{c})={m} > a={a}");
        assert!(m <= b, "min_of_three({a},{b},{c})={m} > b={b}");
        assert!(m <= c, "min_of_three({a},{b},{c})={m} > c={c}");
    }
}

/// Свойство: результат совпадает с оракулом std::cmp::min трёх значений.
#[test]
fn prop_min_of_three_matches_oracle() {
    let mut rng: u64 = 0x5555_6666_7777_8888;
    for _ in 0..1000 {
        let a = in_range(&mut rng, -10_000, 10_000);
        let b = in_range(&mut rng, -10_000, 10_000);
        let c = in_range(&mut rng, -10_000, 10_000);
        let oracle = a.min(b.min(c));
        let got = min_of_three(a, b, c);
        assert_eq!(got, oracle, "min_of_three({a},{b},{c}): got {got}, oracle {oracle}");
    }
}

/// Свойство: min_of_three симметрична — порядок аргументов не меняет результат.
#[test]
fn prop_min_of_three_symmetric() {
    let mut rng: u64 = 0x9999_AAAA_BBBB_CCCC;
    for _ in 0..500 {
        let a = in_range(&mut rng, -10_000, 10_000);
        let b = in_range(&mut rng, -10_000, 10_000);
        let c = in_range(&mut rng, -10_000, 10_000);
        let reference = min_of_three(a, b, c);
        // все 6 перестановок должны дать тот же результат
        assert_eq!(min_of_three(a, c, b), reference, "перестановка (a,c,b)");
        assert_eq!(min_of_three(b, a, c), reference, "перестановка (b,a,c)");
        assert_eq!(min_of_three(b, c, a), reference, "перестановка (b,c,a)");
        assert_eq!(min_of_three(c, a, b), reference, "перестановка (c,a,b)");
        assert_eq!(min_of_three(c, b, a), reference, "перестановка (c,b,a)");
    }
}

/// Свойство: результат равен одному из аргументов (не «придуман»).
#[test]
fn prop_min_of_three_is_one_of_inputs() {
    let mut rng: u64 = 0xDDDD_EEEE_FFFF_0000;
    for _ in 0..1000 {
        let a = in_range(&mut rng, -10_000, 10_000);
        let b = in_range(&mut rng, -10_000, 10_000);
        let c = in_range(&mut rng, -10_000, 10_000);
        let m = min_of_three(a, b, c);
        assert!(
            m == a || m == b || m == c,
            "min_of_three({a},{b},{c})={m} не является ни одним из входов"
        );
    }
}

// ── abs_diff ──────────────────────────────────────────────────────────────────

/// Свойство: результат всегда неотрицателен.
#[test]
fn prop_abs_diff_nonnegative() {
    let mut rng: u64 = 0x0A0B_0C0D_0E0F_0102;
    for _ in 0..1000 {
        let a = in_range(&mut rng, -10_000, 10_000);
        let b = in_range(&mut rng, -10_000, 10_000);
        let d = abs_diff(a, b);
        assert!(d >= 0, "abs_diff({a},{b}) = {d} < 0");
    }
}

/// Свойство: симметричность — abs_diff(a,b) == abs_diff(b,a).
#[test]
fn prop_abs_diff_symmetric() {
    let mut rng: u64 = 0x1A2B_3C4D_5E6F_7081;
    for _ in 0..1000 {
        let a = in_range(&mut rng, -10_000, 10_000);
        let b = in_range(&mut rng, -10_000, 10_000);
        assert_eq!(
            abs_diff(a, b),
            abs_diff(b, a),
            "симметричность нарушена для a={a}, b={b}"
        );
    }
}

/// Свойство: abs_diff совпадает с оракулом (a - b).abs() через i64::abs.
#[test]
fn prop_abs_diff_matches_oracle() {
    let mut rng: u64 = 0x2B3C_4D5E_6F70_8192;
    for _ in 0..1000 {
        let a = in_range(&mut rng, -10_000, 10_000);
        let b = in_range(&mut rng, -10_000, 10_000);
        let oracle = (a - b).abs();
        let got = abs_diff(a, b);
        assert_eq!(got, oracle, "abs_diff({a},{b}): got {got}, oracle {oracle}");
    }
}

/// Свойство: abs_diff(n, n) == 0 для любого n.
#[test]
fn prop_abs_diff_self_is_zero() {
    let mut rng: u64 = 0x3C4D_5E6F_7081_92A3;
    for _ in 0..1000 {
        let n = in_range(&mut rng, -10_000, 10_000);
        assert_eq!(abs_diff(n, n), 0, "abs_diff({n},{n}) должно быть 0");
    }
}

/// Свойство: abs_diff(a, b) == |a - b| == abs_diff(a+k, b+k) (трансляционная инвариантность).
#[test]
fn prop_abs_diff_translation_invariant() {
    let mut rng: u64 = 0x4D5E_6F70_8192_A3B4;
    for _ in 0..500 {
        let a = in_range(&mut rng, -5_000, 5_000);
        let b = in_range(&mut rng, -5_000, 5_000);
        let k = in_range(&mut rng, -1_000, 1_000);
        assert_eq!(
            abs_diff(a, b),
            abs_diff(a + k, b + k),
            "трансляция на k={k}: abs_diff({a},{b}) != abs_diff({},{})",
            a + k,
            b + k
        );
    }
}
