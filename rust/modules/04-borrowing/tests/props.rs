// Property-based tests for m04_borrowing — детерминированные, без внешних крейтов.
// Каждый тест прогоняет ~500–1000 случайных входов с фиксированным сидом.

use m04_borrowing::*;

// ─── детерминированный ГПСЧ (xorshift64*) ───────────────────────────────────
// Используй ЭТОТ helper во всех property-тестах модуля. Сид ФИКСИРОВАННЫЙ (hardcode),
// чтобы прогон был детерминированным.
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

// ─── sum_slice ───────────────────────────────────────────────────────────────

/// sum_slice agrees with a naive loop oracle for random slices.
#[test]
fn prop_sum_slice_matches_naive_loop() {
    let mut rng: u64 = 0xDEAD_BEEF_1234_5678;
    for _ in 0..800 {
        let len = in_range(&mut rng, 0, 30) as usize;
        let v: Vec<i64> = (0..len).map(|_| in_range(&mut rng, -10_000, 10_000)).collect();

        let expected: i64 = {
            let mut s = 0i64;
            for &x in &v {
                s += x;
            }
            s
        };
        assert_eq!(sum_slice(&v), expected, "mismatch on input {:?}", v);
    }
}

/// sum_slice of a single-element slice equals that element.
#[test]
fn prop_sum_slice_single_element() {
    let mut rng: u64 = 0xABCD_1234_FEDC_0001;
    for _ in 0..800 {
        let x = in_range(&mut rng, -10_000, 10_000);
        assert_eq!(sum_slice(&[x]), x);
    }
}

/// sum_slice is always 0 for an empty slice (explicit edge case).
#[test]
fn prop_sum_slice_empty_is_zero() {
    assert_eq!(sum_slice(&[]), 0);
}

/// sum_slice(a ++ b) == sum_slice(a) + sum_slice(b) (additivity / split invariant).
#[test]
fn prop_sum_slice_additive() {
    let mut rng: u64 = 0xC0FF_EAAB_B004_2DEA;
    for _ in 0..600 {
        let len = in_range(&mut rng, 0, 40) as usize;
        let v: Vec<i64> = (0..len).map(|_| in_range(&mut rng, -5_000, 5_000)).collect();
        let split = in_range(&mut rng, 0, len as i64) as usize;

        let left = sum_slice(&v[..split]);
        let right = sum_slice(&v[split..]);
        let whole = sum_slice(&v);
        assert_eq!(left + right, whole, "additivity failed for {:?}", v);
    }
}

/// sum of a slice of all-zeros is always 0.
#[test]
fn prop_sum_slice_all_zeros() {
    let mut rng: u64 = 0x1111_2222_3333_4444;
    for _ in 0..300 {
        let len = in_range(&mut rng, 0, 50) as usize;
        let v: Vec<i64> = vec![0; len];
        assert_eq!(sum_slice(&v), 0);
    }
}

// ─── push_n ──────────────────────────────────────────────────────────────────

/// push_n increases length by exactly count.
#[test]
fn prop_push_n_length_grows_by_count() {
    let mut rng: u64 = 0x9999_AAAA_BBBB_CCCC;
    for _ in 0..800 {
        let init_len = in_range(&mut rng, 0, 20) as usize;
        let mut v: Vec<i64> = (0..init_len).map(|_| in_range(&mut rng, -500, 500)).collect();
        let value = in_range(&mut rng, -10_000, 10_000);
        let count = in_range(&mut rng, 0, 50) as usize;

        let before = v.len();
        push_n(&mut v, value, count);
        assert_eq!(v.len(), before + count, "length mismatch: count={}", count);
    }
}

/// push_n appends exactly the right value count times (tail elements all equal value).
#[test]
fn prop_push_n_correct_value_appended() {
    let mut rng: u64 = 0xFEED_FACE_CAFE_BABE;
    for _ in 0..700 {
        let init_len = in_range(&mut rng, 0, 15) as usize;
        let mut v: Vec<i64> = (0..init_len).map(|_| in_range(&mut rng, -100, 100)).collect();
        let value = in_range(&mut rng, -10_000, 10_000);
        let count = in_range(&mut rng, 1, 40) as usize;

        push_n(&mut v, value, count);

        // The last `count` elements must all equal `value`.
        let tail = &v[v.len() - count..];
        assert!(
            tail.iter().all(|&x| x == value),
            "tail not all {}: {:?}",
            value,
            tail
        );
    }
}

