// Тесты для функции top_n_words из крейта m08_collections.
// Структура файла: сначала детерминированные примеры, затем рандомизированные
// свойства на основе детерминированного xorshift64* ГПСЧ (без внешних зависимостей).

use m08_collections::top_n_words;

// ============================================================
// Детерминированный ГПСЧ (xorshift64*) — тот же алгоритм, что в props.rs
// ============================================================

fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}

fn in_range(state: &mut u64, lo: usize, hi: usize) -> usize {
    let span = (hi - lo + 1) as u64;
    lo + (next_u64(state) % span) as usize
}

// ============================================================
// Наивный оракул — независимая реализация top_n_words для сравнения
// ============================================================

fn naive_top_n(text: &str, n: usize) -> Vec<(String, usize)> {
    use std::collections::HashMap;
    if n == 0 {
        return Vec::new();
    }
    let mut freq: HashMap<String, usize> = HashMap::new();
    for word in text.split_whitespace() {
        *freq.entry(word.to_string()).or_insert(0) += 1;
    }
    let mut pairs: Vec<(String, usize)> = freq.into_iter().collect();
    pairs.sort_by(|a, b| b.1.cmp(&a.1).then_with(|| a.0.cmp(&b.0)));
    pairs.truncate(n);
    pairs
}

// ============================================================
// Детерминированные тесты с конкретным входом/выходом
// ============================================================

#[test]
fn basic_top2_correct_order_and_counts() {
    // "a" встречается 3 раза, "b" — 2, "c" — 1.
    let result = top_n_words("a b a c b a", 2);
    assert_eq!(
        result,
        vec![("a".to_string(), 3), ("b".to_string(), 2)],
        "top 2 should be a(3), b(2)"
    );
}

#[test]
fn top1_returns_only_most_frequent() {
    let result = top_n_words("dog cat dog bird dog cat", 1);
    assert_eq!(result, vec![("dog".to_string(), 3)]);
}

#[test]
fn top_more_than_unique_words_returns_all() {
    // В тексте только 2 уникальных слова, но n=10.
    let result = top_n_words("yes no yes yes", 10);
    // Должны получить оба слова — не больше.
    assert_eq!(result.len(), 2);
    assert_eq!(result[0], ("yes".to_string(), 3));
    assert_eq!(result[1], ("no".to_string(), 1));
}

#[test]
fn empty_text_returns_empty() {
    assert!(top_n_words("", 5).is_empty());
}

#[test]
fn whitespace_only_text_returns_empty() {
    assert!(top_n_words("   \t\n  ", 5).is_empty());
}

#[test]
fn n_zero_always_returns_empty() {
    assert!(top_n_words("hello world hello", 0).is_empty());
    assert!(top_n_words("", 0).is_empty());
}

#[test]
fn tie_breaking_is_alphabetical() {
    // "apple", "banana", "cherry" — у каждого по 1 разу.
    // При ничьей по частоте — алфавитный порядок: apple < banana < cherry.
    let result = top_n_words("cherry banana apple", 3);
    assert_eq!(
        result,
        vec![
            ("apple".to_string(), 1),
            ("banana".to_string(), 1),
            ("cherry".to_string(), 1),
        ],
        "ties should be broken alphabetically"
    );
}

#[test]
fn tie_breaking_selects_lexicographically_smallest_for_top1() {
    // "aaa" и "bbb" по 2 раза — при n=1 должно прийти "aaa" (алфавит).
    let result = top_n_words("bbb aaa bbb aaa", 1);
    assert_eq!(result, vec![("aaa".to_string(), 2)]);
}

#[test]
fn case_sensitive_different_words() {
    // "Cat" и "cat" — разные слова; "Cat" встречается 2 раза, "cat" — 1.
    let result = top_n_words("Cat cat Cat", 2);
    assert_eq!(result[0], ("Cat".to_string(), 2));
    assert_eq!(result[1], ("cat".to_string(), 1));
}

#[test]
fn single_word_single_occurrence() {
    let result = top_n_words("hello", 1);
    assert_eq!(result, vec![("hello".to_string(), 1)]);
}

#[test]
fn single_word_repeated_many_times() {
    let result = top_n_words("x x x x x", 3);
    assert_eq!(result, vec![("x".to_string(), 5)]);
}

