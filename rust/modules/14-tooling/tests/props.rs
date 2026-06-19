// Randomized property tests for m14_tooling.
// Uses a small deterministic PRNG (xorshift64*) — no external crates, fully reproducible.

use m14_tooling::*;

// ---------------------------------------------------------------------------
// Deterministic PRNG helpers
// ---------------------------------------------------------------------------

/// маленький детерминированный ГПСЧ (xorshift64*), без внешних крейтов — воспроизводимо.
/// Используй ЭТОТ helper во всех property-тестах модуля. Сид ФИКСИРОВАННЫЙ (hardcode), чтобы прогон был детерминированным.
fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}

/// целое в диапазоне [lo, hi] включительно (держи диапазон скромным, чтобы оракул не переполнял i64)
fn in_range(state: &mut u64, lo: i64, hi: i64) -> i64 {
    let span = (hi - lo + 1) as u64;
    lo + (next_u64(state) % span) as i64
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Build a Vec<i64> of `len` elements, each in [lo, hi].
fn rand_vec(state: &mut u64, len: usize, lo: i64, hi: i64) -> Vec<i64> {
    (0..len).map(|_| in_range(state, lo, hi)).collect()
}

// ---------------------------------------------------------------------------
// sum_first_n — properties
// ---------------------------------------------------------------------------

/// Property: sum_first_n with n == 0 always returns 0, regardless of the slice.
#[test]
fn prop_sum_first_n_zero_always_zero() {
    let mut rng: u64 = 0xDEAD_BEEF_1234_5678;
    for _ in 0..500 {
        let len = in_range(&mut rng, 0, 20) as usize;
        let v = rand_vec(&mut rng, len, -10_000, 10_000);
        assert_eq!(sum_first_n(&v, 0), 0, "n=0 must yield 0, v={v:?}");
    }
}

/// Property: sum_first_n with n >= len equals the sum of the whole slice (oracle: iter().sum()).
#[test]
fn prop_sum_first_n_full_equals_total() {
    let mut rng: u64 = 0xCAFE_BABE_DEAD_0001;
    for _ in 0..500 {
        let len = in_range(&mut rng, 1, 20) as usize;
        let v = rand_vec(&mut rng, len, -10_000, 10_000);
        let expected: i64 = v.iter().sum();
        // n == len: exact boundary
        assert_eq!(
            sum_first_n(&v, len),
            expected,
            "n==len must sum all, v={v:?}"
        );
        // n > len: clamp to len
        assert_eq!(
            sum_first_n(&v, len + 5),
            expected,
            "n>len must sum all, v={v:?}"
        );
    }
}

/// Property: sum_first_n(v, n) == v[0..n].iter().sum() — oracle via std slice.
#[test]
fn prop_sum_first_n_oracle() {
    let mut rng: u64 = 0x1111_2222_3333_4444;
    for _ in 0..800 {
        let len = in_range(&mut rng, 1, 20) as usize;
        let v = rand_vec(&mut rng, len, -10_000, 10_000);
        let n = in_range(&mut rng, 0, len as i64) as usize;
        let expected: i64 = v[..n].iter().sum();
        assert_eq!(
            sum_first_n(&v, n),
            expected,
            "sum_first_n({v:?}, {n}) != oracle {expected}"
        );
    }
}

/// Property: adding a new element at the front and requesting n+1 increases the sum by that element.
#[test]
fn prop_sum_first_n_prepend_increments() {
    let mut rng: u64 = 0xFEED_C0DE_ABCD_EF01;
    for _ in 0..500 {
        let len = in_range(&mut rng, 1, 15) as usize;
        let v = rand_vec(&mut rng, len, -5_000, 5_000);
        let extra = in_range(&mut rng, -5_000, 5_000);
        let mut v2 = vec![extra];
        v2.extend_from_slice(&v);
        // sum_first_n(v2, len+1) == extra + sum_first_n(v, len)
        let base: i64 = sum_first_n(&v, len);
        let extended: i64 = sum_first_n(&v2, len + 1);
        assert_eq!(extended, base + extra,
            "prepend invariant failed: extra={extra}, v={v:?}");
    }
}

// ---------------------------------------------------------------------------
// max_in — properties
// ---------------------------------------------------------------------------

/// Property: max_in result is always >= every element (it IS the maximum).
#[test]
fn prop_max_in_is_upper_bound() {
    let mut rng: u64 = 0x5555_AAAA_1234_ABCD;
    for _ in 0..800 {
        let len = in_range(&mut rng, 1, 25) as usize;
        let v = rand_vec(&mut rng, len, -10_000, 10_000);
        let m = max_in(&v);
        for &x in &v {
            assert!(
                m >= x,
                "max_in returned {m} but element {x} is larger, v={v:?}"
            );
        }
    }
}

/// Property: max_in result is always an element that actually exists in the slice.
#[test]
fn prop_max_in_is_in_slice() {
    let mut rng: u64 = 0xBEEF_CAFE_0000_1111;
    for _ in 0..800 {
        let len = in_range(&mut rng, 1, 25) as usize;
        let v = rand_vec(&mut rng, len, -10_000, 10_000);
        let m = max_in(&v);
        assert!(
            v.contains(&m),
            "max_in returned {m} which is NOT in v={v:?}"
        );
    }
}

/// Property: max_in agrees with std oracle v.iter().copied().max().unwrap().
#[test]
fn prop_max_in_oracle() {
    let mut rng: u64 = 0x2468_1357_9ACE_BDF0;
    for _ in 0..800 {
        let len = in_range(&mut rng, 1, 20) as usize;
        let v = rand_vec(&mut rng, len, -10_000, 10_000);
        let expected = v.iter().copied().max().unwrap();
        assert_eq!(
            max_in(&v),
            expected,
            "max_in oracle mismatch for v={v:?}"
        );
    }
}

/// Property: all-negative slice — max is still negative, never 0.
#[test]
fn prop_max_in_all_negative_never_zero() {
    let mut rng: u64 = 0xAAAA_BBBB_CCCC_DDDD;
    for _ in 0..500 {
        let len = in_range(&mut rng, 1, 20) as usize;
        // All values in [-10_000, -1]
        let v = rand_vec(&mut rng, len, -10_000, -1);
        let m = max_in(&v);
        assert!(m < 0, "max_in of all-negative slice returned {m} >= 0, v={v:?}");
    }
}

// ---------------------------------------------------------------------------
// average — properties
// ---------------------------------------------------------------------------

/// Property: average of a slice of identical values equals that value.
#[test]
fn prop_average_uniform_equals_value() {
    let mut rng: u64 = 0x1234_5678_9ABC_DEF0;
    for _ in 0..500 {
        let len = in_range(&mut rng, 1, 30) as usize;
        let val = in_range(&mut rng, -10_000, 10_000);
        let v: Vec<i64> = vec![val; len];
        let avg = average(&v);
        assert!(
            (avg - val as f64).abs() < 1e-9,
            "average of uniform {val} should be {val}, got {avg}"
        );
    }
}

/// Property: average agrees with oracle: sum as f64 / len as f64.
#[test]
fn prop_average_oracle() {
    let mut rng: u64 = 0xFEDC_BA98_7654_3210;
    for _ in 0..800 {
        let len = in_range(&mut rng, 1, 20) as usize;
        let v = rand_vec(&mut rng, len, -10_000, 10_000);
        let sum: i64 = v.iter().sum();
        let expected = sum as f64 / v.len() as f64;
        let got = average(&v);
        assert!(
            (got - expected).abs() < 1e-9,
            "average oracle mismatch: got {got}, expected {expected}, v={v:?}"
        );
    }
}

/// Property: average of a non-negative slice is >= 0.
#[test]
fn prop_average_non_negative_slice_is_non_negative() {
    let mut rng: u64 = 0x0ACE_0ACE_FACE_FACE;
    for _ in 0..500 {
        let len = in_range(&mut rng, 1, 20) as usize;
        let v = rand_vec(&mut rng, len, 0, 10_000);
        let avg = average(&v);
        assert!(avg >= 0.0, "average of non-negative slice was {avg}, v={v:?}");
    }
}

/// Property: average is between min and max of the slice.
#[test]
fn prop_average_between_min_and_max() {
    let mut rng: u64 = 0x9876_5432_10FE_DCBA;
    for _ in 0..800 {
        let len = in_range(&mut rng, 1, 20) as usize;
        let v = rand_vec(&mut rng, len, -10_000, 10_000);
        let avg = average(&v);
        let lo = *v.iter().min().unwrap() as f64;
        let hi = *v.iter().max().unwrap() as f64;
        assert!(
            avg >= lo - 1e-9 && avg <= hi + 1e-9,
            "average {avg} not in [{lo}, {hi}], v={v:?}"
        );
    }
}

// ---------------------------------------------------------------------------
// has_duplicate — properties
// ---------------------------------------------------------------------------

/// Property: a slice of all distinct elements (constructed explicitly) has no duplicate.
#[test]
fn prop_has_duplicate_distinct_range_is_false() {
    let mut rng: u64 = 0x1357_2468_ACE0_BDF1;
    for _ in 0..500 {
        // Build a range 0..len — all distinct by construction.
        let len = in_range(&mut rng, 0, 50) as usize;
        let v: Vec<i64> = (0..len as i64).collect();
        assert!(
            !has_duplicate(&v),
            "distinct range 0..{len} should have no duplicate"
        );
    }
}

/// Property: inserting a copy of an existing element always creates a duplicate.
#[test]
fn prop_has_duplicate_inserted_copy_is_true() {
    let mut rng: u64 = 0xDEAD_C0DE_FEED_BEEF;
    for _ in 0..500 {
        let len = in_range(&mut rng, 1, 20) as usize;
        // Unique values: use distinct integers 1000..1000+len
        let mut v: Vec<i64> = (1000..(1000 + len as i64)).collect();
        // Pick a random existing element and duplicate it
        let idx = in_range(&mut rng, 0, len as i64 - 1) as usize;
        let dup = v[idx];
        v.push(dup);
        assert!(
            has_duplicate(&v),
            "slice with inserted duplicate {dup} should return true, v={v:?}"
        );
    }
}

/// Property: a single-element slice never has a duplicate (no other element to compare with).
#[test]
fn prop_has_duplicate_single_is_false() {
    let mut rng: u64 = 0x1111_3333_5555_7777;
    for _ in 0..500 {
        let val = in_range(&mut rng, -10_000, 10_000);
        assert!(
            !has_duplicate(&[val]),
            "single-element [{val}] must not report a duplicate"
        );
    }
}

/// Property: has_duplicate agrees with a naive O(n²) oracle for random slices.
#[test]
fn prop_has_duplicate_oracle() {
    let mut rng: u64 = 0x2222_4444_6666_8888;
    for _ in 0..500 {
        let len = in_range(&mut rng, 0, 15) as usize;
        // Small range to guarantee some duplicates sometimes.
        let v = rand_vec(&mut rng, len, 0, 8);
        // Oracle: brute-force O(n²)
        let mut expected = false;
        'outer: for i in 0..v.len() {
            for j in (i + 1)..v.len() {
                if v[i] == v[j] {
                    expected = true;
                    break 'outer;
                }
            }
        }
        assert_eq!(
            has_duplicate(&v),
            expected,
            "has_duplicate oracle mismatch for v={v:?}"
        );
    }
}

