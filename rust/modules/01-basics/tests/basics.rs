// Эти тесты трогать не нужно — это эталон поведения.
// Они описывают, КАК должны работать функции из src/lib.rs.
// Сейчас они падают (тела — todo!()), но компилируются.

use m01_basics::*;

#[test]
fn fahrenheit_known_points() {
    // Эти точки представимы во f64 точно, поэтому сравниваем напрямую.
    assert_eq!(to_fahrenheit(0.0), 32.0);
    assert_eq!(to_fahrenheit(100.0), 212.0);
}

#[test]
fn fahrenheit_body_temperature() {
    // 37 °C == 98.6 °F. Здесь нарочно сравниваем с допуском (epsilon),
    // а не через ==: результат зависит от того, как именно ты записал
    // множитель (9.0/5.0 против 1.8), и из-за округления f64 он может
    // отличаться в последнем знаке. Это нормально — числа с плавающей
    // точкой почти никогда не сравнивают на точное равенство.
    let got = to_fahrenheit(37.0);
    assert!((got - 98.6).abs() < 1e-9, "ожидали ~98.6, получили {got}");
}

#[test]
fn fahrenheit_minus_forty_is_the_fixed_point() {
    // -40 °C — единственная температура, где Цельсий и Фаренгейт совпадают.
    assert_eq!(to_fahrenheit(-40.0), -40.0);
}

#[test]
fn is_even_basic() {
    assert!(is_even(0));
    assert!(is_even(2));
    assert!(is_even(4));
    assert!(!is_even(1));
    assert!(!is_even(7));
}

#[test]
fn is_even_handles_negatives() {
    assert!(is_even(-2));
    assert!(is_even(-100));
    assert!(!is_even(-3));
}

#[test]
fn min_of_three_picks_smallest() {
    assert_eq!(min_of_three(3, 1, 2), 1);
    assert_eq!(min_of_three(1, 2, 3), 1);
    assert_eq!(min_of_three(2, 3, 1), 1);
    assert_eq!(min_of_three(-1, -7, 4), -7);
}

#[test]
fn min_of_three_all_equal() {
    assert_eq!(min_of_three(5, 5, 5), 5);
}

#[test]
fn abs_diff_is_symmetric() {
    assert_eq!(abs_diff(10, 3), 7);
    assert_eq!(abs_diff(3, 10), 7);
}

#[test]
fn abs_diff_edge_cases() {
    assert_eq!(abs_diff(-5, 5), 10);
    assert_eq!(abs_diff(5, -5), 10);
    assert_eq!(abs_diff(4, 4), 0);
    assert_eq!(abs_diff(0, 0), 0);
}
