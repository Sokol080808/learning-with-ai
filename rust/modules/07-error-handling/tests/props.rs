// Рандомизированные property-тесты модуля 07 — обработка ошибок.
// Используют только публичный API крейта и детерминированный ГПСЧ (без внешних крейтов).

use m07_errors::*;

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

// -----------------------------------------------------------------------
// parse_int: round-trip property — format then parse returns original value
// -----------------------------------------------------------------------
#[test]
fn prop_parse_int_round_trip() {
    let mut rng: u64 = 0xDEAD_BEEF_1234_5678;
    for _ in 0..1000 {
        let n = in_range(&mut rng, -100_000, 100_000);
        let s = n.to_string();
        assert_eq!(
            parse_int(&s),
            Ok(n),
            "round-trip failed for n={n}"
        );
    }
}

// parse_int: Ok result matches std parse
#[test]
fn prop_parse_int_agrees_with_std() {
    let mut rng: u64 = 0xABCD_EF01_2345_6789;
    for _ in 0..1000 {
        let n = in_range(&mut rng, -999_999, 999_999);
        let s = n.to_string();
        // both std and our function must agree on the same value
        let std_result: Result<i64, _> = s.parse::<i64>();
        match parse_int(&s) {
            Ok(v) => assert!(std_result.is_ok() && std_result.unwrap() == v),
            Err(_) => assert!(std_result.is_err()),
        }
    }
}

// parse_int: Err message must contain the input string in single quotes
#[test]
fn prop_parse_int_err_message_contains_input() {
    // a set of known-invalid strings built without calling any solution function
    let bad_inputs = [
        "abc", "12x", "x12", "1.5", " 7", "7 ", "--1", "++1", "1e5",
        "inf", "NaN", "hello world", "½", "",
    ];
    for s in bad_inputs {
        let result = parse_int(s);
        assert!(result.is_err(), "expected Err for input '{s}'");
        let msg = result.unwrap_err();
        let expected_fragment = format!("'{s}'");
        assert!(
            msg.contains(&expected_fragment),
            "error message '{msg}' does not contain '{expected_fragment}'"
        );
    }
}

// parse_int: Ok is always returned for plain decimal integer strings
#[test]
fn prop_parse_int_valid_strings_are_ok() {
    let mut rng: u64 = 0x1111_2222_3333_4444;
    for _ in 0..500 {
        let n = in_range(&mut rng, -10_000, 10_000);
        // plain decimal, positive with explicit '+', already tested negative via n.to_string()
        let plain = n.to_string();
        assert!(parse_int(&plain).is_ok(), "parse_int({plain:?}) should be Ok");

        if n > 0 {
            let with_plus = format!("+{n}");
            assert!(
                parse_int(&with_plus).is_ok(),
                "parse_int({with_plus:?}) should be Ok"
            );
        }
    }
}

// -----------------------------------------------------------------------
// divide: invariants
// -----------------------------------------------------------------------

// divide: non-zero divisor always returns Ok
#[test]
fn prop_divide_nonzero_is_ok() {
    let mut rng: u64 = 0x5555_AAAA_BBBB_CCCC;
    for _ in 0..1000 {
        let a = in_range(&mut rng, -10_000, 10_000);
        // b in [-10_000, -1] ∪ [1, 10_000] — never zero
        let b = {
            let raw = in_range(&mut rng, 1, 10_000);
            if in_range(&mut rng, 0, 1) == 0 { raw } else { -raw }
        };
        assert!(
            divide(a, b).is_ok(),
            "divide({a}, {b}) should be Ok for non-zero b"
        );
    }
}

// divide: result agrees with Rust's built-in integer division (truncates toward zero)
#[test]
fn prop_divide_agrees_with_builtin() {
    let mut rng: u64 = 0xFEDC_BA98_7654_3210;
    for _ in 0..1000 {
        let a = in_range(&mut rng, -10_000, 10_000);
        let b = in_range(&mut rng, 1, 10_000); // positive non-zero for simplicity
        let expected = a / b; // Rust built-in truncation-toward-zero oracle
        assert_eq!(
            divide(a, b),
            Ok(expected),
            "divide({a}, {b}) != Ok({expected})"
        );
    }
}

