// Эти тесты трогать не нужно — это эталон поведения.
// Они описывают, как ДОЛЖНЫ работать функции из src/lib.rs. Сейчас они падают
// (внутри тел стоит todo!(), который паникует). Твоя задача — реализовать функции так,
// чтобы все тесты стали зелёными.

use m03_ownership::*;

#[test]
fn sum_vec_basic() {
    assert_eq!(sum_vec(vec![1, 2, 3, 4]), 10);
}

#[test]
fn sum_vec_empty_is_zero() {
    assert_eq!(sum_vec(vec![]), 0);
}

#[test]
fn sum_vec_with_negatives() {
    assert_eq!(sum_vec(vec![-5, 5, -3, 3]), 0);
    assert_eq!(sum_vec(vec![-10, 4]), -6);
}

#[test]
fn sum_vec_single() {
    assert_eq!(sum_vec(vec![42]), 42);
}

#[test]
fn concat_strings_basic() {
    let a = String::from("foo");
    let b = String::from("bar");
    assert_eq!(concat_strings(a, b), "foobar");
}

#[test]
fn concat_strings_with_empty() {
    assert_eq!(concat_strings(String::new(), String::from("hi")), "hi");
    assert_eq!(concat_strings(String::from("hi"), String::new()), "hi");
    assert_eq!(concat_strings(String::new(), String::new()), "");
}

#[test]
fn repeat_string_basic() {
    assert_eq!(repeat_string(String::from("ab"), 3), "ababab");
}

#[test]
fn repeat_string_zero_is_empty() {
    assert_eq!(repeat_string(String::from("xyz"), 0), "");
}

#[test]
fn repeat_string_once() {
    assert_eq!(repeat_string(String::from("rust"), 1), "rust");
}

#[test]
fn repeat_string_empty_source() {
    assert_eq!(repeat_string(String::new(), 5), "");
}

#[test]
fn clone_and_push_returns_extended_copy() {
    let original = vec![1, 2, 3];
    let extended = clone_and_push(&original, 4);
    assert_eq!(extended, vec![1, 2, 3, 4]);
}

#[test]
fn clone_and_push_does_not_mutate_original() {
    // Главная проверка модуля: оригинал НЕ меняется, потому что мы клонировали.
    // То, что `original` остаётся доступным после вызова, доказывает: владение не ушло.
    let original = vec![10, 20];
    let _extended = clone_and_push(&original, 30);
    assert_eq!(original, vec![10, 20]);
}

#[test]
fn clone_and_push_into_empty() {
    let original: Vec<i64> = vec![];
    let extended = clone_and_push(&original, 7);
    assert_eq!(extended, vec![7]);
    assert_eq!(original, Vec::<i64>::new());
}
