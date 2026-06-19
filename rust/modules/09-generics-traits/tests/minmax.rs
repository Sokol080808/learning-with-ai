// Тесты для MinMaxTracker (модуль 09 — generics & traits).
//
// Файл разделён на две части:
//   1. Детерминированные примерные тесты (конкретный ввод → конкретный вывод).
//   2. Рандомизированные property-тесты с детерминированным xorshift64* RNG.
//
// Тесты проверяют только публичный API трекера и не зависят от деталей реализации.

use m09_generics::MinMaxTracker;

// ---------------------------------------------------------------------------
// Вспомогательный ГПСЧ (xorshift64*) — тот же, что в props.rs
// ---------------------------------------------------------------------------

fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}

fn in_range(state: &mut u64, lo: i64, hi: i64) -> i64 {
    let span = (hi - lo + 1) as u64;
    lo + (next_u64(state) % span) as i64
}

// ---------------------------------------------------------------------------
// Часть 1. Детерминированные примерные тесты
// ---------------------------------------------------------------------------

// --- пустой трекер ---

#[test]
fn empty_tracker_min_is_none() {
    let t: MinMaxTracker<i32> = MinMaxTracker::new();
    assert_eq!(t.min(), None);
}

#[test]
fn empty_tracker_max_is_none() {
    let t: MinMaxTracker<i32> = MinMaxTracker::new();
    assert_eq!(t.max(), None);
}

#[test]
fn empty_tracker_count_is_zero() {
    let t: MinMaxTracker<i32> = MinMaxTracker::new();
    assert_eq!(t.count(), 0);
}

// --- один элемент ---

#[test]
fn single_push_min_equals_value() {
    let mut t = MinMaxTracker::new();
    t.push(42_i32);
    assert_eq!(t.min(), Some(42));
}

#[test]
fn single_push_max_equals_value() {
    let mut t = MinMaxTracker::new();
    t.push(42_i32);
    assert_eq!(t.max(), Some(42));
}

#[test]
fn single_push_count_is_one() {
    let mut t = MinMaxTracker::new();
    t.push(42_i32);
    assert_eq!(t.count(), 1);
}

// --- несколько элементов, i32 ---

#[test]
fn i32_sequence_min_and_max() {
    let mut t = MinMaxTracker::new();
    for v in [3, 1, 4, 1, 5, 9, 2, 6] {
        t.push(v);
    }
    assert_eq!(t.min(), Some(1));
    assert_eq!(t.max(), Some(9));
    assert_eq!(t.count(), 8);
}

#[test]
fn i32_all_negative() {
    let mut t = MinMaxTracker::new();
    for v in [-7, -3, -10, -1] {
        t.push(v);
    }
    assert_eq!(t.min(), Some(-10));
    assert_eq!(t.max(), Some(-1));
}

#[test]
fn i32_all_same_value() {
    let mut t = MinMaxTracker::new();
    for _ in 0..5 {
        t.push(7_i32);
    }
    assert_eq!(t.min(), Some(7));
    assert_eq!(t.max(), Some(7));
    assert_eq!(t.count(), 5);
}

#[test]
fn i32_boundary_min_i32() {
    let mut t = MinMaxTracker::new();
    t.push(i32::MIN);
    t.push(0_i32);
    t.push(i32::MAX);
    assert_eq!(t.min(), Some(i32::MIN));
    assert_eq!(t.max(), Some(i32::MAX));
    assert_eq!(t.count(), 3);
}

// --- f64 ---

#[test]
fn f64_basic_sequence() {
    let mut t = MinMaxTracker::new();
    for v in [1.5_f64, -0.5, 3.14, 2.71] {
        t.push(v);
    }
    assert_eq!(t.min(), Some(-0.5));
    assert_eq!(t.max(), Some(3.14));
    assert_eq!(t.count(), 4);
}

#[test]
fn f64_single_element() {
    let mut t = MinMaxTracker::new();
    t.push(2.718_f64);
    assert_eq!(t.min(), Some(2.718));
    assert_eq!(t.max(), Some(2.718));
}

#[test]
fn f64_negative_values() {
    let mut t = MinMaxTracker::new();
    for v in [-1.0_f64, -5.5, -0.1] {
        t.push(v);
    }
    assert_eq!(t.min(), Some(-5.5));
    assert_eq!(t.max(), Some(-0.1));
}

