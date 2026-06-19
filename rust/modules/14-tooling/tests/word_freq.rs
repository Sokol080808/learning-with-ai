// Тесты для word_frequency из m14_tooling.
// Два слоя: детерминированные примеры (exact assert_eq!) и property-тесты
// на базе xorshift64* ГПСЧ (без внешних крейтов, воспроизводимо).

use m14_tooling::word_frequency;
use std::collections::HashMap;

// ---------------------------------------------------------------------------
// Детерминированные тесты — эталонные входы / выходы
// ---------------------------------------------------------------------------

#[test]
fn wf_empty_string_returns_empty() {
    assert_eq!(word_frequency(""), vec![]);
}

#[test]
fn wf_only_whitespace_returns_empty() {
    assert_eq!(word_frequency("   \t\n  "), vec![]);
}

#[test]
fn wf_single_word() {
    assert_eq!(
        word_frequency("rust"),
        vec![("rust".to_string(), 1)]
    );
}

#[test]
fn wf_single_word_uppercase_lowercased() {
    // «Rust» должна учитываться как «rust»
    assert_eq!(
        word_frequency("Rust"),
        vec![("rust".to_string(), 1)]
    );
}

#[test]
fn wf_two_different_words_sorted_desc_by_count() {
    // "b" появляется дважды, "a" — один раз → b идёт первым
    let result = word_frequency("b a b");
    assert_eq!(
        result,
        vec![("b".to_string(), 2), ("a".to_string(), 1)]
    );
}

#[test]
fn wf_basic_example_hello_world() {
    // Из README: «Hello hello world» → [("hello", 2), ("world", 1)]
    let result = word_frequency("Hello hello world");
    assert_eq!(
        result,
        vec![("hello".to_string(), 2), ("world".to_string(), 1)]
    );
}

#[test]
fn wf_ties_broken_alphabetically() {
    // "apple", "banana", "cherry" — по одному разу; должны идти в алфавитном порядке
    let result = word_frequency("cherry banana apple");
    assert_eq!(
        result,
        vec![
            ("apple".to_string(), 1),
            ("banana".to_string(), 1),
            ("cherry".to_string(), 1),
        ]
    );
}

#[test]
fn wf_mixed_case_counted_together() {
    // «THE», «The», «the» — три вхождения одного слова
    let result = word_frequency("THE The the");
    assert_eq!(result, vec![("the".to_string(), 3)]);
}

#[test]
fn wf_multiple_spaces_between_words() {
    // split_whitespace игнорирует лишние пробелы
    let result = word_frequency("a   a    b");
    assert_eq!(
        result,
        vec![("a".to_string(), 2), ("b".to_string(), 1)]
    );
}

#[test]
fn wf_all_same_word_returns_one_entry() {
    let result = word_frequency("yes yes yes yes");
    assert_eq!(result, vec![("yes".to_string(), 4)]);
}

#[test]
fn wf_sort_desc_then_alpha_complex() {
    // "b b b a a c" → b:3, a:2, c:1
    let result = word_frequency("b b b a a c");
    assert_eq!(
        result,
        vec![
            ("b".to_string(), 3),
            ("a".to_string(), 2),
            ("c".to_string(), 1),
        ]
    );
}

#[test]
fn wf_alpha_tiebreak_reversed_input() {
    // Все три слова по 2 раза → алфавитный порядок: cat, dog, fox
    let result = word_frequency("fox dog cat fox dog cat");
    assert_eq!(
        result,
        vec![
            ("cat".to_string(), 2),
            ("dog".to_string(), 2),
            ("fox".to_string(), 2),
        ]
    );
}

#[test]
fn wf_result_is_owned_strings() {
    // Проверяем, что возвращается Vec<(String, usize)> — мы владеем данными
    let mut result = word_frequency("hello world");
    result[0].0.push('!'); // мутируем String — это должно компилироваться
    // (просто убедились, что тип — String, а не &str)
}

// ---------------------------------------------------------------------------
// ГПСЧ (xorshift64*) — для property-тестов
// ---------------------------------------------------------------------------

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

/// Генерирует случайное «слово» из строчных букв a–e нужной длины.
fn rand_word(state: &mut u64, max_len: usize) -> String {
    let len = in_range(state, 1, max_len);
    (0..len)
        .map(|_| {
            let idx = (next_u64(state) % 5) as u8;
            (b'a' + idx) as char
        })
        .collect()
}

/// Собирает текст из случайных слов через пробел.
fn rand_text(state: &mut u64, word_count: usize, word_max_len: usize) -> String {
    (0..word_count)
        .map(|_| rand_word(state, word_max_len))
        .collect::<Vec<_>>()
        .join(" ")
}

// ---------------------------------------------------------------------------
// Property-тесты
// ---------------------------------------------------------------------------

