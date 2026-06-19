// Эти тесты трогать не нужно — это эталон поведения.
// Они описывают, КАК должны работать функции из src/lib.rs. Сейчас они падают
// (внутри todo!()), и станут зелёными, когда ты допишешь реализацию.

use m11_iterators::*;

// --- apply: вызов переданного замыкания/функции ---

#[test]
fn apply_increment() {
    assert_eq!(apply(|n| n + 1, 41), 42);
}

#[test]
fn apply_square() {
    assert_eq!(apply(|n| n * n, 9), 81);
}

#[test]
fn apply_with_plain_fn() {
    // apply принимает не только замыкания, но и обычную функцию-указатель.
    fn negate(n: i64) -> i64 {
        -n
    }
    assert_eq!(apply(negate, 7), -7);
}

#[test]
fn apply_captures_environment() {
    // Замыкание может захватить переменную из окружения (здесь — k по ссылке).
    let k = 10;
    assert_eq!(apply(|n| n * k, 5), 50);
}

// --- make_adder: фабрика замыканий, impl Fn в возврате ---

#[test]
fn make_adder_basic() {
    let add5 = make_adder(5);
    assert_eq!(add5(10), 15);
    assert_eq!(add5(-5), 0);
}

#[test]
fn make_adder_callable_many_times() {
    // Граница Fn (а не FnOnce) обещает многократный вызов.
    let add3 = make_adder(3);
    assert_eq!(add3(0), 3);
    assert_eq!(add3(0), 3);
    assert_eq!(add3(100), 103);
}

#[test]
fn make_adder_independent_factories() {
    let add2 = make_adder(2);
    let add100 = make_adder(100);
    assert_eq!(add2(2), 4);
    assert_eq!(add100(2), 102);
}

// --- sum_of_squares: цепочка map + sum ---

#[test]
fn sum_of_squares_basic() {
    assert_eq!(sum_of_squares(&[1, 2, 3]), 14);
}

#[test]
fn sum_of_squares_empty() {
    assert_eq!(sum_of_squares(&[]), 0);
}

#[test]
fn sum_of_squares_with_negatives() {
    assert_eq!(sum_of_squares(&[-2, 5]), 29);
}

// --- evens: filter ---

#[test]
fn evens_basic() {
    assert_eq!(evens(&[1, 2, 3, 4]), vec![2, 4]);
}

#[test]
fn evens_none() {
    assert_eq!(evens(&[1, 3, 5]), vec![]);
}

#[test]
fn evens_includes_zero_and_negatives() {
    assert_eq!(evens(&[0, -2, 7, -8]), vec![0, -2, -8]);
}

#[test]
fn evens_preserves_order() {
    assert_eq!(evens(&[8, 6, 4, 2]), vec![8, 6, 4, 2]);
}

// --- word_lengths: map по срезу строк ---

#[test]
fn word_lengths_basic() {
    assert_eq!(word_lengths(&["a", "bb", "ccc"]), vec![1, 2, 3]);
}

#[test]
fn word_lengths_empty_slice() {
    let empty: Vec<usize> = word_lengths(&[]);
    assert_eq!(empty, vec![]);
}

#[test]
fn word_lengths_empty_word() {
    assert_eq!(word_lengths(&["", "hello"]), vec![0, 5]);
}
