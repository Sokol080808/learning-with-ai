// Randomized property tests for m12_lifetimes.
// These complement the deterministic examples in lifetimes.rs — they check
// INVARIANTS over large random input spaces, not just fixed cases.

use m12_lifetimes::*;

// ---------------------------------------------------------------------------
// Детерминированный ГПСЧ (xorshift64*) — без внешних крейтов, воспроизводимо.
// ---------------------------------------------------------------------------

fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}

/// Целое в диапазоне [lo, hi] включительно.
fn in_range(state: &mut u64, lo: i64, hi: i64) -> i64 {
    let span = (hi - lo + 1) as u64;
    lo + (next_u64(state) % span) as i64
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Генерирует случайную строку из ASCII-символов (буквы a–z и пробелы).
fn random_ascii_str(state: &mut u64, len: usize) -> String {
    const CHARS: &[u8] = b"abcdefghijklmnopqrstuvwxyz ";
    (0..len)
        .map(|_| {
            let idx = (next_u64(state) % CHARS.len() as u64) as usize;
            CHARS[idx] as char
        })
        .collect()
}

/// Генерирует вектор случайных i64 заданной длины из диапазона [lo, hi].
fn random_i64_vec(state: &mut u64, len: usize, lo: i64, hi: i64) -> Vec<i64> {
    (0..len)
        .map(|_| in_range(state, lo, hi))
        .collect()
}

// ---------------------------------------------------------------------------
// longest — свойства
// ---------------------------------------------------------------------------

#[test]
fn prop_longest_result_equals_one_of_inputs() {
    // Результат всегда совпадает с одним из двух входов (не какое-то третье значение).
    let mut rng: u64 = 0xDEAD_BEEF_1234_5678;
    for _ in 0..800 {
        let la = in_range(&mut rng, 0, 20) as usize;
        let lb = in_range(&mut rng, 0, 20) as usize;
        let a = random_ascii_str(&mut rng, la);
        let b = random_ascii_str(&mut rng, lb);
        let r = longest(&a, &b);
        assert!(
            r == a.as_str() || r == b.as_str(),
            "longest returned something other than one of the inputs"
        );
    }
}

#[test]
fn prop_longest_length_is_max_of_inputs() {
    // Длина результата всегда равна max(a.len(), b.len()).
    let mut rng: u64 = 0xCAFE_BABE_0000_0001;
    for _ in 0..800 {
        let la = in_range(&mut rng, 0, 30) as usize;
        let lb = in_range(&mut rng, 0, 30) as usize;
        let a = random_ascii_str(&mut rng, la);
        let b = random_ascii_str(&mut rng, lb);
        let r = longest(&a, &b);
        assert_eq!(
            r.len(),
            a.len().max(b.len()),
            "result length != max(a.len(), b.len()): a={a:?} b={b:?}"
        );
    }
}

#[test]
fn prop_longest_equal_length_returns_first() {
    // Когда оба входа одинаковой длины, должен вернуться ПЕРВЫЙ аргумент (по адресу).
    let mut rng: u64 = 0x1111_2222_3333_4444;
    for _ in 0..800 {
        let len = in_range(&mut rng, 0, 25) as usize;
        let a = random_ascii_str(&mut rng, len);
        let b = random_ascii_str(&mut rng, len);
        let r = longest(&a, &b);
        assert_eq!(
            r.as_ptr(),
            a.as_ptr(),
            "equal-length: expected first argument to be returned"
        );
    }
}

#[test]
fn prop_longest_second_longer_returns_second() {
    // Когда b строго длиннее a, результат — b.
    let mut rng: u64 = 0xABCD_EF01_2345_6789;
    for _ in 0..800 {
        let la = in_range(&mut rng, 0, 20) as usize;
        // lb строго больше la
        let lb = la + 1 + (next_u64(&mut rng) % 10) as usize;
        let a = random_ascii_str(&mut rng, la);
        let b = random_ascii_str(&mut rng, lb);
        let r = longest(&a, &b);
        assert_eq!(
            r.as_ptr(),
            b.as_ptr(),
            "b is strictly longer but result is not b"
        );
    }
}

#[test]
fn prop_longest_first_longer_returns_first() {
    // Когда a строго длиннее b, результат — a.
    let mut rng: u64 = 0xFEDC_BA98_7654_3210;
    for _ in 0..800 {
        let lb = in_range(&mut rng, 0, 20) as usize;
        let la = lb + 1 + (next_u64(&mut rng) % 10) as usize;
        let a = random_ascii_str(&mut rng, la);
        let b = random_ascii_str(&mut rng, lb);
        let r = longest(&a, &b);
        assert_eq!(
            r.as_ptr(),
            a.as_ptr(),
            "a is strictly longer but result is not a"
        );
    }
}

#[test]
fn prop_longest_result_is_not_shorter_than_either_input() {
    // Результат не может быть короче ни одного из входов
    // (то есть он выбирает наибольший, а не произвольный).
    let mut rng: u64 = 0x0F0F_0F0F_F0F0_F0F0;
    for _ in 0..800 {
        let la = in_range(&mut rng, 0, 30) as usize;
        let lb = in_range(&mut rng, 0, 30) as usize;
        let a = random_ascii_str(&mut rng, la);
        let b = random_ascii_str(&mut rng, lb);
        let r = longest(&a, &b);
        assert!(
            r.len() >= a.len() && r.len() >= b.len(),
            "result is shorter than one of the inputs"
        );
    }
}

// ---------------------------------------------------------------------------
// first_word — свойства
// ---------------------------------------------------------------------------

#[test]
fn prop_first_word_is_prefix_of_input() {
    // Результат — всегда префикс исходной строки (указывает внутрь неё).
    let mut rng: u64 = 0x1234_5678_9ABC_DEF0;
    for _ in 0..800 {
        let len = in_range(&mut rng, 0, 40) as usize;
        let s = random_ascii_str(&mut rng, len);
        let w = first_word(&s);
        assert!(
            s.starts_with(w),
            "first_word result is not a prefix of the input: s={s:?} w={w:?}"
        );
    }
}

#[test]
fn prop_first_word_no_space_in_result() {
    // Возвращаемое слово никогда не содержит пробела.
    let mut rng: u64 = 0xBEEF_CAFE_1234_ABCD;
    for _ in 0..800 {
        let len = in_range(&mut rng, 0, 40) as usize;
        let s = random_ascii_str(&mut rng, len);
        let w = first_word(&s);
        assert!(
            !w.contains(' '),
            "first_word result contains a space: s={s:?} w={w:?}"
        );
    }
}

#[test]
fn prop_first_word_agrees_with_oracle() {
    // Oracle: берём всё до первого пробела через std-итератор — должно совпадать.
    let mut rng: u64 = 0x0ACE_0ACE_0ACE_0ACE;
    for _ in 0..800 {
        let len = in_range(&mut rng, 0, 40) as usize;
        let s = random_ascii_str(&mut rng, len);
        let expected: &str = match s.find(' ') {
            Some(i) => &s[..i],
            None => &s,
        };
        let got = first_word(&s);
        assert_eq!(
            got, expected,
            "first_word disagreed with oracle: s={s:?}"
        );
    }
}

#[test]
fn prop_first_word_borrows_input_pointer() {
    // Возвращаемый срез всегда начинается с того же адреса, что и входная строка
    // (это следует из того, что первое слово — всегда начало строки).
    let mut rng: u64 = 0xAAAA_BBBB_CCCC_DDDD;
    for _ in 0..800 {
        let len = in_range(&mut rng, 1, 40) as usize; // не пустая, чтобы сравнивать указатели
        let s = random_ascii_str(&mut rng, len);
        let w = first_word(&s);
        // first_word всегда возвращает срез от начала → тот же базовый указатель
        assert_eq!(
            w.as_ptr(),
            s.as_ptr(),
            "first_word result does not start at s.as_ptr()"
        );
    }
}

#[test]
fn prop_first_word_length_le_input_length() {
    // Первое слово не длиннее всей строки.
    let mut rng: u64 = 0x1357_9BDF_2468_ACE0;
    for _ in 0..800 {
        let len = in_range(&mut rng, 0, 40) as usize;
        let s = random_ascii_str(&mut rng, len);
        let w = first_word(&s);
        assert!(
            w.len() <= s.len(),
            "first_word result longer than input"
        );
    }
}

// Edge cases not covered by lifetimes.rs
#[test]
fn first_word_only_spaces() {
    assert_eq!(first_word("   "), "");
}

#[test]
fn first_word_trailing_space() {
    assert_eq!(first_word("word "), "word");
}

// ---------------------------------------------------------------------------
// Wrapper / first_char — свойства
// ---------------------------------------------------------------------------

#[test]
fn prop_wrapper_first_char_some_iff_nonempty() {
    // first_char() возвращает Some iff строка непустая — None только для пустой.
    let mut rng: u64 = 0xFACE_B00C_1234_5678;
    for _ in 0..800 {
        let len = in_range(&mut rng, 0, 30) as usize;
        let s = random_ascii_str(&mut rng, len);
        let w = Wrapper { text: &s };
        if s.is_empty() {
            assert_eq!(w.first_char(), None, "expected None for empty string");
        } else {
            assert!(w.first_char().is_some(), "expected Some for non-empty string");
        }
    }
}

#[test]
fn prop_wrapper_first_char_agrees_with_chars_next() {
    // first_char() == s.chars().next() — oracle через стандартный метод.
    let mut rng: u64 = 0xDEAD_C0DE_DEAD_C0DE;
    for _ in 0..800 {
        let len = in_range(&mut rng, 0, 30) as usize;
        let s = random_ascii_str(&mut rng, len);
        let w = Wrapper { text: &s };
        let expected = s.chars().next();
        assert_eq!(
            w.first_char(),
            expected,
            "first_char disagreed with oracle for s={s:?}"
        );
    }
}

#[test]
fn prop_wrapper_text_field_matches_input() {
    // Поле text хранит ровно ту строку, что передали — не копию, а ссылку.
    let mut rng: u64 = 0x9999_8888_7777_6666;
    for _ in 0..500 {
        let len = in_range(&mut rng, 0, 30) as usize;
        let s = random_ascii_str(&mut rng, len);
        let w = Wrapper { text: &s };
        assert_eq!(w.text, s.as_str());
        assert_eq!(w.text.as_ptr(), s.as_ptr());
    }
}

// ---------------------------------------------------------------------------
// max_slice — свойства
// ---------------------------------------------------------------------------

#[test]
fn prop_max_slice_none_iff_empty() {
    // max_slice возвращает None только для пустого среза.
    let mut rng: u64 = 0x2222_3333_4444_5555;
    for _ in 0..800 {
        let len = in_range(&mut rng, 0, 50) as usize;
        let v = random_i64_vec(&mut rng, len, -10_000, 10_000);
        let result = max_slice(&v);
        if v.is_empty() {
            assert_eq!(result, None);
        } else {
            assert!(result.is_some());
        }
    }
}

#[test]
fn prop_max_slice_agrees_with_iterator_max() {
    // Oracle: v.iter().max() — тот же контракт, результат должен совпадать.
    let mut rng: u64 = 0xC0FF_EE00_1111_2222;
    for _ in 0..800 {
        let len = in_range(&mut rng, 0, 50) as usize;
        let v = random_i64_vec(&mut rng, len, -10_000, 10_000);
        let expected = v.iter().max();
        let got = max_slice(&v);
        assert_eq!(
            got.map(|r| *r),
            expected.copied(),
            "max_slice disagreed with oracle"
        );
    }
}

#[test]
fn prop_max_slice_result_ge_all_elements() {
    // Результат не меньше ни одного элемента среза (это и есть определение максимума).
    let mut rng: u64 = 0x5A5A_5A5A_A5A5_A5A5;
    for _ in 0..800 {
        let len = in_range(&mut rng, 1, 50) as usize; // непустой
        let v = random_i64_vec(&mut rng, len, -10_000, 10_000);
        let max_val = *max_slice(&v).unwrap();
        for &elem in &v {
            assert!(
                max_val >= elem,
                "max_slice result {max_val} < element {elem}"
            );
        }
    }
}

#[test]
fn prop_max_slice_result_is_element_of_slice() {
    // Результат — это один из элементов среза (адрес лежит внутри v).
    let mut rng: u64 = 0x1A2B_3C4D_5E6F_7A8B;
    for _ in 0..800 {
        let len = in_range(&mut rng, 1, 50) as usize;
        let v = random_i64_vec(&mut rng, len, -10_000, 10_000);
        let result_ref = max_slice(&v).unwrap();
        let result_ptr = result_ref as *const i64;
        let base = v.as_ptr();
        // Указатель должен лежать внутри диапазона [base, base+len)
        let in_range_ptr = result_ptr >= base
            && result_ptr < unsafe { base.add(len) };
        assert!(
            in_range_ptr,
            "max_slice returned a pointer outside the slice"
        );
    }
}

#[test]
fn prop_max_slice_result_equals_value_at_its_address() {
    // Разыменование результата совпадает со значением, которое max_slice нашёл.
    let mut rng: u64 = 0xBEEF_DEAD_FEED_CAFE;
    for _ in 0..800 {
        let len = in_range(&mut rng, 1, 50) as usize;
        let v = random_i64_vec(&mut rng, len, -10_000, 10_000);
        let r = max_slice(&v).unwrap();
        // value через oracle
        let expected_val = *v.iter().max().unwrap();
        assert_eq!(*r, expected_val);
    }
}

// Явные граничные случаи, не покрытые lifetimes.rs
#[test]
fn max_slice_all_equal() {
    let v = vec![7_i64; 100];
    let r = max_slice(&v).unwrap();
    assert_eq!(*r, 7);
}

#[test]
fn max_slice_two_elements() {
    assert_eq!(max_slice(&[1_i64, 2]), Some(&2));
    assert_eq!(max_slice(&[2_i64, 1]), Some(&2));
}

#[test]
fn max_slice_all_negative() {
    let v: Vec<i64> = (-100..=-1).collect();
    assert_eq!(max_slice(&v), Some(&-1));
}
