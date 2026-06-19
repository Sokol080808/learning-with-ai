// Тесты для fizzbuzz_vec — deterministic examples + seeded property tests.
// Трогать не нужно: это эталон поведения для контракта функции.

use m01_basics::fizzbuzz_vec;

// ── Детерминированные примеры ──────────────────────────────────────────────────

#[test]
fn empty_for_zero() {
    assert_eq!(fizzbuzz_vec(0), Vec::<String>::new());
}

#[test]
fn single_element_one() {
    assert_eq!(fizzbuzz_vec(1), vec!["1"]);
}

#[test]
fn first_five_elements() {
    assert_eq!(
        fizzbuzz_vec(5),
        vec!["1", "2", "Fizz", "4", "Buzz"]
    );
}

#[test]
fn golden_vector_n15() {
    // Эталонный вектор для n=15: все три случая плюс числа.
    let expected: Vec<String> = vec![
        "1", "2", "Fizz", "4", "Buzz", "Fizz", "7", "8", "Fizz", "Buzz",
        "11", "Fizz", "13", "14", "FizzBuzz",
    ]
    .into_iter()
    .map(String::from)
    .collect();

    assert_eq!(fizzbuzz_vec(15), expected);
}

#[test]
fn length_equals_n() {
    for n in [0u64, 1, 5, 15, 30, 100] {
        let v = fizzbuzz_vec(n);
        assert_eq!(
            v.len(),
            n as usize,
            "fizzbuzz_vec({n}).len() должно быть {n}, получили {}",
            v.len()
        );
    }
}

#[test]
fn fizzbuzz_at_position_15() {
    let v = fizzbuzz_vec(15);
    // индексы с 0, поэтому элемент для числа 15 — v[14]
    assert_eq!(v[14], "FizzBuzz");
}

#[test]
fn fizzbuzz_at_position_30() {
    let v = fizzbuzz_vec(30);
    assert_eq!(v[29], "FizzBuzz", "элемент 30 должен быть FizzBuzz");
    assert_eq!(v[14], "FizzBuzz", "элемент 15 внутри n=30 должен быть FizzBuzz");
}

#[test]
fn pure_fizz_positions() {
    let v = fizzbuzz_vec(15);
    // числа 3, 6, 9, 12 делятся на 3, но не на 5 → "Fizz"
    for pos in [2usize, 5, 8, 11] {
        assert_eq!(v[pos], "Fizz", "позиция {}: ожидали Fizz", pos);
    }
}

#[test]
fn pure_buzz_positions() {
    let v = fizzbuzz_vec(15);
    // числа 5, 10 делятся на 5, но не на 3 → "Buzz"
    for pos in [4usize, 9] {
        assert_eq!(v[pos], "Buzz", "позиция {}: ожидали Buzz", pos);
    }
}

#[test]
fn plain_numbers_are_string_representation() {
    let v = fizzbuzz_vec(15);
    // 1,2,4,7,8,11,13,14 — обычные числа
    let plain: &[(usize, &str)] = &[
        (0, "1"), (1, "2"), (3, "4"), (6, "7"),
        (7, "8"), (10, "11"), (12, "13"), (13, "14"),
    ];
    for &(idx, expected) in plain {
        assert_eq!(v[idx], expected, "позиция {idx}: ожидали {expected}");
    }
}

// ── Детерминированный xorshift64* ГПСЧ (без внешних крейтов) ─────────────────

fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}

/// Возвращает случайное u64 в диапазоне [lo, hi] включительно.
fn rand_range(state: &mut u64, lo: u64, hi: u64) -> u64 {
    let span = hi - lo + 1;
    lo + next_u64(state) % span
}

// ── Property-тесты ─────────────────────────────────────────────────────────────

/// Свойство: длина вектора всегда равна n.
#[test]
fn prop_length_equals_n() {
    let mut rng: u64 = 0xFACE_B00C_1234_5678;
    for _ in 0..500 {
        let n = rand_range(&mut rng, 0, 200);
        let v = fizzbuzz_vec(n);
        assert_eq!(
            v.len(),
            n as usize,
            "fizzbuzz_vec({n}).len() == {}, ожидали {n}",
            v.len()
        );
    }
}

/// Свойство: каждый элемент на позиции, соответствующей числу i (1-based),
/// — это либо "FizzBuzz"/"Fizz"/"Buzz", либо строковое представление i.
/// Проверяем через независимый оракул прямо в тесте.
#[test]
fn prop_each_element_matches_oracle() {
    let mut rng: u64 = 0xDEAD_BEEF_0101_ABCD;
    for _ in 0..500 {
        let n = rand_range(&mut rng, 1, 150);
        let v = fizzbuzz_vec(n);
        for (idx, s) in v.iter().enumerate() {
            let i = (idx + 1) as u64; // номер числа (1-based)
            let expected = if i % 15 == 0 {
                "FizzBuzz".to_string()
            } else if i % 3 == 0 {
                "Fizz".to_string()
            } else if i % 5 == 0 {
                "Buzz".to_string()
            } else {
                i.to_string()
            };
            assert_eq!(
                s, &expected,
                "fizzbuzz_vec({n})[{idx}] == {s:?}, ожидали {expected:?}"
            );
        }
    }
}

/// Свойство: позиции кратных 15 всегда содержат "FizzBuzz".
#[test]
fn prop_multiples_of_15_are_fizzbuzz() {
    let mut rng: u64 = 0x1111_CAFE_FEED_2222;
    for _ in 0..300 {
        let n = rand_range(&mut rng, 15, 300);
        let v = fizzbuzz_vec(n);
        // Проверяем только первые несколько кратных 15, чтобы не повторять всю Oracle выше
        let mut i = 15u64;
        while i <= n {
            let idx = (i - 1) as usize;
            assert_eq!(
                v[idx], "FizzBuzz",
                "fizzbuzz_vec({n})[{idx}] для числа {i} должно быть FizzBuzz, получили {:?}",
                v[idx]
            );
            i += 15;
        }
    }
}

/// Свойство: ни один элемент вектора не является пустой строкой.
#[test]
fn prop_no_empty_strings() {
    let mut rng: u64 = 0x9ABC_DEF0_1234_5678;
    for _ in 0..500 {
        let n = rand_range(&mut rng, 0, 200);
        let v = fizzbuzz_vec(n);
        for (idx, s) in v.iter().enumerate() {
            assert!(!s.is_empty(), "fizzbuzz_vec({n})[{idx}] — пустая строка");
        }
    }
}

/// Свойство: fizzbuzz_vec(n) — это префикс fizzbuzz_vec(n+k) для любого k >= 0.
/// Иными словами, добавление элементов в конец не меняет уже вычисленные.
#[test]
fn prop_prefix_consistency() {
    let mut rng: u64 = 0x5678_9012_3456_7890;
    for _ in 0..300 {
        let n = rand_range(&mut rng, 0, 100);
        let k = rand_range(&mut rng, 0, 50);
        let v_short = fizzbuzz_vec(n);
        let v_long = fizzbuzz_vec(n + k);
        // первые n элементов длинного должны совпадать с коротким
        assert_eq!(
            &v_long[..n as usize],
            v_short.as_slice(),
            "fizzbuzz_vec({}) не является префиксом fizzbuzz_vec({})",
            n,
            n + k
        );
    }
}
