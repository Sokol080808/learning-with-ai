// Эти тесты трогать не нужно — это эталон поведения.
// Они описывают, как ДОЛЖНЫ вести себя типы и функции из src/lib.rs модуля 13.
// Сейчас они падают (внутри `todo!()` — паника), но компилируются. Реализуй стаб —
// и красное станет зелёным.

use m13_smart_pointers::*;

// --- Идея 1: Box<List> — рекурсивный тип; len и sum через рекурсию ---

#[test]
fn empty_list_has_len_zero() {
    let l = List::Nil;
    assert_eq!(l.len(), 0);
}

#[test]
fn empty_list_has_sum_zero() {
    let l = List::Nil;
    assert_eq!(l.sum(), 0);
}

#[test]
fn manual_cons_list_len() {
    // Cons(1, Cons(2, Cons(3, Nil))) — три элемента
    let l = List::Cons(
        1,
        Box::new(List::Cons(2, Box::new(List::Cons(3, Box::new(List::Nil))))),
    );
    assert_eq!(l.len(), 3);
}

#[test]
fn manual_cons_list_sum() {
    // 10 + 20 + 30 = 60
    let l = List::Cons(
        10,
        Box::new(List::Cons(20, Box::new(List::Cons(30, Box::new(List::Nil))))),
    );
    assert_eq!(l.sum(), 60);
}

#[test]
fn single_element_list() {
    let l = List::Cons(42, Box::new(List::Nil));
    assert_eq!(l.len(), 1);
    assert_eq!(l.sum(), 42);
}

// --- Идея 1 (продолжение): build_list собирает Cons-список в правильном порядке ---

#[test]
fn build_list_empty_is_nil() {
    assert_eq!(build_list(&[]), List::Nil);
}

#[test]
fn build_list_single() {
    let expected = List::Cons(7, Box::new(List::Nil));
    assert_eq!(build_list(&[7]), expected);
}

#[test]
fn build_list_preserves_order() {
    // Первый элемент среза должен стать головой всего списка.
    let expected = List::Cons(
        1,
        Box::new(List::Cons(2, Box::new(List::Cons(3, Box::new(List::Nil))))),
    );
    assert_eq!(build_list(&[1, 2, 3]), expected);
}

#[test]
fn build_list_len_and_sum_agree() {
    let l = build_list(&[5, 10, 15, 20]);
    assert_eq!(l.len(), 4);
    assert_eq!(l.sum(), 50);
}

#[test]
fn build_list_handles_negatives() {
    let l = build_list(&[-3, 4, -1]);
    assert_eq!(l.len(), 3);
    assert_eq!(l.sum(), 0);
}

// --- Идея 3: RefCell — изменение через &self ---

#[test]
fn counter_starts_at_zero() {
    let c = Counter::new();
    assert_eq!(c.get(), 0);
}

#[test]
fn counter_increment_once() {
    let c = Counter::new();
    c.increment();
    assert_eq!(c.get(), 1);
}

#[test]
fn counter_increment_many() {
    let c = Counter::new();
    for _ in 0..5 {
        c.increment();
    }
    assert_eq!(c.get(), 5);
}

#[test]
fn counter_mutates_through_shared_reference() {
    // Ключевая идея RefCell: меняем состояние, имея лишь общую ссылку &self.
    let c = Counter::new();
    let view: &Counter = &c; // обычная неизменяемая ссылка
    view.increment();
    view.increment();
    assert_eq!(view.get(), 2);
}

#[test]
fn counter_default_is_zero() {
    let c = Counter::default();
    assert_eq!(c.get(), 0);
}

// --- Идея 2: Rc — разделяемое владение, счётчик сильных ссылок растёт ---

#[test]
fn rc_clone_increases_strong_count() {
    // После хотя бы одного клонирования живых владельцев должно быть как минимум 2.
    let count = rc_clone_count();
    assert!(
        count >= 2,
        "ожидали как минимум 2 сильные ссылки после клонирования, получили {count}"
    );
}