#[test]
fn extra_whitespace_collapsed() {
    // Лишние пробелы не создают пустых слов.
    let result = top_n_words("  the  cat \t the\n cat  ", 2);
    assert_eq!(result.len(), 2);
    assert_eq!(result[0], ("cat".to_string(), 2));
    assert_eq!(result[1], ("the".to_string(), 2));
}

#[test]
fn length_is_capped_at_n() {
    // 5 уникальных слов, запрашиваем top 3.
    let result = top_n_words("e d c b a", 3);
    assert_eq!(result.len(), 3);
}

#[test]
fn all_unique_words_length_respects_n() {
    let text = "one two three four five six seven eight nine ten";
    for n in 0..=10 {
        let result = top_n_words(text, n);
        assert_eq!(
            result.len(),
            n,
            "expected {} results for n={}, got {}",
            n,
            n,
            result.len()
        );
    }
}

// ============================================================
// Рандомизированные свойства
// ============================================================

/// Свойство: длина результата <= min(n, число уникальных слов).
#[test]
fn prop_len_bounded_by_n_and_unique_count() {
    let mut rng: u64 = 0xDEAD_BEEF_0807_C001;
    let words_pool = ["alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta"];
    for _ in 0..800 {
        let n_words = in_range(&mut rng, 0, 25);
        let n = in_range(&mut rng, 0, 10);
        let mut parts = Vec::new();
        for _ in 0..n_words {
            let idx = in_range(&mut rng, 0, words_pool.len() - 1);
            parts.push(words_pool[idx]);
        }
        let text = parts.join(" ");
        let unique_count = {
            let mut set: std::collections::HashSet<&str> =
                std::collections::HashSet::new();
            for w in text.split_whitespace() {
                set.insert(w);
            }
            set.len()
        };
        let result = top_n_words(&text, n);
        assert!(
            result.len() <= n,
            "result.len()={} > n={} for text={:?}",
            result.len(),
            n,
            text
        );
        assert!(
            result.len() <= unique_count,
            "result.len()={} > unique_count={} for text={:?}",
            result.len(),
            unique_count,
            text
        );
    }
}

/// Свойство: результат отсортирован по убыванию частоты (с учётом тай-брейкинга).
#[test]
fn prop_result_sorted_desc_freq_asc_alpha() {
    let mut rng: u64 = 0xC0FE_BABE_1234_5678;
    let words_pool = ["one", "two", "three", "four", "five"];
    for _ in 0..700 {
        let n_words = in_range(&mut rng, 0, 30);
        let n = in_range(&mut rng, 1, 8);
        let mut parts = Vec::new();
        for _ in 0..n_words {
            let idx = in_range(&mut rng, 0, words_pool.len() - 1);
            parts.push(words_pool[idx]);
        }
        let text = parts.join(" ");
        let result = top_n_words(&text, n);

        for window in result.windows(2) {
            let (w0, c0) = &window[0];
            let (w1, c1) = &window[1];
            // c0 >= c1 должно выполняться.
            assert!(
                c0 >= c1,
                "counts not desc: {:?}({}) before {:?}({}) in text={:?}",
                w0, c0, w1, c1, text
            );
            // При равных счётчиках — слово должно идти в алфавитном порядке.
            if c0 == c1 {
                assert!(
                    w0 <= w1,
                    "tie not broken alphabetically: {:?} before {:?} for text={:?}",
                    w0, w1, text
                );
            }
        }
    }
}

/// Свойство: каждое слово в результате действительно встречалось в тексте.
#[test]
fn prop_all_result_words_exist_in_text() {
    let mut rng: u64 = 0xFEED_F00D_ABCD_EF01;
    let words_pool = ["rust", "vec", "map", "set", "str", "box", "arc"];
    for _ in 0..700 {
        let n_words = in_range(&mut rng, 0, 25);
        let n = in_range(&mut rng, 0, 7);
        let mut parts = Vec::new();
        for _ in 0..n_words {
            let idx = in_range(&mut rng, 0, words_pool.len() - 1);
            parts.push(words_pool[idx]);
        }
        let text = parts.join(" ");
        let result = top_n_words(&text, n);
        for (word, _count) in &result {
            assert!(
                text.split_whitespace().any(|w| w == word),
                "word {:?} in result but not found in text={:?}",
                word,
                text
            );
        }
    }
}

