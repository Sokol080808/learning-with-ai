// Свойственные (property-based) тесты модуля 02 — Control Flow.
// Используют детерминированный xorshift64* ГПСЧ — без внешних крейтов, воспроизводимо.

use m02_control_flow::*;

// ---------------------------------------------------------------------------
// Детерминированный ГПСЧ (xorshift64*) — фиксированный сид, без крейтов.
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// fizzbuzz — свойства
// ---------------------------------------------------------------------------

/// Длина результата всегда равна n.
#[test]
fn prop_fizzbuzz_length_equals_n() {
    let mut rng: u64 = 0xDEAD_BEEF_CAFE_1234;
    for _ in 0..500 {
        let n = in_range(&mut rng, 0, 200) as u32;
        let v = fizzbuzz(n);
        assert_eq!(
            v.len(),
            n as usize,
            "fizzbuzz({n}).len() == {n} violated"
        );
    }
}

/// Каждый элемент — непустая строка.
#[test]
fn prop_fizzbuzz_elements_nonempty() {
    let mut rng: u64 = 0x1111_2222_3333_4444;
    for _ in 0..500 {
        let n = in_range(&mut rng, 1, 200) as u32;
        for s in fizzbuzz(n) {
            assert!(!s.is_empty(), "fizzbuzz element must not be empty");
        }
    }
}

/// Кратные 15 → "FizzBuzz"; кратные только 3 → "Fizz"; кратные только 5 → "Buzz";
/// остальные → цифровая строка, которая парсится обратно в то же число.
#[test]
fn prop_fizzbuzz_oracle_per_element() {
    let mut rng: u64 = 0xABCD_EF01_2345_6789;
    for _ in 0..500 {
        let n = in_range(&mut rng, 1, 300) as u32;
        let v = fizzbuzz(n);
        for (idx, s) in v.iter().enumerate() {
            let i = (idx + 1) as u32; // 1-based
            let expected = match (i % 3, i % 5) {
                (0, 0) => "FizzBuzz".to_string(),
                (0, _) => "Fizz".to_string(),
                (_, 0) => "Buzz".to_string(),
                _ => i.to_string(),
            };
            assert_eq!(s, &expected, "fizzbuzz({n})[{idx}] wrong");
        }
    }
}

/// При n == 0 — пустой вектор (граничный случай).
#[test]
fn prop_fizzbuzz_zero_empty() {
    assert!(fizzbuzz(0).is_empty());
}

/// Числовые элементы (не Fizz/Buzz) парсятся обратно в правильный индекс.
#[test]
fn prop_fizzbuzz_numeric_elements_parse_back() {
    let mut rng: u64 = 0xFEED_FACE_DEAD_BEEF;
    for _ in 0..300 {
        let n = in_range(&mut rng, 1, 500) as u32;
        let v = fizzbuzz(n);
        for (idx, s) in v.iter().enumerate() {
            let i = (idx + 1) as u32;
            if i % 3 != 0 && i % 5 != 0 {
                let parsed: u32 = s
                    .parse()
                    .unwrap_or_else(|_| panic!("fizzbuzz({n})[{idx}] not parseable as u32: {s}"));
                assert_eq!(parsed, i, "numeric element should equal position");
            }
        }
    }
}

// ---------------------------------------------------------------------------
// factorial — свойства
// ---------------------------------------------------------------------------

/// factorial(n) > 0 для всех n (факториал никогда не равен нулю).
#[test]
fn prop_factorial_always_positive() {
    let mut rng: u64 = 0x0102_0304_0506_0708;
    for _ in 0..500 {
        let n = in_range(&mut rng, 0, 15) as u64; // ограничиваем 15!, чтобы не переполнить u64
        assert!(factorial(n) > 0, "factorial({n}) must be > 0");
    }
}

/// Рекуррентное соотношение: factorial(n) == n * factorial(n-1) для n >= 1.
#[test]
fn prop_factorial_recurrence() {
    let mut rng: u64 = 0xCAFE_BABE_1357_2468;
    for _ in 0..500 {
        let n = in_range(&mut rng, 1, 15) as u64;
        assert_eq!(
            factorial(n),
            n * factorial(n - 1),
            "factorial({n}) != {n} * factorial({})",
            n - 1
        );
    }
}

