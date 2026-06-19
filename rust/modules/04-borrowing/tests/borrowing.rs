// Эти тесты трогать не нужно — это эталон поведения.
// Они подключают твой крейт и проверяют ПУБЛИЧНЫЙ API модуля 04.
// Пока тела в src/lib.rs — `todo!()`, тесты компилируются, но падают (паника).
// Реализуй функции — и они станут зелёными.

use m04_borrowing::*;

// ───────────────────────── sum_slice ─────────────────────────

#[test]
fn sum_slice_basic() {
    let v = vec![1, 2, 3, 4, 5];
    // Заметь: передаём `&v` — ОДАЛЖИВАЕМ вектор, не отдаём владение.
    assert_eq!(sum_slice(&v), 15);
    // Поэтому `v` всё ещё наш и доступен после вызова:
    assert_eq!(v.len(), 5);
}

#[test]
fn sum_slice_empty_is_zero() {
    let v: Vec<i64> = Vec::new();
    assert_eq!(sum_slice(&v), 0);
}

#[test]
fn sum_slice_with_negatives() {
    let v = vec![10, -3, -7, 5];
    assert_eq!(sum_slice(&v), 5);
}

#[test]
fn sum_slice_accepts_array_slice() {
    // `&[i64]` принимает и срез массива, и подсрез вектора — в этом сила срезов.
    let arr = [100i64, 200, 300];
    assert_eq!(sum_slice(&arr), 600);

    let v = vec![1i64, 2, 3, 4];
    assert_eq!(sum_slice(&v[1..3]), 5); // элементы 2 и 3
}

// ───────────────────────── push_n ─────────────────────────

#[test]
fn push_n_appends_correct_count() {
    let mut v = vec![1, 2];
    push_n(&mut v, 9, 3);
    assert_eq!(v, vec![1, 2, 9, 9, 9]);
}

#[test]
fn push_n_zero_count_no_change() {
    let mut v = vec![7, 8];
    push_n(&mut v, 42, 0);
    assert_eq!(v, vec![7, 8]);
}

#[test]
fn push_n_into_empty() {
    let mut v: Vec<i64> = Vec::new();
    push_n(&mut v, -1, 4);
    assert_eq!(v, vec![-1, -1, -1, -1]);
    assert_eq!(v.len(), 4);
}

// ───────────────────────── double_all ─────────────────────────

#[test]
fn double_all_doubles_each() {
    let mut v = vec![1, 2, 3, 4];
    double_all(&mut v);
    assert_eq!(v, vec![2, 4, 6, 8]);
}

#[test]
fn double_all_empty_stays_empty() {
    let mut v: Vec<i64> = Vec::new();
    double_all(&mut v);
    assert_eq!(v, Vec::<i64>::new());
}

#[test]
fn double_all_handles_negatives_and_zero() {
    let mut v = vec![-3, 0, 5];
    double_all(&mut v);
    assert_eq!(v, vec![-6, 0, 10]);
}

#[test]
fn double_all_idempotent_length() {
    // Удвоение НЕ должно менять длину — мы правим элементы на месте, а не пушим.
    let mut v = vec![10, 20, 30];
    double_all(&mut v);
    assert_eq!(v.len(), 3);
    assert_eq!(v, vec![20, 40, 60]);
}

// ───────────────────────── max_ref ─────────────────────────

#[test]
fn max_ref_returns_reference_to_max() {
    let v = vec![3, 7, 2, 9, 4];
    let m = max_ref(&v);
    assert_eq!(m, Some(&9));
    // Разыменуем ссылку и сравним значение:
    assert_eq!(*m.unwrap(), 9);
}

#[test]
fn max_ref_empty_is_none() {
    let v: Vec<i64> = Vec::new();
    assert_eq!(max_ref(&v), None);
}

#[test]
fn max_ref_single_element() {
    let v = vec![42];
    assert_eq!(max_ref(&v), Some(&42));
}

#[test]
fn max_ref_with_negatives() {
    let v = vec![-10, -3, -50, -1];
    assert_eq!(max_ref(&v), Some(&-1));
}

#[test]
fn max_ref_reference_points_into_slice() {
    // Возвращается ссылка ВНУТРЬ среза, а не копия. Значение по ней совпадает
    // с настоящим максимумом независимо от его позиции.
    let v = vec![5, 1, 8, 8, 2];
    let m = max_ref(&v).unwrap();
    assert_eq!(*m, 8);
}