/// Свойство: счётчик в результате совпадает с реальным числом вхождений.
#[test]
fn prop_counts_match_actual_occurrences() {
    let mut rng: u64 = 0x1111_2222_3333_4444;
    let words_pool = ["foo", "bar", "baz", "qux"];
    for _ in 0..700 {
        let n_words = in_range(&mut rng, 0, 20);
        let n = in_range(&mut rng, 1, 5);
        let mut parts = Vec::new();
        for _ in 0..n_words {
            let idx = in_range(&mut rng, 0, words_pool.len() - 1);
            parts.push(words_pool[idx]);
        }
        let text = parts.join(" ");
        let result = top_n_words(&text, n);
        for (word, count) in &result {
            let actual = text.split_whitespace().filter(|&w| w == word).count();
            assert_eq!(
                *count, actual,
                "count for {:?} is {} but actual occurrences in {:?} = {}",
                word, count, text, actual
            );
        }
    }
}

/// Свойство: нет дубликатов слов в результате.
#[test]
fn prop_no_duplicate_words_in_result() {
    let mut rng: u64 = 0xAAAA_BBBB_CCCC_DDDD;
    let words_pool = ["cat", "dog", "bird", "fish", "ant"];
    for _ in 0..700 {
        let n_words = in_range(&mut rng, 0, 25);
        let n = in_range(&mut rng, 0, 8);
        let mut parts = Vec::new();
        for _ in 0..n_words {
            let idx = in_range(&mut rng, 0, words_pool.len() - 1);
            parts.push(words_pool[idx]);
        }
        let text = parts.join(" ");
        let result = top_n_words(&text, n);
        let mut seen: std::collections::HashSet<&str> = std::collections::HashSet::new();
        for (word, _) in &result {
            assert!(
                seen.insert(word.as_str()),
                "duplicate word {:?} in top_n_words result for text={:?}",
                word,
                text
            );
        }
    }
}

/// Свойство (оракул): top_n_words согласуется с наивной независимой реализацией.
#[test]
fn prop_agrees_with_naive_oracle() {
    let mut rng: u64 = 0x9876_5432_FEDC_BA10;
    let words_pool = ["alpha", "beta", "gamma", "delta", "epsilon"];
    for _ in 0..600 {
        let n_words = in_range(&mut rng, 0, 30);
        let n = in_range(&mut rng, 0, 8);
        let mut parts = Vec::new();
        for _ in 0..n_words {
            let idx = in_range(&mut rng, 0, words_pool.len() - 1);
            parts.push(words_pool[idx]);
        }
        let text = parts.join(" ");
        let result = top_n_words(&text, n);
        let expected = naive_top_n(&text, n);
        assert_eq!(
            result, expected,
            "top_n_words disagrees with oracle for text={:?}, n={}",
            text, n
        );
    }
}

/// Свойство: n=0 всегда возвращает пустой вектор.
#[test]
fn prop_n_zero_always_empty() {
    let mut rng: u64 = 0xBEEF_CAFE_0000_0001;
    let words_pool = ["x", "y", "z"];
    for _ in 0..500 {
        let n_words = in_range(&mut rng, 0, 20);
        let mut parts = Vec::new();
        for _ in 0..n_words {
            let idx = in_range(&mut rng, 0, words_pool.len() - 1);
            parts.push(words_pool[idx]);
        }
        let text = parts.join(" ");
        let result = top_n_words(&text, 0);
        assert!(
            result.is_empty(),
            "n=0 should always return empty, got {:?} for text={:?}",
            result,
            text
        );
    }
}

/// Свойство: если n >= число уникальных слов, возвращаются ВСЕ уникальные слова.
#[test]
fn prop_large_n_returns_all_unique_words() {
    let mut rng: u64 = 0xDEAD_C0DE_1357_2468;
    let words_pool = ["a", "b", "c", "d"];
    for _ in 0..500 {
        let n_words = in_range(&mut rng, 0, 20);
        let mut parts = Vec::new();
        for _ in 0..n_words {
            let idx = in_range(&mut rng, 0, words_pool.len() - 1);
            parts.push(words_pool[idx]);
        }
        let text = parts.join(" ");
        let unique_count = {
            let mut set = std::collections::HashSet::new();
            for w in text.split_whitespace() {
                set.insert(w);
            }
            set.len()
        };
        // Запрашиваем заведомо больше, чем уникальных слов.
        let n = unique_count + 10;
        let result = top_n_words(&text, n);
        assert_eq!(
            result.len(),
            unique_count,
            "with n>=unique_count, result.len() should equal unique_count={} for text={:?}",
            unique_count,
            text
        );
    }
}
