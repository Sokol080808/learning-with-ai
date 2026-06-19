// Эти тесты трогать не нужно — это эталон поведения.
// Они описывают, как ДОЛЖНЫ вести себя функции и типы из src/lib.rs модуля 06.
// Сейчас они падают (внутри `todo!()` — паника), но компилируются. Реализуй стаб —
// и красное станет зелёным.

use m06_enums::*;

// Допуск для сравнения чисел с плавающей точкой: прямое == для f64 ненадёжно.
fn approx(a: f64, b: f64) -> bool {
    (a - b).abs() < 1e-9
}

// --- Идея 1: enum с данными + метод area через match ---

#[test]
fn rect_area_is_width_times_height() {
    let r = Shape::Rect(3.0, 4.0);
    assert!(approx(r.area(), 12.0));
}

#[test]
fn rect_area_unit_square() {
    let r = Shape::Rect(1.0, 1.0);
    assert!(approx(r.area(), 1.0));
}

#[test]
fn circle_area_uses_pi_r_squared() {
    let c = Shape::Circle(2.0);
    // π · 2² = 4π
    assert!(approx(c.area(), std::f64::consts::PI * 4.0));
}

#[test]
fn circle_area_radius_one_is_pi() {
    let c = Shape::Circle(1.0);
    assert!(approx(c.area(), std::f64::consts::PI));
}

#[test]
fn shape_variants_carry_their_own_data() {
    // Разные варианты — разные данные, и они не путаются.
    let shapes = [Shape::Circle(0.0), Shape::Rect(2.0, 5.0)];
    assert!(approx(shapes[0].area(), 0.0));
    assert!(approx(shapes[1].area(), 10.0));
}

// --- Идея 2: safe_div -> Option<f64> ---

#[test]
fn safe_div_normal() {
    assert_eq!(safe_div(10.0, 2.0), Some(5.0));
}

#[test]
fn safe_div_by_zero_is_none() {
    assert_eq!(safe_div(10.0, 0.0), None);
}

#[test]
fn safe_div_zero_numerator() {
    assert_eq!(safe_div(0.0, 4.0), Some(0.0));
}

#[test]
fn safe_div_negative() {
    assert_eq!(safe_div(-9.0, 3.0), Some(-3.0));
}

// --- Идея 3: first_even -> Option<i64> ---

#[test]
fn first_even_finds_first() {
    assert_eq!(first_even(&[1, 3, 4, 6]), Some(4));
}

#[test]
fn first_even_at_start() {
    assert_eq!(first_even(&[2, 7, 9]), Some(2));
}

#[test]
fn first_even_none_when_all_odd() {
    assert_eq!(first_even(&[1, 3, 5, 7]), None);
}

#[test]
fn first_even_none_on_empty() {
    assert_eq!(first_even(&[]), None);
}

#[test]
fn first_even_handles_negatives() {
    // -4 чётное и идёт раньше 2
    assert_eq!(first_even(&[-3, -4, 2]), Some(-4));
}

// --- Идея 4: describe(Option<i64>) -> String ---

#[test]
fn describe_some() {
    assert_eq!(describe(Some(7)), "value: 7");
}

#[test]
fn describe_some_negative() {
    assert_eq!(describe(Some(-1)), "value: -1");
}

#[test]
fn describe_none() {
    assert_eq!(describe(None), "nothing");
}

#[test]
fn describe_composes_with_safe_div_via_round_trip() {
    // safe_div возвращает None при делении на ноль; здесь проверяем именно describe
    // на «ничего».
    assert_eq!(describe(None), "nothing");
    assert_eq!(describe(Some(0)), "value: 0");
}
