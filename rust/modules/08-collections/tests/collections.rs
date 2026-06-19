// Эти тесты трогать не нужно — это эталон поведения.
//
// Они подключают твой крейт и проверяют публичный API модуля 08. Пока тела функций в
// src/lib.rs — это todo!(), тесты компилируются, но падают в рантайме. Реализуй функции —
// и красное станет зелёным.

use m08_collections::*;
use std::collections::HashMap;

// ---------- char_freq ----------

#[test]
fn char_freq_basic() {
    let got = char_freq("aab");
    let mut expected = HashMap::new();
    expected.insert('a', 2);
    expected.insert('b', 1);
    assert_eq!(got, expected);
}

#[test]
fn char_freq_empty_is_empty() {
    let got = char_freq("");
    assert!(got.is_empty());
}

#[test]
fn char_freq_counts_spaces_and_repeats() {
    let got = char_freq("hello world");
    assert_eq!(got.get(&'l'), Some(&3)); // hello + world
    assert_eq!(got.get(&'o'), Some(&2));
    assert_eq!(got.get(&'h'), Some(&1));
    assert_eq!(got.get(&' '), Some(&1));
    // Символа, которого нет, в карте быть не должно.
    assert_eq!(got.get(&'z'), None);
}

#[test]
fn char_freq_counts_unicode_scalars_not_bytes() {
    // 'é' в UTF-8 занимает 2 байта, но это ОДИН char.
    let got = char_freq("é");
    assert_eq!(got.get(&'é'), Some(&1));
    assert_eq!(got.len(), 1);
}

// ---------- dedup_sorted ----------

#[test]
fn dedup_sorted_basic() {
    assert_eq!(dedup_sorted(vec![3, 1, 2, 3, 1]), vec![1, 2, 3]);
}

#[test]
fn dedup_sorted_empty() {
    assert_eq!(dedup_sorted(vec![]), Vec::<i64>::new());
}

#[test]
fn dedup_sorted_already_unique_and_sorted() {
    assert_eq!(dedup_sorted(vec![1, 2, 3]), vec![1, 2, 3]);
}

#[test]
fn dedup_sorted_handles_negatives_and_all_equal() {
    assert_eq!(dedup_sorted(vec![5, 5, 5]), vec![5]);
    assert_eq!(dedup_sorted(vec![0, -2, -2, 3, 0]), vec![-2, 0, 3]);
}

// ---------- word_count ----------

#[test]
fn word_count_basic() {
    let got = word_count("a b a");
    let mut expected = HashMap::new();
    expected.insert("a".to_string(), 2);
    expected.insert("b".to_string(), 1);
    assert_eq!(got, expected);
}

#[test]
fn word_count_collapses_extra_whitespace() {
    // Несколько пробелов, табы и переводы строк — всё это разделители, пустых слов нет.
    let got = word_count("  the  cat \t the\n cat  ");
    assert_eq!(got.get("the"), Some(&2));
    assert_eq!(got.get("cat"), Some(&2));
    assert_eq!(got.len(), 2);
}

#[test]
fn word_count_is_case_sensitive() {
    let got = word_count("Cat cat");
    assert_eq!(got.get("Cat"), Some(&1));
    assert_eq!(got.get("cat"), Some(&1));
    assert_eq!(got.len(), 2);
}

#[test]
fn word_count_empty_text_is_empty() {
    assert!(word_count("").is_empty());
    assert!(word_count("    \t\n  ").is_empty());
}

// ---------- join_words ----------

#[test]
fn join_words_basic() {
    assert_eq!(join_words(&["a", "b", "c"], ", "), "a, b, c");
}

#[test]
fn join_words_single_element_has_no_separator() {
    assert_eq!(join_words(&["solo"], ", "), "solo");
}

#[test]
fn join_words_empty_slice_is_empty_string() {
    assert_eq!(join_words(&[], ", "), "");
}

#[test]
fn join_words_empty_separator() {
    assert_eq!(join_words(&["a", "b", "c"], ""), "abc");
}

#[test]
fn join_words_multichar_separator() {
    assert_eq!(join_words(&["1", "2", "3"], " -> "), "1 -> 2 -> 3");
}
