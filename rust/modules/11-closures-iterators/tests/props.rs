// Рандомизированные property-тесты для m11_iterators.
// Детерминированный ГПСЧ — без внешних крейтов, воспроизводимо.

use m11_iterators::*;

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

// ── apply ────────────────────────────────────────────────────────────────────

/// apply(f, x) == f(x) для любого x и любого pure-замыкания.
/// Оракул: вычислить то же самое напрямую.
#[test]
fn prop_apply_identity() {
    let mut rng: u64 = 0xDEAD_BEEF_1234_5678;
    for _ in 0..1000 {
        let x = in_range(&mut rng, -10_000, 10_000);
        // f(x) = x  (тождественная функция)
        assert_eq!(apply(|n| n, x), x);
    }
}

/// apply(f, x) == f(x) для аддитивного смещения.
#[test]
fn prop_apply_matches_direct_call() {
    let mut rng: u64 = 0xCAFE_BABE_0000_0001;
    for _ in 0..1000 {
        let x = in_range(&mut rng, -5_000, 5_000);
        let k = in_range(&mut rng, -100, 100);
        // оракул — вызвать то же выражение напрямую
        assert_eq!(apply(|n| n + k, x), x + k);
    }
}

/// apply(f, x) == f(x) для умножения — и результат совпадает с прямым вычислением.
#[test]
fn prop_apply_multiply_oracle() {
    let mut rng: u64 = 0x1111_2222_3333_4444;
    for _ in 0..1000 {
        let x = in_range(&mut rng, -1_000, 1_000);
        let k = in_range(&mut rng, -10, 10);
        assert_eq!(apply(|n| n * k, x), x * k);
    }
}

// ── make_adder ───────────────────────────────────────────────────────────────

/// make_adder(n)(x) == x + n  — основное свойство.
#[test]
fn prop_make_adder_oracle() {
    let mut rng: u64 = 0xAAAA_BBBB_CCCC_DDDD;
    for _ in 0..1000 {
        let n = in_range(&mut rng, -5_000, 5_000);
        let x = in_range(&mut rng, -5_000, 5_000);
        let adder = make_adder(n);
        assert_eq!(adder(x), x + n);
    }
}

/// make_adder(0)(x) == x  — нейтральный элемент сложения.
#[test]
fn prop_make_adder_zero_is_identity() {
    let mut rng: u64 = 0x5555_6666_7777_8888;
    let adder = make_adder(0);
    for _ in 0..1000 {
        let x = in_range(&mut rng, -10_000, 10_000);
        assert_eq!(adder(x), x);
    }
}

/// Многократный вызов одного и того же adder возвращает один результат (чистота).
#[test]
fn prop_make_adder_repeatable() {
    let mut rng: u64 = 0x9999_AAAA_BBBB_CCCC;
    for _ in 0..500 {
        let n = in_range(&mut rng, -1_000, 1_000);
        let x = in_range(&mut rng, -1_000, 1_000);
        let adder = make_adder(n);
        // вызываем дважды — результат должен совпасть
        assert_eq!(adder(x), adder(x));
    }
}

/// Два независимых adder не мешают друг другу.
#[test]
fn prop_make_adder_independent() {
    let mut rng: u64 = 0x1234_5678_9ABC_DEF0;
    for _ in 0..500 {
        let n1 = in_range(&mut rng, -500, 500);
        let n2 = in_range(&mut rng, -500, 500);
        let x = in_range(&mut rng, -1_000, 1_000);
        let a1 = make_adder(n1);
        let a2 = make_adder(n2);
        assert_eq!(a1(x), x + n1);
        assert_eq!(a2(x), x + n2);
    }
}

// ── sum_of_squares ───────────────────────────────────────────────────────────

/// sum_of_squares всегда >= 0 (сумма неотрицательных слагаемых).
#[test]
fn prop_sum_of_squares_nonnegative() {
    let mut rng: u64 = 0xFEED_FACE_DEAD_BEEF;
    for _ in 0..500 {
        let len = (next_u64(&mut rng) % 20) as usize;
        let v: Vec<i64> = (0..len)
            .map(|_| in_range(&mut rng, -100, 100))
            .collect();
        assert!(sum_of_squares(&v) >= 0);
    }
}