// divide by zero always returns Err
#[test]
fn prop_divide_by_zero_always_err() {
    let mut rng: u64 = 0xCAFE_BABE_DEAD_BEEF;
    for _ in 0..500 {
        let a = in_range(&mut rng, -100_000, 100_000);
        let result = divide(a, 0);
        assert!(
            result.is_err(),
            "divide({a}, 0) should be Err, got Ok"
        );
        assert_eq!(
            result.unwrap_err(),
            "division by zero",
            "divide({a}, 0) error message mismatch"
        );
    }
}

// divide: sign of result follows arithmetic rules
#[test]
fn prop_divide_sign_invariant() {
    let mut rng: u64 = 0x0102_0304_0506_0708;
    for _ in 0..500 {
        let a = in_range(&mut rng, 1, 10_000);   // positive
        let b = in_range(&mut rng, 1, 10_000);   // positive
        // pos / pos => non-negative
        assert!(divide(a, b).unwrap() >= 0);
        // neg / pos => non-positive
        assert!(divide(-a, b).unwrap() <= 0);
        // pos / neg => non-positive
        assert!(divide(a, -b).unwrap() <= 0);
        // neg / neg => non-negative
        assert!(divide(-a, -b).unwrap() >= 0);
    }
}

// -----------------------------------------------------------------------
// element_at: invariants
// -----------------------------------------------------------------------

// element_at: in-bounds index returns correct value
#[test]
fn prop_element_at_inbounds_returns_correct_value() {
    let mut rng: u64 = 0xAAAA_BBBB_CCCC_DDDD;
    for _ in 0..500 {
        // build a vector of random length 1..=20 with random i64 values
        let len = in_range(&mut rng, 1, 20) as usize;
        let v: Vec<i64> = (0..len)
            .map(|_| in_range(&mut rng, -1000, 1000))
            .collect();
        let idx = in_range(&mut rng, 0, (len - 1) as i64) as usize;
        assert_eq!(
            element_at(&v, idx),
            Ok(v[idx]),
            "element_at(&v, {idx}) should equal v[{idx}]={}", v[idx]
        );
    }
}

// element_at: out-of-bounds index always returns Err with correct message
#[test]
fn prop_element_at_oob_returns_correct_err() {
    let mut rng: u64 = 0x1234_5678_ABCD_EF01;
    for _ in 0..500 {
        let len = in_range(&mut rng, 0, 10) as usize;
        let v: Vec<i64> = (0..len)
            .map(|_| in_range(&mut rng, -1000, 1000))
            .collect();
        // index is always >= len (out-of-bounds)
        let oob_idx = len + in_range(&mut rng, 0, 10) as usize;
        let expected_err = format!("index {oob_idx} out of bounds for length {len}");
        assert_eq!(
            element_at(&v, oob_idx),
            Err(expected_err.clone()),
            "element_at oob error mismatch: expected '{expected_err}'"
        );
    }
}

// element_at on empty slice is always Err regardless of index
#[test]
fn prop_element_at_empty_slice_always_err() {
    let v: Vec<i64> = vec![];
    for idx in 0usize..20 {
        let result = element_at(&v, idx);
        assert!(
            result.is_err(),
            "element_at on empty slice with index {idx} should be Err"
        );
        let msg = result.unwrap_err();
        assert!(
            msg.contains("out of bounds"),
            "error message '{msg}' should contain 'out of bounds'"
        );
        assert!(
            msg.contains("length 0"),
            "error message '{msg}' should contain 'length 0'"
        );
    }
}

// -----------------------------------------------------------------------
// sum_parsed: invariants
// -----------------------------------------------------------------------

