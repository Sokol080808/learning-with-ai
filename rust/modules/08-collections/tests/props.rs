// Randomized property tests for m08_collections.
// Uses a deterministic xorshift64* PRNG — no external crates, fully reproducible.

use m08_collections::*;
use std::collections::HashMap;

// ---------- детерминированный ГПСЧ (xorshift64*) ----------

fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}

// целое в диапазоне [lo, hi] включительно
fn in_range(state: &mut u64, lo: i64, hi: i64) -> i64 {
    let span = (hi - lo + 1) as u64;
    lo + (next_u64(state) % span) as i64
}

// ---------- helpers ----------

/// Строит вектор случайных i64 значений заданной длины из диапазона [lo, hi].
fn random_i64_vec(state: &mut u64, len: usize, lo: i64, hi: i64) -> Vec<i64> {
    (0..len).map(|_| in_range(state, lo, hi)).collect()
}

/// Строит строку из случайных ASCII-символов (printable, 0x20..=0x7E).
fn random_ascii_string(state: &mut u64, len: usize) -> String {
    (0..len)
        .map(|_| {
            let byte = in_range(state, 0x20, 0x7E) as u8;
            byte as char
        })
        .collect()
}

// ============================================================
// char_freq — property tests
// ============================================================

/// Свойство: сумма всех счётчиков == число char-ов в строке.
#[test]
fn prop_char_freq_total_count_equals_char_count() {
    let mut rng: u64 = 0xDEAD_BEEF_1234_5678;
    for _ in 0..800 {
        let len = in_range(&mut rng, 0, 50) as usize;
        let s = random_ascii_string(&mut rng, len);
        let freq = char_freq(&s);
        let total: usize = freq.values().sum();
        assert_eq!(
            total,
            s.chars().count(),
            "sum of counts should equal total char count for {:?}",
            s
        );
    }
}

/// Свойство: в карте нет нулевых счётчиков (каждый ключ виден хотя бы раз).
#[test]
fn prop_char_freq_no_zero_counts() {
    let mut rng: u64 = 0xCAFE_BABE_0000_0001;
    for _ in 0..800 {
        let len = in_range(&mut rng, 0, 60) as usize;
        let s = random_ascii_string(&mut rng, len);
        let freq = char_freq(&s);
        for (&ch, &count) in &freq {
            assert!(
                count > 0,
                "char {:?} has zero count in char_freq({:?})",
                ch,
                s
            );
        }
    }
}

/// Свойство (оракул): char_freq согласуется с наивным ручным подсчётом.
#[test]
fn prop_char_freq_agrees_with_naive_oracle() {
    let mut rng: u64 = 0x1111_2222_3333_4444;
    for _ in 0..700 {
        let len = in_range(&mut rng, 0, 40) as usize;
        let s = random_ascii_string(&mut rng, len);
        let freq = char_freq(&s);

        // Наивный оракул — независимый подсчёт через HashMap.
        let mut oracle: HashMap<char, usize> = HashMap::new();
        for c in s.chars() {
            *oracle.entry(c).or_insert(0) += 1;
        }

        assert_eq!(
            freq, oracle,
            "char_freq disagrees with naive oracle for {:?}",
            s
        );
    }
}

/// Свойство: символы, отсутствующие в строке, не появляются в карте.
#[test]
fn prop_char_freq_absent_chars_not_in_map() {
    let mut rng: u64 = 0xABCD_EF01_2345_6789;
    for _ in 0..500 {
        let len = in_range(&mut rng, 1, 30) as usize;
        let s = random_ascii_string(&mut rng, len);
        let freq = char_freq(&s);

        // Выбираем случайный char, которого точно нет в строке.
        // Используем символ вне ASCII-диапазона строки — Unicode code point 0x100..=0x17F.
        let cp = in_range(&mut rng, 0x100, 0x17F) as u32;
        if let Some(absent) = char::from_u32(cp) {
            if !s.contains(absent) {
                assert_eq!(
                    freq.get(&absent),
                    None,
                    "absent char {:?} should not appear in char_freq",
                    absent
                );
            }
        }
    }
}