// ---------------------------------------------------------------------------
// repeat_str — properties
// ---------------------------------------------------------------------------

/// Property: length of repeat_str(s, n) == s.len() * n.
#[test]
fn prop_repeat_str_length() {
    let mut rng: u64 = 0xABCD_EF01_2345_6789;
    let words = ["a", "ab", "xyz", "hello", ""];
    for _ in 0..500 {
        let wi = (next_u64(&mut rng) % words.len() as u64) as usize;
        let s = words[wi];
        let times = in_range(&mut rng, 0, 20) as usize;
        let result = repeat_str(s, times);
        assert_eq!(
            result.len(),
            s.len() * times,
            "length of repeat_str({s:?}, {times}) should be {}, got {}",
            s.len() * times,
            result.len()
        );
    }
}

/// Property: every chunk of length s.len() in the result equals s.
#[test]
fn prop_repeat_str_each_chunk_equals_s() {
    let mut rng: u64 = 0x0F0F_0F0F_F0F0_F0F0;
    let words = ["x", "ab", "cat", "rust"];
    for _ in 0..500 {
        let wi = (next_u64(&mut rng) % words.len() as u64) as usize;
        let s = words[wi];
        let times = in_range(&mut rng, 1, 20) as usize;
        let result = repeat_str(s, times);
        for chunk_idx in 0..times {
            let start = chunk_idx * s.len();
            let end = start + s.len();
            assert_eq!(
                &result[start..end],
                s,
                "chunk {chunk_idx} of repeat_str({s:?}, {times}) != {s:?}"
            );
        }
    }
}