// sum_parsed: all-valid slices agree with inline oracle (iter().sum)
#[test]
fn prop_sum_parsed_valid_input_equals_sum() {
    let mut rng: u64 = 0xBEEF_CAFE_1357_2468;
    for _ in 0..500 {
        let len = in_range(&mut rng, 0, 15) as usize;
        let values: Vec<i64> = (0..len)
            .map(|_| in_range(&mut rng, -1000, 1000))
            .collect();
        let strings: Vec<String> = values.iter().map(|v| v.to_string()).collect();
        let str_refs: Vec<&str> = strings.iter().map(|s| s.as_str()).collect();

        let expected: i64 = values.iter().sum(); // oracle
        assert_eq!(
            sum_parsed(&str_refs),
            Ok(expected),
            "sum_parsed mismatch for values={values:?}"
        );
    }
}

// sum_parsed: empty slice is always Ok(0)
#[test]
fn prop_sum_parsed_empty_is_zero() {
    assert_eq!(sum_parsed(&[]), Ok(0));
}

// sum_parsed: any invalid element causes Err
#[test]
fn prop_sum_parsed_invalid_element_causes_err() {
    let mut rng: u64 = 0x9999_8888_7777_6666;
    let bad_tokens = ["abc", "x", "1.5", "", "NaN", "inf", "12x"];
    for _ in 0..500 {
        let len = in_range(&mut rng, 1, 8) as usize;
        let mut strings: Vec<String> = (0..len)
            .map(|_| in_range(&mut rng, -500, 500).to_string())
            .collect();
        // inject one bad token at a random position
        let bad_pos = in_range(&mut rng, 0, (len - 1) as i64) as usize;
        let bad_idx = (next_u64(&mut rng) % bad_tokens.len() as u64) as usize;
        strings[bad_pos] = bad_tokens[bad_idx].to_string();

        let str_refs: Vec<&str> = strings.iter().map(|s| s.as_str()).collect();
        assert!(
            sum_parsed(&str_refs).is_err(),
            "sum_parsed should be Err when '{bad}' is in input at position {bad_pos}",
            bad = bad_tokens[bad_idx]
        );
    }
}

// sum_parsed: error propagates from the FIRST bad element (not a later one)
#[test]
fn prop_sum_parsed_error_from_first_bad() {
    let mut rng: u64 = 0x1A2B_3C4D_5E6F_7A8B;
    for _ in 0..500 {
        // build ["1", "2", ..., "bad_token", "more_numbers", ...]
        let prefix_len = in_range(&mut rng, 0, 5) as usize;
        let suffix_len = in_range(&mut rng, 0, 5) as usize;

        let prefix: Vec<String> = (0..prefix_len)
            .map(|_| in_range(&mut rng, -100, 100).to_string())
            .collect();
        // choose a bad token that is unambiguously bad
        let bad_strings = ["bad_first", "not_a_num"];
        let bi = (next_u64(&mut rng) % bad_strings.len() as u64) as usize;
        let bad = bad_strings[bi];
        let suffix: Vec<String> = (0..suffix_len)
            .map(|_| in_range(&mut rng, -100, 100).to_string())
            .collect();

        let mut all_strings: Vec<String> = prefix;
        all_strings.push(bad.to_string());
        all_strings.extend(suffix);

        let str_refs: Vec<&str> = all_strings.iter().map(|s| s.as_str()).collect();
        let result = sum_parsed(&str_refs);
        assert!(result.is_err(), "should be Err");
        let msg = result.unwrap_err();
        // the error must contain the bad token (from parse_int's format)
        assert!(
            msg.contains(bad),
            "error message '{msg}' should contain the bad token '{bad}'"
        );
    }
}

// sum_parsed: single-element slices round-trip
#[test]
fn prop_sum_parsed_single_element_round_trip() {
    let mut rng: u64 = 0x2B3C_4D5E_6F7A_8B9C;
    for _ in 0..500 {
        let n = in_range(&mut rng, -10_000, 10_000);
        let s = n.to_string();
        assert_eq!(
            sum_parsed(&[s.as_str()]),
            Ok(n),
            "sum_parsed of single element {n} should be Ok({n})"
        );
    }
}