/// Property: сумма всех count == общее число токенов в тексте.
#[test]
fn prop_sum_of_counts_equals_token_count() {
    let mut rng: u64 = 0xABCD_1234_EF56_7890;
    for _ in 0..800 {
        let n_words = in_range(&mut rng, 0, 30);
        let text = rand_text(&mut rng, n_words, 4);
        let token_count = text.split_whitespace().count();
        let result = word_frequency(&text);
        let total: usize = result.iter().map(|(_, c)| c).sum();
        assert_eq!(
            total, token_count,
            "sum of counts {total} != token count {token_count}, text={text:?}"
        );
    }
}

/// Property: нет дублирующихся слов в выводе (каждое слово встречается не более одного раза).
#[test]
fn prop_no_duplicate_words_in_output() {
    let mut rng: u64 = 0x1111_2222_AAAA_BBBB;
    for _ in 0..800 {
        let n_words = in_range(&mut rng, 0, 25);
        let text = rand_text(&mut rng, n_words, 3);
        let result = word_frequency(&text);
        let unique_words: std::collections::HashSet<&str> =
            result.iter().map(|(w, _)| w.as_str()).collect();
        assert_eq!(
            unique_words.len(),
            result.len(),
            "duplicate words in output for text={text:?}"
        );
    }
}

/// Property: каждое слово в выводе действительно встречается ровно столько раз,
/// сколько заявлено (сверка с наивным счётчиком).
#[test]
fn prop_counts_match_naive_oracle() {
    let mut rng: u64 = 0xDEAD_BEEF_CAFE_1234;
    for _ in 0..500 {
        let n_words = in_range(&mut rng, 1, 25);
        let text = rand_text(&mut rng, n_words, 4);
        let result = word_frequency(&text);
        // Наивный оракул: HashMap вручную
        let mut oracle: HashMap<String, usize> = HashMap::new();
        for token in text.split_whitespace() {
            *oracle.entry(token.to_lowercase()).or_insert(0) += 1;
        }
        for (word, count) in &result {
            let expected = oracle.get(word).copied().unwrap_or(0);
            assert_eq!(
                *count, expected,
                "count mismatch for word {word:?}: got {count}, oracle says {expected}, text={text:?}"
            );
        }
        // Также: количество уникальных слов совпадает
        assert_eq!(result.len(), oracle.len());
    }
}

/// Property: порядок — убывание count; при равном count слова лексикографически
/// упорядочены по возрастанию.
#[test]
fn prop_output_is_correctly_sorted() {
    let mut rng: u64 = 0x9988_7766_5544_3322;
    for _ in 0..800 {
        let n_words = in_range(&mut rng, 0, 30);
        let text = rand_text(&mut rng, n_words, 3);
        let result = word_frequency(&text);
        for window in result.windows(2) {
            let (word_a, cnt_a) = &window[0];
            let (word_b, cnt_b) = &window[1];
            // cnt_a >= cnt_b
            assert!(
                cnt_a >= cnt_b,
                "counts not non-increasing: {cnt_a} then {cnt_b} (words {word_a:?},{word_b:?}), text={text:?}"
            );
            // если равны — слово A ≤ слово B
            if cnt_a == cnt_b {
                assert!(
                    word_a <= word_b,
                    "ties not alphabetical: {word_a:?} before {word_b:?}, text={text:?}"
                );
            }
        }
    }
}

/// Property: word_frequency пуста тогда и только тогда, когда в тексте нет токенов.
#[test]
fn prop_empty_iff_no_tokens() {
    let mut rng: u64 = 0xF0F0_F0F0_0F0F_0F0F;
    // Несколько случаев с реальными словами
    for _ in 0..500 {
        let n_words = in_range(&mut rng, 0, 20);
        let text = rand_text(&mut rng, n_words, 3);
        let result = word_frequency(&text);
        let has_tokens = text.split_whitespace().next().is_some();
        assert_eq!(
            !result.is_empty(),
            has_tokens,
            "emptiness mismatch for text={text:?}"
        );
    }
}

/// Property: каждое слово в выводе — строчные буквы (lowercase).
#[test]
fn prop_all_output_words_are_lowercase() {
    // Генерируем текст с заглавными буквами
    let mut rng: u64 = 0xC0DE_BABE_1234_5678;
    let mixed_words = ["Hello", "WORLD", "Rust", "ABC", "Test", "foo", "BAR"];
    for _ in 0..500 {
        let n = in_range(&mut rng, 1, 10);
        let tokens: Vec<&str> = (0..n)
            .map(|_| mixed_words[in_range(&mut rng, 0, mixed_words.len() - 1)])
            .collect();
        let text = tokens.join(" ");
        let result = word_frequency(&text);
        for (word, _) in &result {
            assert_eq!(
                word.as_str(),
                word.to_lowercase(),
                "output word {word:?} is not lowercase, text={text:?}"
            );
        }
    }
}