/// Граничный случай: строка из одного символа.
#[test]
fn prop_char_freq_single_char() {
    let mut rng: u64 = 0x9999_AAAA_BBBB_CCCC;
    for _ in 0..500 {
        let byte = in_range(&mut rng, 0x21, 0x7E) as u8; // printable, non-space
        let s = (byte as char).to_string();
        let freq = char_freq(&s);
        assert_eq!(freq.len(), 1);
        assert_eq!(freq[&(byte as char)], 1);
    }
}

// ============================================================
// dedup_sorted — property tests
// ============================================================

/// Свойство: результат строго отсортирован.
#[test]
fn prop_dedup_sorted_result_is_sorted() {
    let mut rng: u64 = 0x5A5A_5A5A_5A5A_5A5A;
    for _ in 0..800 {
        let len = in_range(&mut rng, 0, 60) as usize;
        let v = random_i64_vec(&mut rng, len, -10_000, 10_000);
        let result = dedup_sorted(v);
        for window in result.windows(2) {
            assert!(
                window[0] < window[1],
                "result is not strictly ascending: {:?}",
                window
            );
        }
    }
}

/// Свойство: в результате нет дубликатов.
#[test]
fn prop_dedup_sorted_no_duplicates() {
    let mut rng: u64 = 0xF0F0_F0F0_DADA_DADA;
    for _ in 0..800 {
        let len = in_range(&mut rng, 0, 50) as usize;
        let v = random_i64_vec(&mut rng, len, -5_000, 5_000);
        let result = dedup_sorted(v);
        for window in result.windows(2) {
            assert_ne!(
                window[0], window[1],
                "duplicate found in dedup_sorted output"
            );
        }
    }
}

/// Свойство (оракул): множество элементов результата совпадает с исходным множеством.
#[test]
fn prop_dedup_sorted_same_set_as_input() {
    let mut rng: u64 = 0x0102_0304_0506_0708;
    for _ in 0..700 {
        let len = in_range(&mut rng, 0, 50) as usize;
        let v = random_i64_vec(&mut rng, len, -8_000, 8_000);

        // Оракул: независимое вычисление уникального отсортированного набора через HashSet.
        let mut oracle_set: std::collections::HashSet<i64> = v.iter().copied().collect();
        let mut oracle: Vec<i64> = oracle_set.drain().collect();
        oracle.sort();

        let result = dedup_sorted(v);
        assert_eq!(
            result, oracle,
            "dedup_sorted result does not match oracle unique-sorted set"
        );
    }
}

/// Свойство: длина результата не превышает длину входа.
#[test]
fn prop_dedup_sorted_len_le_input_len() {
    let mut rng: u64 = 0xBEEF_CAFE_DEAD_1234;
    for _ in 0..700 {
        let len = in_range(&mut rng, 0, 80) as usize;
        let v = random_i64_vec(&mut rng, len, -100, 100);
        let input_len = v.len();
        let result = dedup_sorted(v);
        assert!(
            result.len() <= input_len,
            "output len {} > input len {}",
            result.len(),
            input_len
        );
    }
}

/// Свойство: вход с одним различным значением даёт вектор длиной 1.
#[test]
fn prop_dedup_sorted_all_equal_gives_single() {
    let mut rng: u64 = 0x1234_5678_9ABC_DEF0;
    for _ in 0..500 {
        let val = in_range(&mut rng, -1_000, 1_000);
        let count = in_range(&mut rng, 1, 30) as usize;
        let v = vec![val; count];
        let result = dedup_sorted(v);
        assert_eq!(result, vec![val], "all-equal input should give [val]");
    }
}

// ============================================================
// word_count — property tests
// ============================================================

/// Свойство: сумма счётчиков == число слов (split_whitespace).
#[test]
fn prop_word_count_total_equals_word_count() {
    let mut rng: u64 = 0xC0DE_CAFE_1111_2222;
    // Используем небольшой алфавит слов, чтобы были повторения.
    let words_pool = ["alpha", "beta", "gamma", "delta", "epsilon"];
    for _ in 0..700 {
        let n_words = in_range(&mut rng, 0, 20) as usize;
        let mut text_parts: Vec<&str> = Vec::new();
        for _ in 0..n_words {
            let idx = (next_u64(&mut rng) % words_pool.len() as u64) as usize;
            text_parts.push(words_pool[idx]);
        }
        let text = text_parts.join(" ");
        let wc = word_count(&text);
        let total: usize = wc.values().sum();
        assert_eq!(
            total, n_words,
            "sum of word counts should equal number of words for {:?}",
            text
        );
    }
}

