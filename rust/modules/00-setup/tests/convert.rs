// Тесты для функции `convert` и перечисления `Unit` (модуль 00-setup).
//
// Два типа тестов:
//   (a) детерминированные примеры — конкретные входы → ожидаемые выходы;
//   (b) случайные property-тесты — инварианты/round-trip на ~500-1000 входах.
//
// ГПСЧ: тот же xorshift64*, что в props.rs. Сид жёстко зафиксирован — прогон
// воспроизводим и независим от внешних крейтов.

use m00_setup::{convert, Unit};
use Unit::*;

// ---------------------------------------------------------------------------
// Детерминированный ГПСЧ (xorshift64*) — только для property-тестов
// ---------------------------------------------------------------------------

fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}

/// Случайное `i64` в диапазоне [lo, hi] включительно.
/// Держи диапазоны скромными — чтобы промежуточное умножение не переполняло i64.
fn in_range(state: &mut u64, lo: i64, hi: i64) -> i64 {
    let span = (hi - lo + 1) as u64;
    lo + (next_u64(state) % span) as i64
}

// ---------------------------------------------------------------------------
// (a) Детерминированные примеры — из спецификации + граничные случаи
// ---------------------------------------------------------------------------

#[test]
fn convert_hours_to_minutes() {
    assert_eq!(convert(1, Hours, Minutes), 60);
    assert_eq!(convert(2, Hours, Minutes), 120);
    assert_eq!(convert(0, Hours, Minutes), 0);
}

#[test]
fn convert_days_to_seconds() {
    assert_eq!(convert(2, Days, Seconds), 172_800);
    assert_eq!(convert(1, Days, Seconds), 86_400);
    assert_eq!(convert(0, Days, Seconds), 0);
}

#[test]
fn convert_seconds_to_hours() {
    assert_eq!(convert(3600, Seconds, Hours), 1);
    assert_eq!(convert(7200, Seconds, Hours), 2);
    assert_eq!(convert(0,    Seconds, Hours), 0);
}

#[test]
fn convert_same_unit_is_identity() {
    // Перевод в ту же единицу всегда возвращает исходное значение
    assert_eq!(convert(0,      Seconds, Seconds), 0);
    assert_eq!(convert(42,     Minutes, Minutes), 42);
    assert_eq!(convert(7,      Hours,   Hours),   7);
    assert_eq!(convert(365,    Days,    Days),     365);
}

#[test]
fn convert_minutes_to_seconds() {
    assert_eq!(convert(1,  Minutes, Seconds), 60);
    assert_eq!(convert(60, Minutes, Seconds), 3_600);
    assert_eq!(convert(0,  Minutes, Seconds), 0);
}

#[test]
fn convert_hours_to_seconds() {
    assert_eq!(convert(1,  Hours, Seconds), 3_600);
    assert_eq!(convert(24, Hours, Seconds), 86_400);
}

#[test]
fn convert_days_to_hours() {
    assert_eq!(convert(1, Days, Hours), 24);
    assert_eq!(convert(2, Days, Hours), 48);
    assert_eq!(convert(0, Days, Hours), 0);
}

#[test]
fn convert_days_to_minutes() {
    assert_eq!(convert(1, Days, Minutes), 1_440); // 24 * 60
    assert_eq!(convert(0, Days, Minutes), 0);
}

#[test]
fn convert_seconds_to_minutes_truncates() {
    // Целочисленное деление: дробная часть отбрасывается
    assert_eq!(convert(90,  Seconds, Minutes), 1); // 90/60 = 1.5 → 1
    assert_eq!(convert(59,  Seconds, Minutes), 0); // 59/60 = 0.98… → 0
    assert_eq!(convert(120, Seconds, Minutes), 2);
}

#[test]
fn convert_seconds_to_days_truncates() {
    assert_eq!(convert(86_399, Seconds, Days), 0); // чуть меньше суток → 0
    assert_eq!(convert(86_400, Seconds, Days), 1);
    assert_eq!(convert(86_401, Seconds, Days), 1);
}

#[test]
fn convert_zero_any_pair() {
    // Ноль в любой единице — всегда ноль
    let units = [Seconds, Minutes, Hours, Days];
    for &from in &units {
        for &to in &units {
            assert_eq!(
                convert(0, from, to),
                0,
                "convert(0, {from:?}, {to:?}) must be 0"
            );
        }
    }
}

// ---------------------------------------------------------------------------
// (b) Property-тесты — инварианты и round-trip
// ---------------------------------------------------------------------------

