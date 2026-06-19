// Randomised property tests for m06_enums.
// All tests are DETERMINISTIC: fixed seeds + xorshift64* PRNG, no external crates.
// These run green on the reference solution and stay RED on the todo!() stub.

use m06_enums::*;

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

// ── Shape::area ─────────────────────────────────────────────────────────────

/// Rect area always equals w*h (oracle: straightforward multiplication).
#[test]
fn prop_rect_area_equals_w_times_h() {
    let mut rng: u64 = 0xDEAD_BEEF_1234_5678;
    for _ in 0..1000 {
        let w = in_range(&mut rng, 0, 10_000) as f64 / 100.0;
        let h = in_range(&mut rng, 0, 10_000) as f64 / 100.0;
        let shape = Shape::Rect(w, h);
        let expected = w * h;
        let got = shape.area();
        assert!(
            (got - expected).abs() < 1e-9,
            "Rect({w},{h}).area() = {got}, expected {expected}"
        );
    }
}

/// Rect area is always >= 0 for non-negative dimensions.
#[test]
fn prop_rect_area_nonnegative() {
    let mut rng: u64 = 0xCAFE_BABE_0000_0001;
    for _ in 0..1000 {
        let w = in_range(&mut rng, 0, 10_000) as f64 / 100.0;
        let h = in_range(&mut rng, 0, 10_000) as f64 / 100.0;
        assert!(Shape::Rect(w, h).area() >= 0.0);
    }
}

/// Circle area == PI * r^2 (oracle computed inline).
#[test]
fn prop_circle_area_equals_pi_r_squared() {
    let mut rng: u64 = 0x0123_4567_89AB_CDEF;
    for _ in 0..1000 {
        let r = in_range(&mut rng, 0, 10_000) as f64 / 100.0;
        let expected = std::f64::consts::PI * r * r;
        let got = Shape::Circle(r).area();
        assert!(
            (got - expected).abs() < 1e-9,
            "Circle({r}).area() = {got}, expected {expected}"
        );
    }
}

/// Circle area is always >= 0 for non-negative radii.
#[test]
fn prop_circle_area_nonnegative() {
    let mut rng: u64 = 0xFEDC_BA98_7654_3210;
    for _ in 0..1000 {
        let r = in_range(&mut rng, 0, 10_000) as f64 / 100.0;
        assert!(Shape::Circle(r).area() >= 0.0);
    }
}

/// Doubling radius quadruples circle area.
#[test]
fn prop_circle_area_scales_quadratically_with_radius() {
    let mut rng: u64 = 0x1111_2222_3333_4444;
    for _ in 0..500 {
        let r = in_range(&mut rng, 1, 1_000) as f64 / 100.0;
        let a1 = Shape::Circle(r).area();
        let a2 = Shape::Circle(2.0 * r).area();
        // a2 should be 4 * a1
        assert!(
            (a2 - 4.0 * a1).abs() < 1e-6,
            "Circle({r}): area*4 = {}, Circle({}): area = {a2}",
            4.0 * a1,
            2.0 * r
        );
    }
}

/// Scaling both Rect dimensions by k scales area by k^2.
#[test]
fn prop_rect_area_scales_with_dimensions() {
    let mut rng: u64 = 0xAAAA_BBBB_CCCC_DDDD;
    for _ in 0..500 {
        let w = in_range(&mut rng, 1, 1_000) as f64 / 100.0;
        let h = in_range(&mut rng, 1, 1_000) as f64 / 100.0;
        let k = in_range(&mut rng, 1, 10) as f64;
        let a1 = Shape::Rect(w, h).area();
        let a2 = Shape::Rect(w * k, h * k).area();
        let expected = a1 * k * k;
        assert!(
            (a2 - expected).abs() < 1e-6,
            "Rect({w},{h}) scaled by {k}: expected {expected}, got {a2}"
        );
    }
}

// ── safe_div ─────────────────────────────────────────────────────────────────

/// safe_div(a, b) with b != 0 always returns Some and the value equals a/b.
#[test]
fn prop_safe_div_nonzero_gives_some_with_correct_value() {
    let mut rng: u64 = 0x9999_8888_7777_6666;
    for _ in 0..1000 {
        // pick b in [-1000, -1] ∪ [1, 1000] — never zero
        let b_raw = in_range(&mut rng, 1, 1000) as f64;
        let sign = if next_u64(&mut rng) & 1 == 0 { 1.0_f64 } else { -1.0_f64 };
        let b = b_raw * sign;
        let a = in_range(&mut rng, -10_000, 10_000) as f64;
        let result = safe_div(a, b);
        assert!(result.is_some(), "safe_div({a}, {b}) should be Some");
        let got = result.unwrap();
        let expected = a / b;
        assert!(
            (got - expected).abs() < 1e-9,
            "safe_div({a}, {b}) = {got}, expected {expected}"
        );
    }
}