/// sum_of_squares == сумма x*x по наивному циклу (оракул).
#[test]
fn prop_sum_of_squares_matches_naive_loop() {
    let mut rng: u64 = 0x0F0F_0F0F_F0F0_F0F0;
    for _ in 0..500 {
        let len = (next_u64(&mut rng) % 30) as usize;
        let v: Vec<i64> = (0..len)
            .map(|_| in_range(&mut rng, -300, 300))
            .collect();
        // оракул: наивный цикл
        let expected: i64 = v.iter().fold(0i64, |acc, &x| acc + x * x);
        assert_eq!(sum_of_squares(&v), expected);
    }
}

/// Пустой срез даёт 0.
#[test]
fn prop_sum_of_squares_empty() {
    assert_eq!(sum_of_squares(&[]), 0);
}

/// sum_of_squares нечувствителен к знаку элементов: x и -x дают тот же квадрат.
#[test]
fn prop_sum_of_squares_sign_invariant() {
    let mut rng: u64 = 0xABCD_EF01_2345_6789;
    for _ in 0..500 {
        let len = (next_u64(&mut rng) % 20) as usize;
        let v: Vec<i64> = (0..len)
            .map(|_| in_range(&mut rng, -200, 200))
            .collect();
        let neg_v: Vec<i64> = v.iter().map(|&x| -x).collect();
        assert_eq!(sum_of_squares(&v), sum_of_squares(&neg_v));
    }
}

/// Одноэлементный срез: sum_of_squares(&[x]) == x*x.
#[test]
fn prop_sum_of_squares_single_element() {
    let mut rng: u64 = 0x1357_2468_9BDF_0ACE;
    for _ in 0..1000 {
        let x = in_range(&mut rng, -1_000, 1_000);
        assert_eq!(sum_of_squares(&[x]), x * x);
    }
}

// ── evens ────────────────────────────────────────────────────────────────────

/// Все элементы результата evens чётные.
#[test]
fn prop_evens_all_results_are_even() {
    let mut rng: u64 = 0x2468_ACE0_1357_BDF9;
    for _ in 0..500 {
        let len = (next_u64(&mut rng) % 30) as usize;
        let v: Vec<i64> = (0..len)
            .map(|_| in_range(&mut rng, -200, 200))
            .collect();
        for &e in evens(&v).iter() {
            assert_eq!(e % 2, 0, "evens вернул нечётное: {e}");
        }
    }
}

/// Ни один нечётный элемент из входа не должен попасть в результат.
#[test]
fn prop_evens_no_odds_in_result() {
    let mut rng: u64 = 0x13BD_F902_4680_ACE1;
    for _ in 0..500 {
        let len = (next_u64(&mut rng) % 30) as usize;
        let v: Vec<i64> = (0..len)
            .map(|_| in_range(&mut rng, -200, 200))
            .collect();
        let result = evens(&v);
        for &e in &result {
            assert!(e % 2 == 0);
        }
    }
}

/// evens == наивный filter-collect по оракулу.
#[test]
fn prop_evens_matches_naive_filter() {
    let mut rng: u64 = 0xC0DE_BABE_CAFE_FEED;
    for _ in 0..500 {
        let len = (next_u64(&mut rng) % 30) as usize;
        let v: Vec<i64> = (0..len)
            .map(|_| in_range(&mut rng, -300, 300))
            .collect();
        // оракул: std iterator напрямую
        let expected: Vec<i64> = v.iter().copied().filter(|&x| x % 2 == 0).collect();
        assert_eq!(evens(&v), expected);
    }
}

/// Порядок сохраняется: evens — стабильный фильтр.
#[test]
fn prop_evens_preserves_order() {
    let mut rng: u64 = 0xBEEF_DEAD_1234_5678;
    for _ in 0..500 {
        let len = (next_u64(&mut rng) % 20) as usize;
        let v: Vec<i64> = (0..len)
            .map(|_| in_range(&mut rng, -200, 200))
            .collect();
        let result = evens(&v);
        // Все элементы result должны появляться в v в том же порядке.
        let mut v_iter = v.iter();
        for &e in &result {
            // advance v_iter до первого вхождения e (с того места где мы остановились)
            let found = v_iter.any(|&x| x == e);
            assert!(found, "порядок нарушен: {e} не найден в остатке v");
        }
    }
}