/// Свойство (оракул): word_count согласуется с наивным ручным подсчётом.
#[test]
fn prop_word_count_agrees_with_naive_oracle() {
    let mut rng: u64 = 0xDEAD_C0DE_ABCD_0001;
    let words_pool = ["foo", "bar", "baz", "qux", "quux", "corge"];
    for _ in 0..700 {
        let n_words = in_range(&mut rng, 0, 25) as usize;
        let mut parts: Vec<&str> = Vec::new();
        for _ in 0..n_words {
            let idx = (next_u64(&mut rng) % words_pool.len() as u64) as usize;
            parts.push(words_pool[idx]);
        }
        // Вставляем случайное количество пробелов между словами.
        let text = parts.join("  ");
        let wc = word_count(&text);

        let mut oracle: HashMap<String, usize> = HashMap::new();
        for w in text.split_whitespace() {
            *oracle.entry(w.to_string()).or_insert(0) += 1;
        }
        assert_eq!(
            wc, oracle,
            "word_count disagrees with oracle for {:?}",
            text
        );
    }
}

/// Свойство: слово, которого нет в тексте, не появляется в карте.
#[test]
fn prop_word_count_absent_word_not_in_map() {
    let mut rng: u64 = 0xFEED_FACE_BEAD_1357;
    let words_pool = ["one", "two", "three", "four", "five"];
    for _ in 0..500 {
        let n_words = in_range(&mut rng, 1, 15) as usize;
        let mut parts: Vec<&str> = Vec::new();
        for _ in 0..n_words {
            let idx = (next_u64(&mut rng) % words_pool.len() as u64) as usize;
            parts.push(words_pool[idx]);
        }
        let text = parts.join(" ");
        let wc = word_count(&text);
        // "seven" никогда не попадает в пул слов.
        assert_eq!(
            wc.get("seven"),
            None,
            "absent word 'seven' should not be in word_count"
        );
    }
}

/// Свойство: пустая и пробельная строки дают пустую карту.
#[test]
fn prop_word_count_whitespace_only_is_empty() {
    let mut rng: u64 = 0x0000_0001_DEAD_BEEF;
    for _ in 0..300 {
        let spaces = in_range(&mut rng, 0, 30) as usize;
        let s: String = " ".repeat(spaces);
        assert!(
            word_count(&s).is_empty(),
            "whitespace-only string should give empty word_count"
        );
    }
}

/// Свойство: каждый счётчик >= 1 (нулевых записей нет).
#[test]
fn prop_word_count_no_zero_counts() {
    let mut rng: u64 = 0xAAAA_BBBB_CCCC_DDDD;
    let words_pool = ["rust", "is", "great"];
    for _ in 0..500 {
        let n_words = in_range(&mut rng, 0, 20) as usize;
        let mut parts: Vec<&str> = Vec::new();
        for _ in 0..n_words {
            let idx = (next_u64(&mut rng) % words_pool.len() as u64) as usize;
            parts.push(words_pool[idx]);
        }
        let text = parts.join(" ");
        let wc = word_count(&text);
        for (word, &count) in &wc {
            assert!(count > 0, "word {:?} has zero count", word);
        }
    }
}

// ============================================================
// join_words — property tests
// ============================================================

/// Свойство: результат содержит ровно (n-1) вхождений sep (при sep непустом и n>0).
#[test]
fn prop_join_words_separator_count() {
    let mut rng: u64 = 0x1357_2468_ABCD_EF01;
    let word_pool = ["a", "b", "c", "d", "e", "f"];
    let sep_pool = [", ", " -> ", "|", "::"];
    for _ in 0..700 {
        let n = in_range(&mut rng, 0, 10) as usize;
        let sep_idx = (next_u64(&mut rng) % sep_pool.len() as u64) as usize;
        let sep = sep_pool[sep_idx];

        let words: Vec<&str> = (0..n)
            .map(|_| {
                let idx = (next_u64(&mut rng) % word_pool.len() as u64) as usize;
                word_pool[idx]
            })
            .collect();

        let result = join_words(&words, sep);

        if n == 0 {
            assert_eq!(result, "", "empty slice should give empty string");
        } else {
            // Считаем вхождения sep в результат.
            let count = if sep.is_empty() {
                0
            } else {
                result.matches(sep).count()
            };
            // Для n слов должно быть ровно n-1 разделителей.
            // (Это точно при условии что слова сами не содержат sep — здесь это так.)
            assert_eq!(
                count,
                n - 1,
                "expected {} separators for {} words, got {} in {:?}",
                n - 1,
                n,
                count,
                result
            );
        }
    }
}

