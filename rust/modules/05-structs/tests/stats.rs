// Тесты для Stats — аккумулятора статистики.
// Два раздела:
//   (a) детерминированные примеры (конкретный вход → конкретный выход, граничные случаи);
//   (b) рандомизированные property-тесты (инварианты / оракул).
//
// Запуск: ./rust/run.sh 05  или  cargo test -p m05_structs

use m05_structs::Stats;

// ---------------------------------------------------------------------------
// (a) Детерминированные тесты
// ---------------------------------------------------------------------------

/// Пустой аккумулятор: count == 0, все Option-методы возвращают None.
#[test]
fn stats_empty_count_is_zero() {
    let s = Stats::new();
    assert_eq!(s.count(), 0);
}

#[test]
fn stats_empty_mean_is_none() {
    let s = Stats::new();
    assert_eq!(s.mean(), None);
}

#[test]
fn stats_empty_min_is_none() {
    let s = Stats::new();
    assert_eq!(s.min(), None);
}

#[test]
fn stats_empty_max_is_none() {
    let s = Stats::new();
    assert_eq!(s.max(), None);
}

/// Одно число: count == 1, mean/min/max == то же число.
#[test]
fn stats_single_value() {
    let mut s = Stats::new();
    s.push(42.0);
    assert_eq!(s.count(), 1);
    assert_eq!(s.mean(), Some(42.0));
    assert_eq!(s.min(), Some(42.0));
    assert_eq!(s.max(), Some(42.0));
}

/// Одно отрицательное число.
#[test]
fn stats_single_negative() {
    let mut s = Stats::new();
    s.push(-7.5);
    assert_eq!(s.count(), 1);
    assert_eq!(s.mean(), Some(-7.5));
    assert_eq!(s.min(), Some(-7.5));
    assert_eq!(s.max(), Some(-7.5));
}

/// Известная последовательность: 1, 2, 3, 4, 5.
#[test]
fn stats_sequence_1_to_5() {
    let mut s = Stats::new();
    for v in [1.0_f64, 2.0, 3.0, 4.0, 5.0] {
        s.push(v);
    }
    assert_eq!(s.count(), 5);
    assert_eq!(s.mean(), Some(3.0));  // (1+2+3+4+5)/5 = 15/5 = 3
    assert_eq!(s.min(), Some(1.0));
    assert_eq!(s.max(), Some(5.0));
}

/// Последовательность в обратном порядке — результат тот же.
#[test]
fn stats_sequence_5_to_1() {
    let mut s = Stats::new();
    for v in [5.0_f64, 4.0, 3.0, 2.0, 1.0] {
        s.push(v);
    }
    assert_eq!(s.count(), 5);
    assert_eq!(s.mean(), Some(3.0));
    assert_eq!(s.min(), Some(1.0));
    assert_eq!(s.max(), Some(5.0));
}

/// Все нули: mean == 0, min == 0, max == 0.
#[test]
fn stats_all_zeros() {
    let mut s = Stats::new();
    for _ in 0..100 {
        s.push(0.0);
    }
    assert_eq!(s.count(), 100);
    assert_eq!(s.mean(), Some(0.0));
    assert_eq!(s.min(), Some(0.0));
    assert_eq!(s.max(), Some(0.0));
}

/// Отрицательные и положительные числа.
#[test]
fn stats_mixed_signs() {
    let mut s = Stats::new();
    for v in [-3.0_f64, -1.0, 0.0, 1.0, 3.0] {
        s.push(v);
    }
    assert_eq!(s.count(), 5);
    assert_eq!(s.mean(), Some(0.0));  // сумма = 0
    assert_eq!(s.min(), Some(-3.0));
    assert_eq!(s.max(), Some(3.0));
}

/// count увеличивается с каждым push.
#[test]
fn stats_count_increments() {
    let mut s = Stats::new();
    for i in 0..10 {
        assert_eq!(s.count(), i);
        s.push(i as f64);
    }
    assert_eq!(s.count(), 10);
}

/// Минимум и максимум корректно обновляются при добавлении нового экстремума.
#[test]
fn stats_min_max_update_on_new_extreme() {
    let mut s = Stats::new();
    s.push(5.0);
    assert_eq!(s.min(), Some(5.0));
    assert_eq!(s.max(), Some(5.0));

    s.push(10.0);
    assert_eq!(s.min(), Some(5.0));
    assert_eq!(s.max(), Some(10.0));

    s.push(-2.0);
    assert_eq!(s.min(), Some(-2.0));
    assert_eq!(s.max(), Some(10.0));
}

/// Два одинаковых значения: min == max == то значение.
#[test]
fn stats_two_equal_values() {
    let mut s = Stats::new();
    s.push(7.0);
    s.push(7.0);
    assert_eq!(s.count(), 2);
    assert_eq!(s.mean(), Some(7.0));
    assert_eq!(s.min(), Some(7.0));
    assert_eq!(s.max(), Some(7.0));
}

// ---------------------------------------------------------------------------
// (b) Рандомизированные property-тесты
// ---------------------------------------------------------------------------
// Тот же xorshift64* ГПСЧ, что и в props.rs — детерминированный, без крейтов.

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

/// count совпадает с количеством вызовов push.
#[test]
fn prop_stats_count_matches_push_calls() {
    let mut rng: u64 = 0xABCD_EF01_2345_6789;
    for n in 0..500_usize {
        let mut s = Stats::new();
        for _ in 0..n {
            let v = in_range(&mut rng, -10_000, 10_000) as f64;
            s.push(v);
        }
        assert_eq!(s.count(), n, "count should equal number of push calls");
    }
}