/// push_n with count=0 leaves the vector completely unchanged.
#[test]
fn prop_push_n_zero_count_no_change() {
    let mut rng: u64 = 0x0102_0304_0506_0708;
    for _ in 0..500 {
        let init_len = in_range(&mut rng, 0, 30) as usize;
        let v_orig: Vec<i64> = (0..init_len).map(|_| in_range(&mut rng, -999, 999)).collect();
        let mut v = v_orig.clone();
        let value = in_range(&mut rng, -10_000, 10_000);

        push_n(&mut v, value, 0);

        assert_eq!(v, v_orig, "vector changed even though count=0");
    }
}

/// push_n prefix (elements before the appended tail) stays unchanged.
#[test]
fn prop_push_n_prefix_unchanged() {
    let mut rng: u64 = 0xAABB_CCDD_EEFF_0011;
    for _ in 0..600 {
        let init_len = in_range(&mut rng, 1, 20) as usize;
        let prefix: Vec<i64> = (0..init_len).map(|_| in_range(&mut rng, -1000, 1000)).collect();
        let mut v = prefix.clone();
        let value = in_range(&mut rng, -10_000, 10_000);
        let count = in_range(&mut rng, 1, 30) as usize;

        push_n(&mut v, value, count);

        assert_eq!(
            &v[..init_len],
            prefix.as_slice(),
            "prefix was mutated by push_n"
        );
    }
}

// ─── double_all ──────────────────────────────────────────────────────────────

/// double_all matches element-wise multiplication by 2 (oracle).
#[test]
fn prop_double_all_matches_oracle() {
    let mut rng: u64 = 0x1357_9BDF_2468_ACE0;
    for _ in 0..800 {
        let len = in_range(&mut rng, 0, 40) as usize;
        let original: Vec<i64> = (0..len).map(|_| in_range(&mut rng, -5_000, 5_000)).collect();
        let mut v = original.clone();

        double_all(&mut v);

        let expected: Vec<i64> = original.iter().map(|&x| x * 2).collect();
        assert_eq!(v, expected, "double_all mismatch for {:?}", original);
    }
}

/// double_all never changes the length of the vector.
#[test]
fn prop_double_all_length_preserved() {
    let mut rng: u64 = 0x2222_3333_4444_5555;
    for _ in 0..800 {
        let len = in_range(&mut rng, 0, 50) as usize;
        let mut v: Vec<i64> = (0..len).map(|_| in_range(&mut rng, -1_000, 1_000)).collect();

        double_all(&mut v);

        assert_eq!(v.len(), len, "length changed after double_all");
    }
}

/// double_all applied twice equals multiplication by 4.
#[test]
fn prop_double_all_twice_is_times_four() {
    let mut rng: u64 = 0x5A5A_A5A5_F0F0_0F0F;
    for _ in 0..500 {
        let len = in_range(&mut rng, 0, 30) as usize;
        // Keep values small to avoid overflow after ×4.
        let original: Vec<i64> = (0..len).map(|_| in_range(&mut rng, -1_000, 1_000)).collect();
        let mut v = original.clone();

        double_all(&mut v);
        double_all(&mut v);

        let expected: Vec<i64> = original.iter().map(|&x| x * 4).collect();
        assert_eq!(v, expected);
    }
}

/// sum(double_all(v)) == 2 * sum(v) for all random slices.
#[test]
fn prop_double_all_sum_doubles() {
    let mut rng: u64 = 0xBEEF_DEAD_CAFE_1234;
    for _ in 0..700 {
        let len = in_range(&mut rng, 0, 40) as usize;
        let original: Vec<i64> = (0..len).map(|_| in_range(&mut rng, -2_000, 2_000)).collect();
        let sum_before = sum_slice(&original);
        let mut v = original.clone();

        double_all(&mut v);

        assert_eq!(sum_slice(&v), 2 * sum_before);
    }
}

