// Эти тесты трогать не нужно — это эталон поведения.
//
// Они описывают, КАК функции модуля должны себя вести. Сейчас они падают, потому
// что в src/lib.rs стоит todo!(). Твоя задача — заменить todo!() реализацией так,
// чтобы все assert'ы прошли.

use m07_errors::*;

#[test]
fn parse_int_ok_plain() {
    assert_eq!(parse_int("42"), Ok(42));
}

#[test]
fn parse_int_ok_negative() {
    assert_eq!(parse_int("-7"), Ok(-7));
}

#[test]
fn parse_int_ok_explicit_plus() {
    assert_eq!(parse_int("+3"), Ok(3));
}

#[test]
fn parse_int_ok_zero() {
    assert_eq!(parse_int("0"), Ok(0));
}

#[test]
fn parse_int_err_on_letters() {
    // На нечисловой строке — Err. Точный текст важен: он содержит исходную строку.
    assert_eq!(parse_int("abc"), Err("not a number: 'abc'".to_string()));
}

#[test]
fn parse_int_err_on_empty() {
    assert_eq!(parse_int(""), Err("not a number: ''".to_string()));
}

#[test]
fn parse_int_err_on_trailing_garbage() {
    assert_eq!(parse_int("12x"), Err("not a number: '12x'".to_string()));
}

#[test]
fn divide_ok_exact() {
    assert_eq!(divide(10, 2), Ok(5));
}

#[test]
fn divide_ok_truncates_toward_zero() {
    // Целочисленное деление: 7 / 2 == 3 (дробная часть отбрасывается).
    assert_eq!(divide(7, 2), Ok(3));
}

#[test]
fn divide_ok_negative() {
    assert_eq!(divide(-9, 3), Ok(-3));
}

#[test]
fn divide_by_zero_is_err() {
    assert_eq!(divide(1, 0), Err("division by zero".to_string()));
}

#[test]
fn element_at_ok_first() {
    let v = [10, 20, 30];
    assert_eq!(element_at(&v, 0), Ok(10));
}

#[test]
fn element_at_ok_last() {
    let v = [10, 20, 30];
    assert_eq!(element_at(&v, 2), Ok(30));
}

#[test]
fn element_at_out_of_bounds_is_err() {
    let v = [10, 20, 30];
    assert_eq!(
        element_at(&v, 5),
        Err("index 5 out of bounds for length 3".to_string())
    );
}

#[test]
fn element_at_on_empty_slice_is_err() {
    let v: [i64; 0] = [];
    assert_eq!(
        element_at(&v, 0),
        Err("index 0 out of bounds for length 0".to_string())
    );
}

#[test]
fn sum_parsed_ok() {
    assert_eq!(sum_parsed(&["1", "2", "3"]), Ok(6));
}

#[test]
fn sum_parsed_ok_with_signs() {
    assert_eq!(sum_parsed(&["10", "-4", "+1"]), Ok(7));
}

#[test]
fn sum_parsed_empty_is_zero() {
    assert_eq!(sum_parsed(&[]), Ok(0));
}

#[test]
fn sum_parsed_propagates_first_error() {
    // "x" — не число. Оператор ? должен вернуть ИМЕННО эту ошибку (про 'x'),
    // и не дойти до "4".
    assert_eq!(
        sum_parsed(&["1", "x", "4"]),
        Err("not a number: 'x'".to_string())
    );
}