// --- char ---

#[test]
fn char_basic_sequence() {
    let mut t = MinMaxTracker::new();
    for c in ['f', 'a', 'z', 'm'] {
        t.push(c);
    }
    assert_eq!(t.min(), Some('a'));
    assert_eq!(t.max(), Some('z'));
    assert_eq!(t.count(), 4);
}

#[test]
fn char_single() {
    let mut t = MinMaxTracker::new();
    t.push('q');
    assert_eq!(t.min(), Some('q'));
    assert_eq!(t.max(), Some('q'));
}

#[test]
fn char_boundary_ascii() {
    let mut t = MinMaxTracker::new();
    t.push('A'); // 65
    t.push('Z'); // 90
    t.push('a'); // 97
    t.push('z'); // 122
    assert_eq!(t.min(), Some('A'));
    assert_eq!(t.max(), Some('z'));
}

// --- count растёт ровно на 1 за каждый push ---

#[test]
fn count_increments_correctly() {
    let mut t = MinMaxTracker::new();
    assert_eq!(t.count(), 0);
    for i in 1..=10 {
        t.push(i as i32);
        assert_eq!(t.count(), i);
    }
}

// --- min/max обновляются по ходу ---

#[test]
fn min_max_update_incrementally() {
    let mut t = MinMaxTracker::new();
    t.push(5_i32);
    assert_eq!(t.min(), Some(5));
    assert_eq!(t.max(), Some(5));

    t.push(10);
    assert_eq!(t.min(), Some(5));
    assert_eq!(t.max(), Some(10));

    t.push(1);
    assert_eq!(t.min(), Some(1));
    assert_eq!(t.max(), Some(10));

    t.push(7);
    assert_eq!(t.min(), Some(1));
    assert_eq!(t.max(), Some(10));
}

// ---------------------------------------------------------------------------
// Часть 2. Рандомизированные property-тесты
// ---------------------------------------------------------------------------

/// Инвариант: min() <= каждому элементу, max() >= каждому элементу (i32).
#[test]
fn prop_i32_min_le_all_elements_max_ge_all_elements() {
    let mut rng: u64 = 0xDEAD_BEEF_0101_0101;
    for _ in 0..500 {
        let len = in_range(&mut rng, 1, 30) as usize;
        let values: Vec<i32> = (0..len)
            .map(|_| in_range(&mut rng, -10_000, 10_000) as i32)
            .collect();

        let mut t = MinMaxTracker::new();
        for &v in &values {
            t.push(v);
        }
        let mn = t.min().expect("min should be Some after pushes");
        let mx = t.max().expect("max should be Some after pushes");

        for &v in &values {
            assert!(mn <= v, "min={mn} > element={v} in {:?}", values);
            assert!(mx >= v, "max={mx} < element={v} in {:?}", values);
        }
    }
}

/// Инвариант: min() и max() — реально присутствуют в поданных значениях (i32).
#[test]
fn prop_i32_min_max_are_contained_in_input() {
    let mut rng: u64 = 0xCAFE_BABE_1234_5678;
    for _ in 0..500 {
        let len = in_range(&mut rng, 1, 25) as usize;
        let values: Vec<i32> = (0..len)
            .map(|_| in_range(&mut rng, -10_000, 10_000) as i32)
            .collect();

        let mut t = MinMaxTracker::new();
        for &v in &values {
            t.push(v);
        }
        let mn = t.min().unwrap();
        let mx = t.max().unwrap();

        assert!(values.contains(&mn), "min={mn} not in {:?}", values);
        assert!(values.contains(&mx), "max={mx} not in {:?}", values);
    }
}

/// Оракул: min() и max() совпадают с наивным поиском через итератор (i32).
#[test]
fn prop_i32_agrees_with_naive_oracle() {
    let mut rng: u64 = 0x9999_AAAA_BBBB_CCCC;
    for _ in 0..500 {
        let len = in_range(&mut rng, 1, 20) as usize;
        let values: Vec<i32> = (0..len)
            .map(|_| in_range(&mut rng, -10_000, 10_000) as i32)
            .collect();

        let mut t = MinMaxTracker::new();
        for &v in &values {
            t.push(v);
        }

        let expected_min = *values.iter().min().unwrap();
        let expected_max = *values.iter().max().unwrap();

        assert_eq!(t.min(), Some(expected_min), "min mismatch for {:?}", values);
        assert_eq!(t.max(), Some(expected_max), "max mismatch for {:?}", values);
    }
}