/// Инвариант: перевод в ту же единицу = тождество для любого значения
#[test]
fn prop_same_unit_identity() {
    let mut rng: u64 = 0xABCD_1234_EF56_7890;
    let units = [Seconds, Minutes, Hours, Days];
    for _ in 0..500 {
        let v = in_range(&mut rng, 0, 10_000);
        for &u in &units {
            assert_eq!(
                convert(v, u, u),
                v,
                "convert({v}, {u:?}, {u:?}) should be {v}"
            );
        }
    }
}

/// Round-trip: convert(convert(v, A→B), B→A) == v  (только когда нет потерь при делении)
/// Чтобы избежать потерь, берём кратные значения:
///   Seconds→Minutes: значение кратно 60
///   Seconds→Hours:   значение кратно 3600
///   Minutes→Hours:   значение кратно 60
#[test]
fn prop_round_trip_seconds_minutes() {
    let mut rng: u64 = 0x1111_AAAA_2222_BBBB;
    for _ in 0..500 {
        // Берём кратное 60 число секунд (диапазон 0..600 минут)
        let minutes = in_range(&mut rng, 0, 600);
        let seconds = minutes * 60;
        let back = convert(convert(seconds, Seconds, Minutes), Minutes, Seconds);
        assert_eq!(
            back, seconds,
            "round-trip Seconds→Minutes→Seconds failed for {seconds}s"
        );
    }
}

#[test]
fn prop_round_trip_minutes_hours() {
    let mut rng: u64 = 0xCCCC_3333_DDDD_4444;
    for _ in 0..500 {
        let hours = in_range(&mut rng, 0, 500);
        let minutes = hours * 60;
        let back = convert(convert(minutes, Minutes, Hours), Hours, Minutes);
        assert_eq!(
            back, minutes,
            "round-trip Minutes→Hours→Minutes failed for {minutes} min"
        );
    }
}

#[test]
fn prop_round_trip_hours_days() {
    let mut rng: u64 = 0xEEEE_5555_FFFF_6666;
    for _ in 0..500 {
        let days = in_range(&mut rng, 0, 365);
        let hours = days * 24;
        let back = convert(convert(hours, Hours, Days), Days, Hours);
        assert_eq!(
            back, hours,
            "round-trip Hours→Days→Hours failed for {hours} h"
        );
    }
}

/// Инвариант монотонности: если a > b, то convert(a, U, V) >= convert(b, U, V)
/// (нестрогое неравенство из-за усечения при делении)
#[test]
fn prop_monotone() {
    let mut rng: u64 = 0x7777_8888_9999_AAAA;
    let units = [Seconds, Minutes, Hours, Days];
    for _ in 0..200 {
        let a = in_range(&mut rng, 1, 5_000);
        let b = in_range(&mut rng, 0, a - 1); // b < a
        let ui = next_u64(&mut rng) as usize % units.len();
        let uj = next_u64(&mut rng) as usize % units.len();
        let from = units[ui];
        let to   = units[uj];
        let ca = convert(a, from, to);
        let cb = convert(b, from, to);
        assert!(
            ca >= cb,
            "monotone violated: convert({a},{from:?},{to:?})={ca} < convert({b},{from:?},{to:?})={cb}"
        );
    }
}

/// Oracle: convert через секунды вручную совпадает с реализацией
#[test]
fn prop_oracle_vs_naive() {
    fn to_seconds(v: i64, u: Unit) -> i64 {
        match u {
            Seconds => v,
            Minutes => v * 60,
            Hours   => v * 3_600,
            Days    => v * 86_400,
        }
    }
    fn from_seconds(s: i64, u: Unit) -> i64 {
        match u {
            Seconds => s,
            Minutes => s / 60,
            Hours   => s / 3_600,
            Days    => s / 86_400,
        }
    }

    let mut rng: u64 = 0xBEEF_CAFE_0123_4567;
    let units = [Seconds, Minutes, Hours, Days];
    for _ in 0..500 {
        let v = in_range(&mut rng, 0, 10_000);
        let ui = next_u64(&mut rng) as usize % units.len();
        let uj = next_u64(&mut rng) as usize % units.len();
        let from = units[ui];
        let to   = units[uj];
        let expected = from_seconds(to_seconds(v, from), to);
        assert_eq!(
            convert(v, from, to),
            expected,
            "convert({v}, {from:?}, {to:?}) mismatch"
        );
    }
}
