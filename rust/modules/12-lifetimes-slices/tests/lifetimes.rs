// Эти тесты трогать не нужно — это эталон поведения.
// Они описывают, КАК должны работать функции и типы из src/lib.rs. Сейчас они
// падают (внутри todo!()), и станут зелёными, когда ты допишешь реализацию.

use m12_lifetimes::*;

// --- longest: более длинная строка, при равенстве — первый аргумент ---

#[test]
fn longest_second_is_longer() {
    assert_eq!(longest("hi", "hello"), "hello");
}

#[test]
fn longest_first_is_longer() {
    assert_eq!(longest("hello", "hi"), "hello");
}

#[test]
fn longest_equal_length_returns_first() {
    // При одинаковой длине контракт требует вернуть ПЕРВЫЙ аргумент.
    assert_eq!(longest("ab", "cd"), "ab");
}

#[test]
fn longest_with_empty() {
    assert_eq!(longest("", "x"), "x");
    assert_eq!(longest("x", ""), "x");
}

#[test]
fn longest_counts_bytes_not_chars() {
    // "кот" — 6 байт (3 символа по 2 байта), "котик" — 10 байт. Длиннее "котик".
    assert_eq!(longest("кот", "котик"), "котик");
}

#[test]
fn longest_result_borrows_inputs() {
    // Результат — это срез в один из входов: тот же адрес, что у выбранной строки.
    let a = String::from("looong");
    let b = String::from("short");
    let r = longest(&a, &b);
    assert_eq!(r, "looong");
    assert_eq!(r.as_ptr(), a.as_ptr());
}

// --- first_word: срез до первого пробела ---

#[test]
fn first_word_basic() {
    assert_eq!(first_word("hello world"), "hello");
}

#[test]
fn first_word_no_space_returns_whole() {
    assert_eq!(first_word("rust"), "rust");
}

#[test]
fn first_word_empty() {
    assert_eq!(first_word(""), "");
}

#[test]
fn first_word_leading_space() {
    // Строка начинается с пробела → первое слово пустое.
    assert_eq!(first_word(" leading"), "");
}

#[test]
fn first_word_multiple_words() {
    // Берётся только ПЕРВОЕ слово, остальное игнорируется.
    assert_eq!(first_word("one two three"), "one");
}

#[test]
fn first_word_borrows_input() {
    // Возвращаемый срез указывает внутрь исходной строки (без копирования).
    let s = String::from("alpha beta");
    let w = first_word(&s);
    assert_eq!(w, "alpha");
    assert_eq!(w.as_ptr(), s.as_ptr());
}

// --- Wrapper<'a> и first_char ---

#[test]
fn wrapper_first_char_basic() {
    let w = Wrapper { text: "rust" };
    assert_eq!(w.first_char(), Some('r'));
}

#[test]
fn wrapper_first_char_empty() {
    let w = Wrapper { text: "" };
    assert_eq!(w.first_char(), None);
}

#[test]
fn wrapper_first_char_unicode() {
    // 'ё' — один символ Юникода (в UTF-8 занимает 2 байта). Нужен именно символ.
    let w = Wrapper { text: "ёж" };
    assert_eq!(w.first_char(), Some('ё'));
}

#[test]
fn wrapper_holds_borrow() {
    // Обёртка хранит ссылку на чужую строку и читает её через поле text.
    let s = String::from("hello");
    let w = Wrapper { text: &s };
    assert_eq!(w.text, "hello");
    assert_eq!(w.first_char(), Some('h'));
}

// --- max_slice: ссылка на максимум, None для пустого ---

#[test]
fn max_slice_basic() {
    assert_eq!(max_slice(&[3, 1, 4, 1, 5, 9, 2, 6]), Some(&9));
}

#[test]
fn max_slice_negatives() {
    assert_eq!(max_slice(&[-10, -3, -7]), Some(&-3));
}

#[test]
fn max_slice_single() {
    assert_eq!(max_slice(&[42]), Some(&42));
}

#[test]
fn max_slice_empty() {
    assert_eq!(max_slice(&[]), None);
}

#[test]
fn max_slice_returns_reference_into_slice() {
    // Результат — ссылка ВНУТРЬ среза, а не копия: совпадает адрес последнего (макс.) элемента.
    let v = vec![1_i64, 7, 7, 2];
    let m = max_slice(&v).unwrap();
    assert_eq!(*m, 7);
    // max() возвращает ссылку на ПОСЛЕДНИЙ максимум — это элемент с индексом 2.
    assert_eq!(m as *const i64, &v[2] as *const i64);
}