/// safe_div(a, 0.0) always returns None, for any a.
#[test]
fn prop_safe_div_zero_denominator_is_none() {
    let mut rng: u64 = 0x5555_4444_3333_2222;
    for _ in 0..1000 {
        let a = in_range(&mut rng, -10_000, 10_000) as f64;
        assert_eq!(
            safe_div(a, 0.0),
            None,
            "safe_div({a}, 0.0) should be None"
        );
    }
}

/// Round-trip: if safe_div returns Some(q) then q * b ≈ a.
#[test]
fn prop_safe_div_round_trip_multiplication() {
    let mut rng: u64 = 0x1A2B_3C4D_5E6F_7A8B;
    for _ in 0..1000 {
        let b = in_range(&mut rng, 1, 10_000) as f64 / 100.0 + 0.01; // strictly positive
        let a = in_range(&mut rng, -10_000, 10_000) as f64;
        if let Some(q) = safe_div(a, b) {
            let reconstructed = q * b;
            assert!(
                (reconstructed - a).abs() < 1e-6,
                "safe_div({a},{b})={q}, but {q}*{b}={reconstructed} != {a}"
            );
        }
    }
}

// ── first_even ───────────────────────────────────────────────────────────────

/// If first_even returns Some(n), then n is even.
#[test]
fn prop_first_even_result_is_always_even() {
    let mut rng: u64 = 0xBEEF_CAFE_DEAD_F00D;
    for _ in 0..1000 {
        let len = in_range(&mut rng, 0, 20) as usize;
        let v: Vec<i64> = (0..len).map(|_| in_range(&mut rng, -10_000, 10_000)).collect();
        if let Some(n) = first_even(&v) {
            assert_eq!(n % 2, 0, "first_even returned {n} which is odd; slice = {v:?}");
        }
    }
}

/// If first_even returns Some(n), n actually appears in the slice.
#[test]
fn prop_first_even_result_is_in_slice() {
    let mut rng: u64 = 0x1234_ABCD_EF98_7654;
    for _ in 0..1000 {
        let len = in_range(&mut rng, 1, 20) as usize;
        let v: Vec<i64> = (0..len).map(|_| in_range(&mut rng, -10_000, 10_000)).collect();
        if let Some(n) = first_even(&v) {
            assert!(
                v.contains(&n),
                "first_even returned {n} but it's not in {v:?}"
            );
        }
    }
}

/// If first_even returns Some(n), every element before n in the slice must be odd.
#[test]
fn prop_first_even_is_truly_first() {
    let mut rng: u64 = 0xF0F0_F0F0_1234_5678;
    for _ in 0..1000 {
        let len = in_range(&mut rng, 1, 20) as usize;
        let v: Vec<i64> = (0..len).map(|_| in_range(&mut rng, -10_000, 10_000)).collect();
        if let Some(n) = first_even(&v) {
            let pos = v.iter().position(|&x| x == n).unwrap();
            for i in 0..pos {
                assert_ne!(
                    v[i] % 2, 0,
                    "element at index {i} (={}) is even, but first_even returned index {pos} (={n}); slice={v:?}",
                    v[i]
                );
            }
        }
    }
}

/// If no element in the slice is even, first_even returns None.
/// Oracle: count of even elements via stdlib.
#[test]
fn prop_first_even_none_iff_no_evens_in_slice() {
    let mut rng: u64 = 0x0F0F_0F0F_ABCD_1234;
    for _ in 0..1000 {
        // Build slices of odd numbers only to guarantee None.
        let len = in_range(&mut rng, 0, 20) as usize;
        // Odd numbers: 2k+1 for k in [0,9999] → range [-9999, 9999] odd
        let v: Vec<i64> = (0..len)
            .map(|_| {
                let k = in_range(&mut rng, -5_000, 4_999);
                2 * k + 1 // always odd
            })
            .collect();
        assert_eq!(
            first_even(&v),
            None,
            "first_even should be None for all-odd slice {v:?}"
        );
    }
}

