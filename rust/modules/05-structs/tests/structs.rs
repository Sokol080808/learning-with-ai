// Эти тесты трогать не нужно — это эталон поведения.
// Запуск: ./rust/run.sh 05

use m05_structs::*;

// --- Point::new и доступ к полям -------------------------------------------

#[test]
fn point_new_sets_fields() {
    let p = Point::new(3.0, -4.0);
    assert_eq!(p.x, 3.0);
    assert_eq!(p.y, -4.0);
}

// --- distance_to ------------------------------------------------------------

#[test]
fn distance_classic_3_4_5() {
    let a = Point::new(0.0, 0.0);
    let b = Point::new(3.0, 4.0);
    // Классический треугольник: расстояние ровно 5.0.
    assert_eq!(a.distance_to(&b), 5.0);
}

#[test]
fn distance_is_symmetric() {
    let a = Point::new(1.0, 2.0);
    let b = Point::new(4.0, 6.0);
    assert_eq!(a.distance_to(&b), b.distance_to(&a));
}

#[test]
fn distance_to_itself_is_zero() {
    let a = Point::new(7.5, -2.25);
    assert_eq!(a.distance_to(&a), 0.0);
}

#[test]
fn distance_does_not_consume_arguments() {
    // distance_to берёт &self и &Point, значит обе точки остаются доступны после вызова.
    let a = Point::new(0.0, 0.0);
    let b = Point::new(0.0, 5.0);
    let d = a.distance_to(&b);
    assert_eq!(d, 5.0);
    // Точки всё ещё наши — пользуемся ими снова:
    assert_eq!(a.x, 0.0);
    assert_eq!(b.y, 5.0);
}

// --- Rectangle::area / is_square -------------------------------------------

#[test]
fn rectangle_area() {
    let r = Rectangle::new(3.0, 4.0);
    assert_eq!(r.area(), 12.0);
}

#[test]
fn rectangle_is_square_true() {
    let r = Rectangle::new(5.0, 5.0);
    assert!(r.is_square());
    assert_eq!(r.area(), 25.0);
}

#[test]
fn rectangle_is_square_false() {
    let r = Rectangle::new(2.0, 3.0);
    assert!(!r.is_square());
}

// --- derive(PartialEq, Clone, Debug) для Point ------------------------------

#[test]
fn points_equal_by_value() {
    let a = Point::new(1.0, 2.0);
    let b = Point::new(1.0, 2.0);
    // PartialEq: одинаковые поля => точки равны.
    assert_eq!(a, b);
}

#[test]
fn points_not_equal() {
    let a = Point::new(1.0, 2.0);
    let c = Point::new(1.0, 2.5);
    assert_ne!(a, c);
}

#[test]
fn point_clone_is_equal_to_original() {
    let a = Point::new(-3.0, 9.0);
    let b = a.clone(); // Clone: явная копия
    assert_eq!(a, b);
}

// --- Кортежная структура Color ---------------------------------------------

#[test]
fn color_fields_by_index() {
    let c = Color(255, 128, 0);
    assert_eq!(c.0, 255);
    assert_eq!(c.1, 128);
    assert_eq!(c.2, 0);
}

#[test]
fn color_brightness_sums_components() {
    let c = Color(255, 255, 255);
    // 255 + 255 + 255 — сумма не влезает в u8, поэтому метод возвращает u32.
    assert_eq!(c.brightness(), 765);
}

#[test]
fn colors_equal_by_value() {
    let a = Color(10, 20, 30);
    let b = Color(10, 20, 30);
    assert_eq!(a, b);
    assert_ne!(a, Color(10, 20, 31));
}