/// Property: repeat_str(s, 0) == "" for any s.
#[test]
fn prop_repeat_str_zero_always_empty() {
    let mut rng: u64 = 0xF1F1_F1F1_E2E2_E2E2;
    let words = ["", "a", "hello world", "12345"];
    for _ in 0..500 {
        let wi = (next_u64(&mut rng) % words.len() as u64) as usize;
        let s = words[wi];
        assert_eq!(
            repeat_str(s, 0),
            "",
            "repeat_str({s:?}, 0) must be empty"
        );
    }
}

/// Property: repeat_str(s, 1) == s for any s.
#[test]
fn prop_repeat_str_once_returns_s() {
    let mut rng: u64 = 0xC3C3_C3C3_D4D4_D4D4;
    let words = ["", "a", "hi", "world", "abc123"];
    for _ in 0..500 {
        let wi = (next_u64(&mut rng) % words.len() as u64) as usize;
        let s = words[wi];
        assert_eq!(
            repeat_str(s, 1),
            s,
            "repeat_str({s:?}, 1) must equal s"
        );
    }
}

/// Property: repeat_str(s, n) == repeat_str(s, n-1) + s (incremental build oracle).
#[test]
fn prop_repeat_str_incremental_oracle() {
    let mut rng: u64 = 0x1A2B_3C4D_5E6F_7A8B;
    let words = ["a", "bc", "xyz"];
    for _ in 0..500 {
        let wi = (next_u64(&mut rng) % words.len() as u64) as usize;
        let s = words[wi];
        let times = in_range(&mut rng, 1, 30) as usize;
        // Oracle: build by hand
        let mut expected = String::new();
        for _ in 0..times {
            expected.push_str(s);
        }
        assert_eq!(
            repeat_str(s, times),
            expected,
            "repeat_str({s:?}, {times}) != manually built oracle"
        );
    }
}
