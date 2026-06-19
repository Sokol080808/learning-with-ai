// Интеграционные тесты для заданий count_words / most_frequent (модуль 04).
// Эти тесты проверяют ПУБЛИЧНЫЙ API крейта m04_borrowing и не зависят от
// конкретной реализации — только от сигнатур и контракта.
//
// Файл содержит:
//   (a) детерминированные тесты с конкретными входами и ожидаемыми выходами;
//   (b) рандомизированные property-тесты (детерминированный xorshift64*,
//       фиксированный сид, ~500–1000 итераций).

use m04_borrowing::{count_words, most_frequent};
use std::collections::HashMap;

// ─── детерминированный ГПСЧ (xorshift64*) ───────────────────────────────────
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

// ─── вспомогательная функция: собрать HashMap из текста ─────────────────────
fn build_counts(text: &str) -> HashMap<&str, usize> {
    let mut map = HashMap::new();
    count_words(text, &mut map);
    map
}

// ════════════════════════════════════════════════════════════════════════════
// (a) ДЕТЕРМИНИРОВАННЫЕ ТЕСТЫ
// ════════════════════════════════════════════════════════════════════════════

// ─── count_words: базовые случаи ────────────────────────────────────────────

#[test]
fn count_words_single_word() {
    let text = "hello";
    let counts = build_counts(text);
    assert_eq!(counts.len(), 1);
    assert_eq!(counts["hello"], 1);
}

#[test]
fn count_words_two_different_words() {
    let text = "foo bar";
    let counts = build_counts(text);
    assert_eq!(counts.get("foo"), Some(&1));
    assert_eq!(counts.get("bar"), Some(&1));
    assert_eq!(counts.len(), 2);
}

#[test]
fn count_words_repeated_word() {
    let text = "yes yes yes";
    let counts = build_counts(text);
    assert_eq!(counts["yes"], 3);
    assert_eq!(counts.len(), 1);
}

#[test]
fn count_words_mixed_frequencies() {
    // "the" × 3, "quick" × 1, "brown" × 1, "fox" × 2
    let text = "the quick brown fox the fox the";
    let counts = build_counts(text);
    assert_eq!(counts["the"], 3);
    assert_eq!(counts["quick"], 1);
    assert_eq!(counts["brown"], 1);
    assert_eq!(counts["fox"], 2);
    assert_eq!(counts.len(), 4);
}

#[test]
fn count_words_empty_string_yields_no_entries() {
    let text = "";
    let counts = build_counts(text);
    assert!(counts.is_empty(), "пустая строка не должна создавать записей");
}

#[test]
fn count_words_only_whitespace_yields_no_entries() {
    // split_whitespace игнорирует любые пробельные символы
    let text = "   \t  \n  ";
    let counts = build_counts(text);
    assert!(counts.is_empty());
}

#[test]
fn count_words_accumulates_into_existing_map() {
    // Функция ДОБАВЛЯЕТ в переданный map, а не обнуляет его.
    let text1 = "apple banana";
    let text2 = "banana cherry";
    let mut counts = HashMap::new();
    count_words(text1, &mut counts);
    count_words(text2, &mut counts);
    assert_eq!(counts["apple"], 1);
    assert_eq!(counts["banana"], 2); // встретилось в обоих текстах
    assert_eq!(counts["cherry"], 1);
}

#[test]
fn count_words_keys_are_slices_of_text() {
    // Ключи должны быть &str — срезы внутрь исходного text.
    // Проверяем: key, возвращённый из map, совпадает по значению с соответствующим
    // фрагментом text.
    let text = "rust borrow rust";
    let counts = build_counts(text);
    // Убедимся, что ключ "rust" действительно есть в таблице
    assert!(counts.contains_key("rust"));
    assert_eq!(counts["rust"], 2);
}

#[test]
fn count_words_single_character_words() {
    let text = "a b a c a b";
    let counts = build_counts(text);
    assert_eq!(counts["a"], 3);
    assert_eq!(counts["b"], 2);
    assert_eq!(counts["c"], 1);
}

// ─── most_frequent: базовые случаи ──────────────────────────────────────────