/// double_all on a vec of zeros stays all zeros.
#[test]
fn prop_double_all_zeros_stays_zero() {
    let mut rng: u64 = 0x0000_0000_1111_FFFF;
    for _ in 0..300 {
        let len = in_range(&mut rng, 0, 50) as usize;
        let mut v: Vec<i64> = vec![0; len];
        double_all(&mut v);
        assert!(v.iter().all(|&x| x == 0));
    }
}

// ─── max_ref ─────────────────────────────────────────────────────────────────

/// max_ref returns None iff the slice is empty.
#[test]
fn prop_max_ref_none_iff_empty() {
    // Empty → None
    assert_eq!(max_ref(&[]), None);

    let mut rng: u64 = 0xCAFE_BABE_1234_5678;
    for _ in 0..700 {
        let len = in_range(&mut rng, 1, 40) as usize;
        let v: Vec<i64> = (0..len).map(|_| in_range(&mut rng, -10_000, 10_000)).collect();
        assert!(
            max_ref(&v).is_some(),
            "max_ref returned None for non-empty slice"
        );
    }
}

/// max_ref value agrees with a naive max oracle.
#[test]
fn prop_max_ref_matches_naive_max() {
    let mut rng: u64 = 0x1234_ABCD_5678_EF01;
    for _ in 0..800 {
        let len = in_range(&mut rng, 1, 50) as usize;
        let v: Vec<i64> = (0..len).map(|_| in_range(&mut rng, -10_000, 10_000)).collect();

        // naive oracle: linear scan
        let mut expected = v[0];
        for &x in &v[1..] {
            if x > expected {
                expected = x;
            }
        }

        let result = max_ref(&v).expect("max_ref returned None for non-empty slice");
        assert_eq!(
            *result, expected,
            "max_ref value wrong for {:?}",
            v
        );
    }
}

/// The value returned by max_ref is actually present in the slice (not a fabricated copy).
#[test]
fn prop_max_ref_value_is_in_slice() {
    let mut rng: u64 = 0xFACE_FEED_C0DE_BEEF;
    for _ in 0..700 {
        let len = in_range(&mut rng, 1, 40) as usize;
        let v: Vec<i64> = (0..len).map(|_| in_range(&mut rng, -10_000, 10_000)).collect();

        let result = *max_ref(&v).unwrap();
        assert!(
            v.contains(&result),
            "max_ref returned {} which is not in {:?}",
            result,
            v
        );
    }
}

/// max_ref value is >= every element in the slice.
#[test]
fn prop_max_ref_is_upper_bound() {
    let mut rng: u64 = 0x9876_FEDC_BA10_3210;
    for _ in 0..800 {
        let len = in_range(&mut rng, 1, 50) as usize;
        let v: Vec<i64> = (0..len).map(|_| in_range(&mut rng, -10_000, 10_000)).collect();

        let m = *max_ref(&v).unwrap();
        for &x in &v {
            assert!(m >= x, "max_ref={} but element {} is larger, slice={:?}", m, x, v);
        }
    }
}

/// max_ref of a single-element slice returns that element.
#[test]
fn prop_max_ref_single_element() {
    let mut rng: u64 = 0x1111_2222_3333_4444;
    for _ in 0..500 {
        let x = in_range(&mut rng, -10_000, 10_000);
        assert_eq!(max_ref(&[x]), Some(&x));
    }
}

/// max_ref of an all-equal slice returns that value.
#[test]
fn prop_max_ref_all_equal() {
    let mut rng: u64 = 0xAAAA_BBBB_CCCC_DDDD;
    for _ in 0..400 {
        let val = in_range(&mut rng, -10_000, 10_000);
        let len = in_range(&mut rng, 1, 50) as usize;
        let v: Vec<i64> = vec![val; len];
        assert_eq!(*max_ref(&v).unwrap(), val);
    }
}