/// Свойство (round-trip/оракул): join_words совпадает с words.join(sep).
#[test]
fn prop_join_words_agrees_with_stdlib_join() {
    let mut rng: u64 = 0xFEDC_BA98_7654_3210;
    let word_pool = ["alpha", "beta", "gamma", "delta"];
    let sep_pool = ["", ", ", " | ", "---"];
    for _ in 0..700 {
        let n = in_range(&mut rng, 0, 12) as usize;
        let sep_idx = (next_u64(&mut rng) % sep_pool.len() as u64) as usize;
        let sep = sep_pool[sep_idx];

        let words: Vec<&str> = (0..n)
            .map(|_| {
                let idx = (next_u64(&mut rng) % word_pool.len() as u64) as usize;
                word_pool[idx]
            })
            .collect();

        let result = join_words(&words, sep);
        let oracle = words.join(sep);
        assert_eq!(
            result, oracle,
            "join_words({:?}, {:?}) = {:?} but oracle = {:?}",
            words, sep, result, oracle
        );
    }
}

/// Свойство: результат начинается с первого слова и заканчивается последним (при n>=1).
#[test]
fn prop_join_words_starts_and_ends_with_words() {
    let mut rng: u64 = 0x1234_ABCD_5678_EF90;
    let word_pool = ["hello", "world", "rust", "test"];
    for _ in 0..700 {
        let n = in_range(&mut rng, 1, 10) as usize;
        let words: Vec<&str> = (0..n)
            .map(|_| {
                let idx = (next_u64(&mut rng) % word_pool.len() as u64) as usize;
                word_pool[idx]
            })
            .collect();
        let sep = "::";
        let result = join_words(&words, sep);
        assert!(
            result.starts_with(words[0]),
            "result {:?} should start with {:?}",
            result,
            words[0]
        );
        assert!(
            result.ends_with(words[n - 1]),
            "result {:?} should end with {:?}",
            result,
            words[n - 1]
        );
    }
}

/// Свойство: пустой разделитель — конкатенация без зазоров.
#[test]
fn prop_join_words_empty_sep_is_concat() {
    let mut rng: u64 = 0xAAAA_1111_BBBB_2222;
    let word_pool = ["x", "yy", "zzz", "a", "bc"];
    for _ in 0..700 {
        let n = in_range(&mut rng, 0, 10) as usize;
        let words: Vec<&str> = (0..n)
            .map(|_| {
                let idx = (next_u64(&mut rng) % word_pool.len() as u64) as usize;
                word_pool[idx]
            })
            .collect();
        let result = join_words(&words, "");
        let expected: String = words.concat();
        assert_eq!(
            result, expected,
            "join with empty sep should be plain concatenation"
        );
    }
}

/// Свойство: длина результата корректна.
#[test]
fn prop_join_words_result_length() {
    let mut rng: u64 = 0x9876_5432_10FE_DCBA;
    let word_pool = ["ab", "cde", "fg", "h"];
    for _ in 0..700 {
        let n = in_range(&mut rng, 0, 12) as usize;
        let sep = "##";
        let words: Vec<&str> = (0..n)
            .map(|_| {
                let idx = (next_u64(&mut rng) % word_pool.len() as u64) as usize;
                word_pool[idx]
            })
            .collect();
        let result = join_words(&words, sep);
        let expected_len: usize = words.iter().map(|w| w.len()).sum::<usize>()
            + if n > 0 { (n - 1) * sep.len() } else { 0 };
        assert_eq!(
            result.len(),
            expected_len,
            "length mismatch for {:?} joined with {:?}",
            words,
            sep
        );
    }
}