#[test]
fn most_frequent_empty_map_is_none() {
    let counts: HashMap<&str, usize> = HashMap::new();
    assert_eq!(most_frequent(&counts), None);
}

#[test]
fn most_frequent_single_entry() {
    let text = "solo";
    let counts = build_counts(text);
    let result = most_frequent(&counts);
    assert_eq!(result, Some(("solo", 1)));
}

#[test]
fn most_frequent_picks_highest() {
    let text = "the quick brown fox the fox the";
    let counts = build_counts(text);
    let (word, freq) = most_frequent(&counts).expect("не должно быть None");
    assert_eq!(word, "the");
    assert_eq!(freq, 3);
}

#[test]
fn most_frequent_freq_is_correct_count() {
    // Проверяем, что возвращаемый счётчик совпадает с тем, что реально в map.
    let text = "go go go stop go stop";
    let counts = build_counts(text);
    let (word, freq) = most_frequent(&counts).unwrap();
    // Победитель — "go" с freq 4
    assert_eq!(word, "go");
    assert_eq!(freq, 4);
    assert_eq!(counts[word], freq);
}

#[test]
fn most_frequent_two_words_unequal() {
    let text = "x x x y y";
    let counts = build_counts(text);
    let (word, freq) = most_frequent(&counts).unwrap();
    assert_eq!(word, "x");
    assert_eq!(freq, 3);
}

#[test]
fn most_frequent_does_not_consume_map() {
    // most_frequent принимает &map, а не map — после вызова map по-прежнему доступен.
    let text = "alpha beta alpha";
    let counts = build_counts(text);
    let _ = most_frequent(&counts);
    // Мы всё ещё можем использовать counts:
    assert_eq!(counts["alpha"], 2);
    assert_eq!(counts.len(), 2);
}

// ─── count_words + most_frequent вместе ─────────────────────────────────────

#[test]
fn round_trip_count_then_most_frequent() {
    let text = "one two two three three three";
    let counts = build_counts(text);
    let (word, freq) = most_frequent(&counts).unwrap();
    assert_eq!(word, "three");
    assert_eq!(freq, 3);
}

#[test]
fn round_trip_empty_text() {
    let text = "";
    let counts = build_counts(text);
    assert_eq!(most_frequent(&counts), None);
}

// ════════════════════════════════════════════════════════════════════════════
// (b) РАНДОМИЗИРОВАННЫЕ PROPERTY-ТЕСТЫ
// ════════════════════════════════════════════════════════════════════════════

// Словарь для генерации случайных слов — фиксированный, маленький, чтобы
// слова гарантированно повторялись и частоты были нетривиальными.
const WORDS: &[&str] = &[
    "alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta", "theta",
];

/// Собрать текст из случайных слов словаря.
fn random_text<'a>(rng: &mut u64, n_words: usize) -> String {
    let parts: Vec<&str> = (0..n_words)
        .map(|_| WORDS[in_range(rng, 0, WORDS.len() - 1)])
        .collect();
    parts.join(" ")
}

/// Наивный оракул: подсчитать частоты простым циклом.
fn naive_counts(text: &str) -> HashMap<String, usize> {
    let mut map = HashMap::new();
    for word in text.split_whitespace() {
        *map.entry(word.to_string()).or_insert(0usize) += 1;
    }
    map
}

/// count_words совпадает с наивным оракулом для случайных текстов.
#[test]
fn prop_count_words_matches_naive_oracle() {
    let mut rng: u64 = 0xDEAD_BEEF_BABE_1234;
    for _ in 0..700 {
        let n = in_range(&mut rng, 0, 60);
        let text = random_text(&mut rng, n);
        let counts = build_counts(&text);
        let oracle = naive_counts(&text);

        // Одинаковое количество уникальных слов
        assert_eq!(
            counts.len(),
            oracle.len(),
            "разные размеры таблиц для '{}'",
            text
        );
        // Каждый ключ из оракула должен быть в counts с тем же значением
        for (word, &expected) in &oracle {
            let got = counts.get(word.as_str()).copied().unwrap_or(0);
            assert_eq!(
                got, expected,
                "слово '{}': ожидалось {}, получено {}",
                word, expected, got
            );
        }
    }
}