/// count() точно равен числу поданных значений (i32).
#[test]
fn prop_i32_count_equals_push_count() {
    let mut rng: u64 = 0xF0F0_F0F0_F0F0_F0F0;
    for _ in 0..500 {
        let len = in_range(&mut rng, 0, 50) as usize;
        let mut t: MinMaxTracker<i32> = MinMaxTracker::new();
        for i in 0..len {
            t.push(i as i32);
        }
        assert_eq!(t.count(), len, "count mismatch: expected {len}");
    }
}

/// Оракул: min() и max() совпадают с наивным поиском для f64.
#[test]
fn prop_f64_agrees_with_naive_oracle() {
    let mut rng: u64 = 0x1A2B_3C4D_5E6F_7080;
    for _ in 0..500 {
        let len = in_range(&mut rng, 1, 20) as usize;
        // Генерируем значения как целые [-1000, 1000], конвертируем в f64 (избегаем NaN).
        let values: Vec<f64> = (0..len)
            .map(|_| in_range(&mut rng, -1_000, 1_000) as f64)
            .collect();

        let mut t = MinMaxTracker::new();
        for &v in &values {
            t.push(v);
        }

        // Наивный оракул — partial_min/max через fold.
        let expected_min = values.iter().cloned().fold(f64::INFINITY, f64::min);
        let expected_max = values.iter().cloned().fold(f64::NEG_INFINITY, f64::max);

        assert_eq!(t.min(), Some(expected_min), "f64 min mismatch for {:?}", values);
        assert_eq!(t.max(), Some(expected_max), "f64 max mismatch for {:?}", values);
    }
}

/// Оракул: min() и max() совпадают с наивным поиском для char.
#[test]
fn prop_char_agrees_with_naive_oracle() {
    // Алфавит: строчные a-z (26 символов).
    const ALPHABET: &[char] = &[
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    ];
    let mut rng: u64 = 0xBEEF_CAFE_DEAD_1234;
    for _ in 0..500 {
        let len = in_range(&mut rng, 1, 20) as usize;
        let values: Vec<char> = (0..len)
            .map(|_| ALPHABET[in_range(&mut rng, 0, 25) as usize])
            .collect();

        let mut t = MinMaxTracker::new();
        for &c in &values {
            t.push(c);
        }

        let expected_min = *values.iter().min().unwrap();
        let expected_max = *values.iter().max().unwrap();

        assert_eq!(t.min(), Some(expected_min), "char min mismatch for {:?}", values);
        assert_eq!(t.max(), Some(expected_max), "char max mismatch for {:?}", values);
    }
}

/// Порядок элементов не влияет на результат (перестановка инвариантна).
#[test]
fn prop_i32_order_independent() {
    let mut rng: u64 = 0x1111_2222_3333_4444;
    for _ in 0..300 {
        let len = in_range(&mut rng, 2, 15) as usize;
        let values: Vec<i32> = (0..len)
            .map(|_| in_range(&mut rng, -1_000, 1_000) as i32)
            .collect();

        // Прямой порядок.
        let mut t1 = MinMaxTracker::new();
        for &v in &values {
            t1.push(v);
        }

        // Обратный порядок.
        let mut t2 = MinMaxTracker::new();
        for &v in values.iter().rev() {
            t2.push(v);
        }

        assert_eq!(t1.min(), t2.min(), "min differs by order for {:?}", values);
        assert_eq!(t1.max(), t2.max(), "max differs by order for {:?}", values);
    }
}

/// min() <= max() всегда (когда трекер непустой).
#[test]
fn prop_i32_min_le_max() {
    let mut rng: u64 = 0x5555_6666_7777_8888;
    for _ in 0..500 {
        let len = in_range(&mut rng, 1, 20) as usize;
        let mut t: MinMaxTracker<i32> = MinMaxTracker::new();
        for _ in 0..len {
            t.push(in_range(&mut rng, -10_000, 10_000) as i32);
        }
        let mn = t.min().unwrap();
        let mx = t.max().unwrap();
        assert!(mn <= mx, "min={mn} > max={mx}");
    }
}
