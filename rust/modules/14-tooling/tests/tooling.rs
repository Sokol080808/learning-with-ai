// Эти тесты трогать не нужно — это эталон поведения.
// Запуск: ./rust/run.sh 14
//
// Они описывают, как функции ДОЛЖНЫ работать после починки багов. Сейчас они красные —
// это нормально: красный тест указывает на баг, зелёный подтверждает починку.

use m14_tooling::*;

// --- sum_first_n: сумма первых n элементов -----------------------------------

#[test]
fn sum_first_n_basic() {
    // Берём первые 2 элемента: 10 + 20 = 30.
    assert_eq!(sum_first_n(&[10, 20, 30, 40], 2), 30);
}

#[test]
fn sum_first_n_three_elements() {
    // Первые 3: 1 + 2 + 3 = 6.
    assert_eq!(sum_first_n(&[1, 2, 3, 4, 5], 3), 6);
}

#[test]
fn sum_first_n_counts_the_very_first_element() {
    // Ключевой случай для off-by-one: один элемент, берём его — должно быть 5, а не 0.
    assert_eq!(sum_first_n(&[5], 1), 5);
}

#[test]
fn sum_first_n_zero_takes_nothing() {
    assert_eq!(sum_first_n(&[1, 2, 3], 0), 0);
}

#[test]
fn sum_first_n_n_bigger_than_len_sums_all() {
    // n >= len => суммируем всё, без выхода за границы: 7 + 7 + 7 = 21.
    assert_eq!(sum_first_n(&[7, 7, 7], 5), 21);
}

#[test]
fn sum_first_n_does_not_consume_slice() {
    // Функция берёт &[i64], значит вектор остаётся нашим после вызова.
    let data = vec![4, 5, 6];
    let _ = sum_first_n(&data, 2);
    assert_eq!(data, vec![4, 5, 6]);
}

// --- max_in: максимум среза --------------------------------------------------

#[test]
fn max_in_positive() {
    assert_eq!(max_in(&[3, 9, 1]), 9);
}

#[test]
fn max_in_all_negative() {
    // Главный случай: все числа отрицательные. Максимум — -2, а НЕ 0.
    assert_eq!(max_in(&[-5, -2, -9]), -2);
}

#[test]
fn max_in_single_element() {
    assert_eq!(max_in(&[42]), 42);
}

#[test]
fn max_in_max_is_first() {
    assert_eq!(max_in(&[100, 1, 2, 3]), 100);
}

#[test]
fn max_in_max_is_last() {
    assert_eq!(max_in(&[1, 2, 3, 100]), 100);
}

// --- average: среднее арифметическое -----------------------------------------

#[test]
fn average_two_numbers() {
    assert_eq!(average(&[2, 4]), 3.0);
}

#[test]
fn average_gives_fraction() {
    // Среднее должно быть дробным: (1+2+3+4)/4 = 2.5, а не 2.
    assert_eq!(average(&[1, 2, 3, 4]), 2.5);
}

#[test]
fn average_single_element() {
    // Делитель — это КОЛИЧЕСТВО элементов (1), а не (1 - 1) = 0.
    assert_eq!(average(&[5]), 5.0);
}

#[test]
fn average_three_numbers() {
    // (1 + 2 + 4) / 3 = 7 / 3 = 2.333... — проверяем, что деление вещественное.
    let a = average(&[1, 2, 4]);
    assert!((a - (7.0 / 3.0)).abs() < 1e-9);
}

// --- has_duplicate: есть ли повтор значения -----------------------------------

#[test]
fn has_duplicate_none() {
    // Все элементы уникальны — НЕ должно находиться дубликата.
    assert!(!has_duplicate(&[1, 2, 3]));
}

#[test]
fn has_duplicate_yes() {
    assert!(has_duplicate(&[1, 2, 1]));
}

#[test]
fn has_duplicate_single_element_is_not_a_duplicate() {
    // Один элемент сравнивать не с чем — дубликата нет.
    assert!(!has_duplicate(&[5]));
}

#[test]
fn has_duplicate_empty() {
    assert!(!has_duplicate(&[]));
}

#[test]
fn has_duplicate_adjacent() {
    assert!(has_duplicate(&[7, 7]));
}

// --- repeat_str: повторение строки -------------------------------------------

#[test]
fn repeat_str_three_times() {
    assert_eq!(repeat_str("ab", 3), "ababab");
}

#[test]
fn repeat_str_once_returns_the_string_itself() {
    // Ключевой случай для off-by-one: один повтор — это сама строка, а не пустая.
    assert_eq!(repeat_str("hi", 1), "hi");
}

#[test]
fn repeat_str_zero_is_empty() {
    assert_eq!(repeat_str("x", 0), "");
}

#[test]
fn repeat_str_returns_owned_string() {
    // Результат — владеющая String, ею можно свободно распоряжаться.
    let mut s = repeat_str("na", 2);
    s.push_str("!");
    assert_eq!(s, "nana!");
}
