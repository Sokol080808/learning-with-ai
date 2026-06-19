// Эти тесты трогать не нужно — это эталон поведения.
//
// Они описывают, КАК должен вести себя твой код в src/lib.rs. Сейчас они падают
// (внутри `todo!()` — паника). Реализуй задания так, чтобы все стали зелёными:
//   ./rust/run.sh 10

use m10_traits::*;
use std::f64::consts::PI;

// Сравнение чисел с плавающей точкой — только через допуск, никогда `==`.
fn approx(a: f64, b: f64) -> bool {
    (a - b).abs() < 1e-9
}

// ───────────────────────── Задание 1. Трейт-объекты ─────────────────────────

#[test]
fn circle_area() {
    let c = Circle { r: 2.0 };
    assert!(approx(c.area(), PI * 4.0));
}

#[test]
fn square_area() {
    let s = Square { side: 3.0 };
    assert!(approx(s.area(), 9.0));
}

#[test]
fn total_area_of_mixed_shapes() {
    // Срез разнородных фигур, спрятанных за Box<dyn Shape>.
    let shapes: Vec<Box<dyn Shape>> = vec![
        Box::new(Circle { r: 1.0 }), // площадь = π
        Box::new(Square { side: 2.0 }), // площадь = 4
        Box::new(Square { side: 5.0 }), // площадь = 25
    ];
    let expected = PI + 4.0 + 25.0;
    assert!(approx(total_area(&shapes), expected));
}

#[test]
fn total_area_empty_is_zero() {
    let shapes: Vec<Box<dyn Shape>> = Vec::new();
    assert!(approx(total_area(&shapes), 0.0));
}

// ───────────────────────── Задание 2. Display для Point ─────────────────────────

#[test]
fn point_display_integers() {
    let p = Point { x: 1.0, y: 2.0 };
    assert_eq!(format!("{}", p), "(1, 2)");
}

#[test]
fn point_display_fractional() {
    let p = Point { x: 1.5, y: -2.0 };
    assert_eq!(format!("{}", p), "(1.5, -2)");
}

#[test]
fn point_to_string_uses_display() {
    // .to_string() автоматически появляется у любого типа с impl Display.
    let p = Point { x: 0.0, y: 0.0 };
    assert_eq!(p.to_string(), "(0, 0)");
}

// ───────────────────────── Задание 3. Iterator для Counter ─────────────────────────

#[test]
fn counter_yields_one_through_five() {
    let collected: Vec<u32> = Counter::new().collect();
    assert_eq!(collected, vec![1, 2, 3, 4, 5]);
}

#[test]
fn counter_stops_after_five() {
    let mut c = Counter::new();
    assert_eq!(c.next(), Some(1));
    assert_eq!(c.next(), Some(2));
    assert_eq!(c.next(), Some(3));
    assert_eq!(c.next(), Some(4));
    assert_eq!(c.next(), Some(5));
    assert_eq!(c.next(), None);
    assert_eq!(c.next(), None); // и дальше всегда None
}

#[test]
fn counter_works_with_iterator_adapters() {
    // Раз реализован next, доступны map/filter/sum и прочие комбинаторы.
    let only_even: Vec<u32> = Counter::new().filter(|n| n % 2 == 0).collect();
    assert_eq!(only_even, vec![2, 4]);
}

#[test]
fn counter_sum_is_fifteen() {
    assert_eq!(counter_sum(), 15);
}
