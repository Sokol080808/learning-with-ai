// Эти тесты трогать не нужно — это эталон поведения новой задачи модуля 14.
// Запуск: ./rust/run.sh 14   (или `cargo test -p m14_tooling`)
//
// Помимо этих интеграционных тестов у `celsius_to_fahrenheit` есть DOC-ТЕСТ
// в самой функции (в `src/lib.rs`). Прогнать именно его:
//     cargo test -p m14_tooling --doc
// На стабе с `todo!()` doc-тест краснеет паникой; на починенной функции — зелёный.

use m14_tooling::*;

// ---------------------------------------------------------------------------
// celsius_to_fahrenheit — детерминированные примеры
// ---------------------------------------------------------------------------

#[test]
fn c2f_freezing_point() {
    // 0 °C — точка замерзания воды — это 32 °F.
    assert_eq!(celsius_to_fahrenheit(0.0), 32.0);
}

#[test]
fn c2f_boiling_point() {
    // 100 °C — кипение воды — это 212 °F.
    assert_eq!(celsius_to_fahrenheit(100.0), 212.0);
}

#[test]
fn c2f_the_crossover_point() {
    // -40 — единственная точка, где шкалы Цельсия и Фаренгейта совпадают.
    assert_eq!(celsius_to_fahrenheit(-40.0), -40.0);
}

#[test]
fn c2f_body_temperature() {
    // 37 °C ≈ 98.6 °F. Здесь результат дробный — сравниваем с допуском, а не ==.
    let f = celsius_to_fahrenheit(37.0);
    assert!((f - 98.6).abs() < 1e-9, "37C -> 98.6F expected, got {f}");
}

// ---------------------------------------------------------------------------
// checked_div — паника как документированный контракт (#[should_panic])
// ---------------------------------------------------------------------------

#[test]
fn checked_div_normal() {
    assert_eq!(checked_div(10, 2), 5);
    assert_eq!(checked_div(-9, 3), -3);
}

#[test]
#[should_panic(expected = "division by zero")]
fn checked_div_by_zero_panics() {
    // Деление на ноль — нарушение инварианта; функция обязана паниковать.
    // #[should_panic(expected = "...")] проверяет и факт паники, и её сообщение.
    let _ = checked_div(1, 0);
}

// ---------------------------------------------------------------------------
// Property-тесты (детерминированный xorshift64*, без внешних крейтов)
// ---------------------------------------------------------------------------

/// маленький детерминированный ГПСЧ (xorshift64*) — воспроизводимо, сид фиксирован.
fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}

/// целое в диапазоне [lo, hi] включительно (держи диапазон скромным).
fn in_range(state: &mut u64, lo: i64, hi: i64) -> i64 {
    let span = (hi - lo + 1) as u64;
    lo + (next_u64(state) % span) as i64
}

/// Property: celsius_to_fahrenheit совпадает с оракулом `c * 9/5 + 32`.
#[test]
fn prop_c2f_matches_oracle() {
    let mut rng: u64 = 0xC0FF_EE00_1234_5678;
    for _ in 0..800 {
        // целочисленные градусы в скромном диапазоне → f64 точно, можно ==
        let c = in_range(&mut rng, -1000, 1000) as f64;
        let expected = c * 9.0 / 5.0 + 32.0;
        assert_eq!(
            celsius_to_fahrenheit(c),
            expected,
            "c2f({c}) != oracle {expected}"
        );
    }
}

/// Property: формула монотонна — больший Цельсий даёт больший Фаренгейт.
#[test]
fn prop_c2f_is_monotonic() {
    let mut rng: u64 = 0x1357_9BDF_2468_ACE0;
    for _ in 0..500 {
        let a = in_range(&mut rng, -1000, 1000) as f64;
        let b = in_range(&mut rng, -1000, 1000) as f64;
        let fa = celsius_to_fahrenheit(a);
        let fb = celsius_to_fahrenheit(b);
        if a < b {
            assert!(fa < fb, "monotonicity broke: c2f({a})={fa}, c2f({b})={fb}");
        } else if a > b {
            assert!(fa > fb, "monotonicity broke: c2f({a})={fa}, c2f({b})={fb}");
        } else {
            assert_eq!(fa, fb);
        }
    }
}

/// Property: checked_div совпадает с обычным целочисленным делением для b != 0.
#[test]
fn prop_checked_div_matches_integer_division() {
    let mut rng: u64 = 0xDEAD_0BAD_F00D_CAFE;
    for _ in 0..800 {
        let a = in_range(&mut rng, -100_000, 100_000);
        // b в [-1000, 1000] БЕЗ нуля
        let mut b = in_range(&mut rng, -1000, 1000);
        if b == 0 {
            b = 1;
        }
        assert_eq!(checked_div(a, b), a / b, "checked_div({a}, {b}) != a/b");
    }
}
