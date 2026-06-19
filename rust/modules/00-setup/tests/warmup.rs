// Эти тесты трогать не нужно — это эталон поведения.
//
// Крейт модуля подключается так: `use m00_setup::*;`. Сейчас все тесты
// падают, потому что тела функций в src/lib.rs — это `todo!()` (паника в
// рантайме). Реализуй функции — и тесты позеленеют.

use m00_setup::*;

#[test]
fn add_basic() {
    assert_eq!(add(2, 3), 5);
    assert_eq!(add(40, 2), 42);
}

#[test]
fn add_with_zero() {
    assert_eq!(add(0, 0), 0);
    assert_eq!(add(7, 0), 7);
    assert_eq!(add(0, 7), 7);
}

#[test]
fn add_negatives() {
    assert_eq!(add(-4, 4), 0);
    assert_eq!(add(-5, -3), -8);
    assert_eq!(add(10, -3), 7);
}

#[test]
fn seconds_in_one_hour() {
    assert_eq!(seconds_in(1), 3600);
}

#[test]
fn seconds_in_several_hours() {
    assert_eq!(seconds_in(2), 7200);
    assert_eq!(seconds_in(24), 86_400);
}

#[test]
fn seconds_in_zero() {
    assert_eq!(seconds_in(0), 0);
}