/// Если в срезе нет чётных — результат пустой.
#[test]
fn prop_evens_all_odd_gives_empty() {
    let mut rng: u64 = 0x9753_1BDF_2468_ACE0;
    for _ in 0..500 {
        let len = (next_u64(&mut rng) % 20) as usize;
        // строим вектор только из нечётных чисел
        let v: Vec<i64> = (0..len)
            .map(|_| {
                let n = in_range(&mut rng, -100, 100);
                // гарантируем нечётность
                if n % 2 == 0 { n + 1 } else { n }
            })
            .collect();
        assert!(evens(&v).is_empty(), "ожидали пустой результат для нечётного входа");
    }
}

/// Длина evens <= длина входа.
#[test]
fn prop_evens_length_le_input() {
    let mut rng: u64 = 0x0102_0304_0506_0708;
    for _ in 0..500 {
        let len = (next_u64(&mut rng) % 30) as usize;
        let v: Vec<i64> = (0..len)
            .map(|_| in_range(&mut rng, -200, 200))
            .collect();
        assert!(evens(&v).len() <= v.len());
    }
}

// ── word_lengths ─────────────────────────────────────────────────────────────

/// Длина выходного вектора == длина входного.
#[test]
fn prop_word_lengths_output_len_equals_input_len() {
    // Фиксированный набор слов; randomize — выбираем подмножество.
    let words: &[&str] = &[
        "rust", "closure", "iterator", "map", "filter", "sum", "collect",
        "fn", "trait", "impl", "", "a", "hello", "world", "абв",
    ];
    let mut rng: u64 = 0xF00D_CAFE_1234_ABCD;
    for _ in 0..500 {
        let len = (next_u64(&mut rng) % (words.len() as u64 + 1)) as usize;
        let sample: Vec<&str> = (0..len)
            .map(|_| words[(next_u64(&mut rng) % words.len() as u64) as usize])
            .collect();
        assert_eq!(word_lengths(&sample).len(), sample.len());
    }
}

/// Каждый элемент результата == длина соответствующего слова (оракул: .len()).
#[test]
fn prop_word_lengths_each_matches_len() {
    let words: &[&str] = &[
        "rust", "closure", "iterator", "map", "filter", "sum", "collect",
        "fn", "trait", "impl", "", "a", "hello", "world",
    ];
    let mut rng: u64 = 0xACED_CAFE_BABE_0011;
    for _ in 0..500 {
        let len = (next_u64(&mut rng) % (words.len() as u64 + 1)) as usize;
        let sample: Vec<&str> = (0..len)
            .map(|_| words[(next_u64(&mut rng) % words.len() as u64) as usize])
            .collect();
        let result = word_lengths(&sample);
        for (i, (&word, &got)) in sample.iter().zip(result.iter()).enumerate() {
            assert_eq!(got, word.len(), "позиция {i}: слово «{word}»");
        }
    }
}

/// Пустой срез -> пустой результат.
#[test]
fn prop_word_lengths_empty_input() {
    let empty: &[&str] = &[];
    assert_eq!(word_lengths(empty), Vec::<usize>::new());
}

/// Однобуквенные слова -> все длины == 1.
#[test]
fn prop_word_lengths_single_char_words() {
    let mut rng: u64 = 0x5A5A_5A5A_A5A5_A5A5;
    let single_chars: &[&str] = &["a", "b", "c", "d", "e", "f", "g", "h", "i", "j"];
    for _ in 0..500 {
        let len = (next_u64(&mut rng) % (single_chars.len() as u64 + 1)) as usize;
        let sample: Vec<&str> = (0..len)
            .map(|_| single_chars[(next_u64(&mut rng) % single_chars.len() as u64) as usize])
            .collect();
        for &l in word_lengths(&sample).iter() {
            assert_eq!(l, 1);
        }
    }
}

/// Если все слова пустые, все длины == 0.
#[test]
fn prop_word_lengths_all_empty_words() {
    let mut rng: u64 = 0x1234_1234_5678_5678;
    for _ in 0..500 {
        let len = (next_u64(&mut rng) % 20) as usize;
        let sample: Vec<&str> = vec![""; len];
        for &l in word_lengths(&sample).iter() {
            assert_eq!(l, 0);
        }
    }
}