/// Сумма всех частот равна общему числу слов.
#[test]
fn prop_count_words_total_equals_word_count() {
    let mut rng: u64 = 0xCAFE_BABE_1111_2222;
    for _ in 0..700 {
        let n = in_range(&mut rng, 0, 80);
        let text = random_text(&mut rng, n);
        let counts = build_counts(&text);
        let total: usize = counts.values().sum();
        assert_eq!(
            total, n,
            "сумма частот {} != количество слов {} для '{}'",
            total, n, text
        );
    }
}

/// Все ключи в таблице — слова, реально присутствующие в тексте.
#[test]
fn prop_count_words_keys_are_substrings_of_text() {
    let mut rng: u64 = 0x1357_ABCD_2468_EF01;
    for _ in 0..600 {
        let n = in_range(&mut rng, 1, 50);
        let text = random_text(&mut rng, n);
        let counts = build_counts(&text);
        for key in counts.keys() {
            assert!(
                text.split_whitespace().any(|w| w == *key),
                "ключ '{}' не является словом из текста '{}'",
                key,
                text
            );
        }
    }
}

/// Все частоты строго положительны.
#[test]
fn prop_count_words_all_counts_positive() {
    let mut rng: u64 = 0xFEED_FACE_DEAD_C0DE;
    for _ in 0..600 {
        let n = in_range(&mut rng, 1, 60);
        let text = random_text(&mut rng, n);
        let counts = build_counts(&text);
        for (word, &count) in &counts {
            assert!(
                count > 0,
                "нулевой счётчик для слова '{}' в тексте '{}'",
                word,
                text
            );
        }
    }
}

/// most_frequent возвращает None тогда и только тогда, когда таблица пуста.
#[test]
fn prop_most_frequent_none_iff_empty() {
    // Пустая таблица → None
    let empty: HashMap<&str, usize> = HashMap::new();
    assert_eq!(most_frequent(&empty), None);

    let mut rng: u64 = 0xAAAA_1234_5678_BBBB;
    for _ in 0..700 {
        let n = in_range(&mut rng, 1, 60);
        let text = random_text(&mut rng, n);
        let counts = build_counts(&text);
        assert!(
            most_frequent(&counts).is_some(),
            "most_frequent вернул None для непустой таблицы"
        );
    }
}

/// Частота, возвращённая most_frequent, — максимальная в таблице.
#[test]
fn prop_most_frequent_value_is_max() {
    let mut rng: u64 = 0x9876_5432_FEDC_BA01;
    for _ in 0..700 {
        let n = in_range(&mut rng, 1, 60);
        let text = random_text(&mut rng, n);
        let counts = build_counts(&text);
        let (_, freq) = most_frequent(&counts).unwrap();

        let real_max = *counts.values().max().unwrap();
        assert_eq!(
            freq, real_max,
            "most_frequent вернул частоту {} < реального максимума {} для '{}'",
            freq, real_max, text
        );
    }
}

/// Слово, возвращённое most_frequent, реально есть в таблице с указанной частотой.
#[test]
fn prop_most_frequent_word_present_in_map() {
    let mut rng: u64 = 0x1111_EEEE_2222_FFFF;
    for _ in 0..700 {
        let n = in_range(&mut rng, 1, 60);
        let text = random_text(&mut rng, n);
        let counts = build_counts(&text);
        let (word, freq) = most_frequent(&counts).unwrap();

        assert_eq!(
            counts.get(word),
            Some(&freq),
            "слово '{}' с частотой {} не совпадает с таблицей",
            word,
            freq
        );
    }
}

/// Частота победителя >= частоты каждого другого слова.
#[test]
fn prop_most_frequent_is_upper_bound_of_all_counts() {
    let mut rng: u64 = 0xBEEF_DEAD_CAFE_5678;
    for _ in 0..700 {
        let n = in_range(&mut rng, 1, 60);
        let text = random_text(&mut rng, n);
        let counts = build_counts(&text);
        let (_, freq) = most_frequent(&counts).unwrap();

        for (w, &c) in &counts {
            assert!(
                freq >= c,
                "most_frequent freq {} < freq {} for word '{}' in '{}'",
                freq,
                c,
                w,
                text
            );
        }
    }
}
