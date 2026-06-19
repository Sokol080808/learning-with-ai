// Randomized property tests for m00_setup.
// Uses a deterministic xorshift64* PRNG — no external crates, fully reproducible.

use m00_setup::*;

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

// --- add: oracle (std addition) agrees with add() for random inputs ---
#[test]
fn prop_add_matches_oracle() {
    let mut rng: u64 = 0xDEAD_BEEF_1234_5678;
    for _ in 0..1000 {
        let a = in_range(&mut rng, -10_000, 10_000);
        let b = in_range(&mut rng, -10_000, 10_000);
        let expected = a + b; // inline oracle: plain arithmetic
        assert_eq!(
            add(a, b),
            expected,
            "add({a}, {b}) should be {expected}"
        );
    }
}

// --- add: commutativity — add(a,b) == add(b,a) ---
#[test]
fn prop_add_commutative() {
    let mut rng: u64 = 0xCAFE_BABE_0000_0001;
    for _ in 0..1000 {
        let a = in_range(&mut rng, -10_000, 10_000);
        let b = in_range(&mut rng, -10_000, 10_000);
        assert_eq!(
            add(a, b),
            add(b, a),
            "add should be commutative for a={a}, b={b}"
        );
    }
}

// --- add: identity — add(n, 0) == n ---
#[test]
fn prop_add_identity_zero() {
    let mut rng: u64 = 0x0000_0000_ABCD_EF01;
    for _ in 0..1000 {
        let n = in_range(&mut rng, -10_000, 10_000);
        assert_eq!(add(n, 0), n, "add(n, 0) must equal n for n={n}");
        assert_eq!(add(0, n), n, "add(0, n) must equal n for n={n}");
    }
}

// --- add: inverse — add(n, -n) == 0 ---
#[test]
fn prop_add_self_negation() {
    let mut rng: u64 = 0x1111_2222_3333_4444;
    for _ in 0..1000 {
        let n = in_range(&mut rng, -10_000, 10_000);
        assert_eq!(add(n, -n), 0, "add(n, -n) must be 0 for n={n}");
    }
}

// --- add: sign of result matches sign of sum for large same-sign inputs ---
#[test]
fn prop_add_positive_inputs_give_positive_result() {
    let mut rng: u64 = 0xFEED_FACE_CAFE_0001;
    for _ in 0..500 {
        let a = in_range(&mut rng, 1, 10_000);
        let b = in_range(&mut rng, 1, 10_000);
        let result = add(a, b);
        assert!(result > 0, "add({a}, {b}) should be positive, got {result}");
    }
}

#[test]
fn prop_add_negative_inputs_give_negative_result() {
    let mut rng: u64 = 0xAAAA_BBBB_CCCC_DDDD;
    for _ in 0..500 {
        let a = in_range(&mut rng, -10_000, -1);
        let b = in_range(&mut rng, -10_000, -1);
        let result = add(a, b);
        assert!(result < 0, "add({a}, {b}) should be negative, got {result}");
    }
}

// --- add: explicit edge cases not covered by warmup.rs ---
#[test]
fn add_edge_cases() {
    // Large symmetric values
    assert_eq!(add(10_000, -10_000), 0);
    assert_eq!(add(-1, 1), 0);
    assert_eq!(add(i64::MAX / 2, i64::MAX / 2), i64::MAX - 1);
}

// --- seconds_in: oracle agrees with seconds_in() for random inputs ---
#[test]
fn prop_seconds_in_matches_oracle() {
    let mut rng: u64 = 0x5555_6666_7777_8888;
    for _ in 0..1000 {
        let h = in_range(&mut rng, 0, 10_000);
        let expected = 3600_i64 * h; // inline oracle
        assert_eq!(
            seconds_in(h),
            expected,
            "seconds_in({h}) should be {expected}"
        );
    }
}

// --- seconds_in: result is always a multiple of 3600 ---
#[test]
fn prop_seconds_in_divisible_by_3600() {
    let mut rng: u64 = 0x9999_AAAA_BBBB_CCCC;
    for _ in 0..1000 {
        let h = in_range(&mut rng, 0, 10_000);
        let s = seconds_in(h);
        assert_eq!(
            s % 3600,
            0,
            "seconds_in({h}) = {s} should be divisible by 3600"
        );
    }
}

// --- seconds_in: monotone — more hours means more seconds ---
#[test]
fn prop_seconds_in_monotone() {
    let mut rng: u64 = 0x1234_5678_9ABC_DEF0;
    for _ in 0..500 {
        let h1 = in_range(&mut rng, 0, 5_000);
        let h2 = in_range(&mut rng, h1 + 1, h1 + 5_000);
        assert!(
            seconds_in(h2) > seconds_in(h1),
            "seconds_in({h2}) should be greater than seconds_in({h1})"
        );
    }
}

// --- seconds_in: additive — seconds_in(a+b) == seconds_in(a) + seconds_in(b) ---
#[test]
fn prop_seconds_in_additive() {
    let mut rng: u64 = 0xDEAD_C0DE_BEEF_0001;
    for _ in 0..1000 {
        let a = in_range(&mut rng, 0, 5_000);
        let b = in_range(&mut rng, 0, 5_000);
        assert_eq!(
            seconds_in(a + b),
            seconds_in(a) + seconds_in(b),
            "seconds_in should be additive for a={a}, b={b}"
        );
    }
}

// --- seconds_in: explicit edge cases ---
#[test]
fn seconds_in_edge_cases() {
    assert_eq!(seconds_in(0), 0);
    assert_eq!(seconds_in(1), 3600);
    assert_eq!(seconds_in(24), 86_400);
    // negative hours (contract uses i64, so it should scale linearly)
    assert_eq!(seconds_in(-1), -3600);
    assert_eq!(seconds_in(-24), -86_400);
}