/// Оракул: сравниваем с наивным произведением в цикле.
#[test]
fn prop_factorial_vs_naive() {
    let mut rng: u64 = 0x9876_5432_10FE_DCBA;
    for _ in 0..500 {
        let n = in_range(&mut rng, 0, 15) as u64;
        let naive: u64 = (1..=n).product();
        assert_eq!(factorial(n), naive, "factorial({n}) disagrees with naive product");
    }
}

/// Граничный случай: factorial(0) == 1.
#[test]
fn prop_factorial_zero_is_one() {
    assert_eq!(factorial(0), 1);
}

// ---------------------------------------------------------------------------
// fib — свойства
// ---------------------------------------------------------------------------

/// fib(n) >= 0 (тип u64, всегда неотрицательно — проверяем неуменьшение ряда).
/// Ряд Фибоначчи неубывает: fib(n+1) >= fib(n) для n >= 0.
#[test]
fn prop_fib_nondecreasing() {
    let mut rng: u64 = 0x1234_5678_9ABC_DEF0;
    for _ in 0..500 {
        let n = in_range(&mut rng, 0, 70) as u32; // fib(93) близко к u64::MAX
        assert!(
            fib(n + 1) >= fib(n),
            "fib({}) < fib({n}) — ряд не неубывает",
            n + 1
        );
    }
}

/// Рекуррентное соотношение: fib(n) == fib(n-1) + fib(n-2) для n >= 2.
#[test]
fn prop_fib_recurrence() {
    let mut rng: u64 = 0xDECA_FBAD_0011_2233;
    for _ in 0..500 {
        let n = in_range(&mut rng, 2, 70) as u32;
        assert_eq!(
            fib(n),
            fib(n - 1) + fib(n - 2),
            "fib({n}) != fib({}) + fib({})",
            n - 1,
            n - 2
        );
    }
}

/// Оракул: итеративное вычисление (независимая реализация).
#[test]
fn prop_fib_vs_oracle() {
    fn fib_oracle(n: u32) -> u64 {
        let (mut a, mut b) = (0u64, 1u64);
        for _ in 0..n {
            (a, b) = (b, a + b);
        }
        a
    }

    let mut rng: u64 = 0xF0E1_D2C3_B4A5_9687;
    for _ in 0..500 {
        let n = in_range(&mut rng, 0, 70) as u32;
        assert_eq!(fib(n), fib_oracle(n), "fib({n}) disagrees with oracle");
    }
}

/// Граничные случаи: fib(0) == 0, fib(1) == 1.
#[test]
fn prop_fib_base_cases() {
    assert_eq!(fib(0), 0);
    assert_eq!(fib(1), 1);
}

// ---------------------------------------------------------------------------
// gcd — свойства
// ---------------------------------------------------------------------------

/// gcd(a, b) делит и a, и b.
#[test]
fn prop_gcd_divides_both() {
    let mut rng: u64 = 0xBEEF_DEAD_CAFE_4321;
    for _ in 0..500 {
        let a = in_range(&mut rng, 0, 10_000) as u64;
        let b = in_range(&mut rng, 0, 10_000) as u64;
        let g = gcd(a, b);
        if g > 0 {
            assert_eq!(a % g, 0, "gcd({a},{b})={g} does not divide {a}");
            assert_eq!(b % g, 0, "gcd({a},{b})={g} does not divide {b}");
        }
    }
}

/// gcd(a, b) == gcd(b, a) — коммутативность.
#[test]
fn prop_gcd_commutative() {
    let mut rng: u64 = 0x1357_9BDF_2468_ACE0;
    for _ in 0..500 {
        let a = in_range(&mut rng, 0, 10_000) as u64;
        let b = in_range(&mut rng, 0, 10_000) as u64;
        assert_eq!(gcd(a, b), gcd(b, a), "gcd({a},{b}) != gcd({b},{a})");
    }
}

/// gcd(a, a) == a для a > 0.
#[test]
fn prop_gcd_same_args() {
    let mut rng: u64 = 0xF1E2_D3C4_B5A6_9780;
    for _ in 0..500 {
        let a = in_range(&mut rng, 1, 10_000) as u64;
        assert_eq!(gcd(a, a), a, "gcd({a},{a}) should be {a}");
    }
}