/// После хотя бы одного push все Option-методы возвращают Some.
#[test]
fn prop_stats_some_after_nonempty() {
    let mut rng: u64 = 0x1234_5678_ABCD_EF01;
    for _ in 0..500 {
        let mut s = Stats::new();
        let n = 1 + (next_u64(&mut rng) % 50) as usize;
        for _ in 0..n {
            let v = in_range(&mut rng, -1_000, 1_000) as f64;
            s.push(v);
        }
        assert!(s.mean().is_some(), "mean should be Some after {} pushes", n);
        assert!(s.min().is_some(), "min should be Some after {} pushes", n);
        assert!(s.max().is_some(), "max should be Some after {} pushes", n);
    }
}

/// min <= mean <= max для непустого аккумулятора.
#[test]
fn prop_stats_min_le_mean_le_max() {
    let mut rng: u64 = 0xDEAD_BEEF_0102_0304;
    for _ in 0..500 {
        let mut s = Stats::new();
        let n = 1 + (next_u64(&mut rng) % 100) as usize;
        for _ in 0..n {
            let v = in_range(&mut rng, -1_000, 1_000) as f64;
            s.push(v);
        }
        let mn = s.min().unwrap();
        let mx = s.max().unwrap();
        let mean = s.mean().unwrap();
        assert!(
            mn <= mean && mean <= mx,
            "min({}) <= mean({}) <= max({}) violated",
            mn, mean, mx
        );
    }
}

/// min <= max для непустого аккумулятора.
#[test]
fn prop_stats_min_le_max() {
    let mut rng: u64 = 0x5566_7788_99AA_BBCC;
    for _ in 0..1000 {
        let mut s = Stats::new();
        let n = 1 + (next_u64(&mut rng) % 50) as usize;
        for _ in 0..n {
            let v = in_range(&mut rng, -10_000, 10_000) as f64;
            s.push(v);
        }
        assert!(
            s.min().unwrap() <= s.max().unwrap(),
            "min should be <= max"
        );
    }
}

/// mean совпадает с оракулом (независимо вычисленное среднее).
#[test]
fn prop_stats_mean_oracle() {
    let mut rng: u64 = 0xFACE_CAFE_BABE_F00D;
    for _ in 0..500 {
        let n = 1 + (next_u64(&mut rng) % 100) as usize;
        let mut values = Vec::with_capacity(n);
        for _ in 0..n {
            let v = in_range(&mut rng, -1_000, 1_000) as f64;
            values.push(v);
        }
        let mut s = Stats::new();
        for &v in &values {
            s.push(v);
        }
        let oracle_mean: f64 = values.iter().sum::<f64>() / n as f64;
        assert_eq!(
            s.mean().unwrap(),
            oracle_mean,
            "mean oracle mismatch for {} values",
            n
        );
    }
}

/// min совпадает с оракулом (fold по вектору).
#[test]
fn prop_stats_min_oracle() {
    let mut rng: u64 = 0x1111_AAAA_2222_BBBB;
    for _ in 0..500 {
        let n = 1 + (next_u64(&mut rng) % 100) as usize;
        let mut values = Vec::with_capacity(n);
        for _ in 0..n {
            let v = in_range(&mut rng, -1_000, 1_000) as f64;
            values.push(v);
        }
        let mut s = Stats::new();
        for &v in &values {
            s.push(v);
        }
        let oracle_min = values.iter().cloned().fold(f64::INFINITY, f64::min);
        assert_eq!(
            s.min().unwrap(),
            oracle_min,
            "min oracle mismatch for {:?}",
            values
        );
    }
}

/// max совпадает с оракулом (fold по вектору).
#[test]
fn prop_stats_max_oracle() {
    let mut rng: u64 = 0xCCCC_3333_DDDD_4444;
    for _ in 0..500 {
        let n = 1 + (next_u64(&mut rng) % 100) as usize;
        let mut values = Vec::with_capacity(n);
        for _ in 0..n {
            let v = in_range(&mut rng, -1_000, 1_000) as f64;
            values.push(v);
        }
        let mut s = Stats::new();
        for &v in &values {
            s.push(v);
        }
        let oracle_max = values.iter().cloned().fold(f64::NEG_INFINITY, f64::max);
        assert_eq!(
            s.max().unwrap(),
            oracle_max,
            "max oracle mismatch for {:?}",
            values
        );
    }
}

/// Монотонность: добавление числа за пределами [min, max] обновляет соответствующий экстремум.
#[test]
fn prop_stats_min_max_monotone() {
    let mut rng: u64 = 0xEEEE_5555_FFFF_6666;
    for _ in 0..500 {
        let mut s = Stats::new();
        // Заполняем [0, 100]
        let n = 1 + (next_u64(&mut rng) % 50) as usize;
        for _ in 0..n {
            let v = in_range(&mut rng, 0, 100) as f64;
            s.push(v);
        }
        let old_min = s.min().unwrap();
        let old_max = s.max().unwrap();

        // Добавляем заведомо меньшее число
        s.push(-999.0);
        assert_eq!(s.min().unwrap(), -999.0, "min should update to new minimum");
        assert_eq!(s.max().unwrap(), old_max, "max should not change");

        // Добавляем заведомо большее число
        s.push(9999.0);
        assert_eq!(s.min().unwrap(), -999.0, "min should stay -999");
        assert_eq!(s.max().unwrap(), 9999.0, "max should update to new maximum");

        let _ = old_min; // используем, чтобы компилятор не предупреждал
    }
}