/// Cross-check against std oracle: first_even agrees with iter().find(|&&n| n%2==0).
#[test]
fn prop_first_even_agrees_with_std_find_oracle() {
    let mut rng: u64 = 0xA5A5_5A5A_B3B3_3B3B;
    for _ in 0..1000 {
        let len = in_range(&mut rng, 0, 20) as usize;
        let v: Vec<i64> = (0..len).map(|_| in_range(&mut rng, -10_000, 10_000)).collect();
        let oracle = v.iter().copied().find(|&n| n % 2 == 0);
        let got = first_even(&v);
        assert_eq!(
            got, oracle,
            "first_even({v:?}) = {got:?}, oracle = {oracle:?}"
        );
    }
}

// ── describe ─────────────────────────────────────────────────────────────────

/// describe(Some(n)) always starts with "value: ".
#[test]
fn prop_describe_some_starts_with_value_prefix() {
    let mut rng: u64 = 0x7777_8888_9999_AAAA;
    for _ in 0..1000 {
        let n = in_range(&mut rng, -10_000, 10_000);
        let s = describe(Some(n));
        assert!(
            s.starts_with("value: "),
            "describe(Some({n})) = {s:?}, expected prefix \"value: \""
        );
    }
}

/// describe(Some(n)) contains the decimal representation of n.
#[test]
fn prop_describe_some_contains_number() {
    let mut rng: u64 = 0x1B2B_3B4B_5B6B_7B8B;
    for _ in 0..1000 {
        let n = in_range(&mut rng, -10_000, 10_000);
        let s = describe(Some(n));
        let expected = format!("value: {n}");
        assert_eq!(
            s, expected,
            "describe(Some({n})) = {s:?}, expected {expected:?}"
        );
    }
}

/// describe(None) always returns exactly "nothing".
#[test]
fn prop_describe_none_is_always_nothing() {
    // No randomness needed: describe(None) is a constant. Checked 1000 times for
    // symmetry and to guarantee the stub panics (not returns wrong value) when red.
    for _ in 0..1000 {
        assert_eq!(describe(None), "nothing");
    }
}

/// Round-trip: describe(Some(n)) can be parsed back to n.
#[test]
fn prop_describe_some_round_trip_parse() {
    let mut rng: u64 = 0xC0DE_CAFE_0BAD_F00D;
    for _ in 0..1000 {
        let n = in_range(&mut rng, -10_000, 10_000);
        let s = describe(Some(n));
        // s should be "value: N" — strip prefix and parse
        let stripped = s.strip_prefix("value: ").expect("missing prefix");
        let parsed: i64 = stripped
            .parse()
            .expect(&format!("cannot parse {stripped:?} as i64"));
        assert_eq!(parsed, n, "round-trip failed for n={n}: got {parsed}");
    }
}

// ── edge cases ────────────────────────────────────────────────────────────────

/// Shape::Circle with radius 0 has area 0.
#[test]
fn edge_circle_zero_radius_area_is_zero() {
    let area = Shape::Circle(0.0).area();
    assert!((area - 0.0).abs() < 1e-12, "Circle(0).area() = {area}");
}

/// Shape::Rect with one zero side has area 0.
#[test]
fn edge_rect_zero_side_area_is_zero() {
    assert!((Shape::Rect(0.0, 5.0).area() - 0.0).abs() < 1e-12);
    assert!((Shape::Rect(3.0, 0.0).area() - 0.0).abs() < 1e-12);
}

/// first_even on a single even element returns Some of that element.
#[test]
fn edge_first_even_single_even() {
    assert_eq!(first_even(&[42]), Some(42));
    assert_eq!(first_even(&[-8]), Some(-8));
}

/// first_even on a single odd element returns None.
#[test]
fn edge_first_even_single_odd() {
    assert_eq!(first_even(&[7]), None);
    assert_eq!(first_even(&[-3]), None);
}

/// describe(Some(0)) returns "value: 0" (zero is a boundary value).
#[test]
fn edge_describe_some_zero() {
    assert_eq!(describe(Some(0)), "value: 0");
}

/// safe_div with both operands zero returns None (0/0 is undefined).
#[test]
fn edge_safe_div_zero_over_zero_is_none() {
    assert_eq!(safe_div(0.0, 0.0), None);
}

/// safe_div with negative denominator still returns Some with correct sign.
#[test]
fn edge_safe_div_negative_denominator() {
    let result = safe_div(6.0, -2.0);
    assert!(result.is_some());
    assert!((result.unwrap() - (-3.0)).abs() < 1e-12);
}