/// gcd(a, 0) == a и gcd(0, b) == b.
#[test]
fn prop_gcd_with_zero() {
    let mut rng: u64 = 0x8765_4321_FEDC_BA98;
    for _ in 0..500 {
        let a = in_range(&mut rng, 1, 10_000) as u64;
        assert_eq!(gcd(a, 0), a, "gcd({a}, 0) should be {a}");
        assert_eq!(gcd(0, a), a, "gcd(0, {a}) should be {a}");
    }
}

/// gcd(a*k, b*k) == k * gcd(a, b) — мультипликативность.
#[test]
fn prop_gcd_multiplicative() {
    let mut rng: u64 = 0x2B4D_6F81_A3C5_E709;
    for _ in 0..500 {
        let a = in_range(&mut rng, 1, 1_000) as u64;
        let b = in_range(&mut rng, 1, 1_000) as u64;
        let k = in_range(&mut rng, 1, 20) as u64;
        assert_eq!(
            gcd(a * k, b * k),
            k * gcd(a, b),
            "gcd({a}*{k}, {b}*{k}) != {k} * gcd({a},{b})"
        );
    }
}

/// Оракул: проверяем что g = gcd(a,b) является наибольшим общим делителем —
/// нет числа между g+1 и min(a,b), которое делило бы оба.
#[test]
fn prop_gcd_is_greatest() {
    let mut rng: u64 = 0x9A8B_7C6D_5E4F_3021;
    for _ in 0..200 {
        // Небольшой диапазон, т.к. внутренний цикл по делителям.
        let a = in_range(&mut rng, 1, 300) as u64;
        let b = in_range(&mut rng, 1, 300) as u64;
        let g = gcd(a, b);
        // Наивный оракул: найти наибольший общий делитель перебором.
        let naive = (1..=a.min(b)).filter(|&d| a % d == 0 && b % d == 0).last().unwrap_or(1);
        assert_eq!(g, naive, "gcd({a},{b}) = {g} but naive oracle says {naive}");
    }
}

// ---------------------------------------------------------------------------
// classify — свойства
// ---------------------------------------------------------------------------

/// classify(n) возвращает строго один из трёх допустимых литералов.
#[test]
fn prop_classify_valid_output() {
    let valid = ["negative", "zero", "positive"];
    let mut rng: u64 = 0x1A2B_3C4D_5E6F_7080;
    for _ in 0..500 {
        let n = in_range(&mut rng, -10_000, 10_000);
        let s = classify(n);
        assert!(
            valid.contains(&s),
            "classify({n}) returned unexpected value: {s}"
        );
    }
}

/// classify совпадает с наивным оракулом (цепочка if).
#[test]
fn prop_classify_oracle() {
    let mut rng: u64 = 0xA1B2_C3D4_E5F6_0718;
    for _ in 0..500 {
        let n = in_range(&mut rng, -10_000, 10_000);
        let expected = if n < 0 {
            "negative"
        } else if n == 0 {
            "zero"
        } else {
            "positive"
        };
        assert_eq!(classify(n), expected, "classify({n}) != oracle");
    }
}

/// Знак результата classify согласован: отрицательные числа → "negative" и т.д.
#[test]
fn prop_classify_sign_consistency() {
    let mut rng: u64 = 0x5F4E_3D2C_1B0A_9988;
    for _ in 0..500 {
        let n = in_range(&mut rng, -10_000, 10_000);
        match classify(n) {
            "negative" => assert!(n < 0, "classify({n}) == 'negative' but n >= 0"),
            "zero" => assert_eq!(n, 0, "classify({n}) == 'zero' but n != 0"),
            "positive" => assert!(n > 0, "classify({n}) == 'positive' but n <= 0"),
            other => panic!("classify({n}) returned unknown value: {other}"),
        }
    }
}

/// Граничные случаи: 0, -1, 1, i64::MIN, i64::MAX.
#[test]
fn prop_classify_boundaries() {
    assert_eq!(classify(0), "zero");
    assert_eq!(classify(-1), "negative");
    assert_eq!(classify(1), "positive");
    assert_eq!(classify(i64::MIN), "negative");
    assert_eq!(classify(i64::MAX), "positive");
}
