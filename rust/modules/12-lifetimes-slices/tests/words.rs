// Тесты для функций words() и longest_word() из модуля 12.
// Два уровня проверки:
//   (a) детерминированные примеры — конкретные входы → ожидаемые выходы;
//   (b) рандомизированные свойства — инварианты на большом пространстве входов.
// Компилируется и с todo!() в lib.rs (тесты «красные»), и с реализацией («зелёные»).

use m12_lifetimes::*;

// ===========================================================================
// Часть A — детерминированные примеры
// ===========================================================================

// --- words: базовые случаи ---

#[test]
fn words_empty_string() {
    let result = words("");
    assert!(result.is_empty(), "пустая строка должна давать пустой вектор");
}

#[test]
fn words_only_spaces() {
    let result = words("   ");
    assert!(result.is_empty(), "строка из пробелов — ноль слов");
}

#[test]
fn words_single_word_no_spaces() {
    assert_eq!(words("rust"), vec!["rust"]);
}

#[test]
fn words_two_words() {
    assert_eq!(words("hello world"), vec!["hello", "world"]);
}

#[test]
fn words_leading_trailing_spaces() {
    assert_eq!(words("  hello  "), vec!["hello"]);
}

#[test]
fn words_multiple_spaces_between() {
    assert_eq!(words("a   b"), vec!["a", "b"]);
}

#[test]
fn words_three_words() {
    assert_eq!(words("one two three"), vec!["one", "two", "three"]);
}

#[test]
fn words_tab_and_newline_are_whitespace() {
    // split_whitespace разбивает по любому пробельному символу
    assert_eq!(words("a\tb\nc"), vec!["a", "b", "c"]);
}

// --- words: проверка суб-срезов через указатели ---

#[test]
fn words_each_element_is_subslice_of_input() {
    let s = String::from("hello world rust");
    let base = s.as_ptr() as usize;
    let end = base + s.len();
    let ws = words(&s);
    for w in &ws {
        let ptr = w.as_ptr() as usize;
        assert!(
            ptr >= base && ptr + w.len() <= end,
            "слово {:?} не является суб-срезом исходной строки (ptr={ptr:#x}, диапазон={base:#x}..{end:#x})",
            w
        );
    }
}

#[test]
fn words_first_word_starts_at_same_ptr_as_input_when_no_leading_space() {
    let s = String::from("alpha beta");
    let ws = words(&s);
    assert!(!ws.is_empty());
    assert_eq!(
        ws[0].as_ptr(),
        s.as_ptr(),
        "первое слово без ведущих пробелов должно начинаться с того же адреса"
    );
}

#[test]
fn words_no_element_contains_whitespace() {
    let s = "hello world  rust  lang";
    let ws = words(s);
    for w in &ws {
        assert!(
            !w.contains(char::is_whitespace),
            "слово {:?} содержит пробельный символ",
            w
        );
    }
}

#[test]
fn words_count_matches_split_whitespace() {
    let cases = [
        "",
        "   ",
        "a",
        "hello world",
        "  leading and trailing  ",
        "one  two  three",
    ];
    for s in cases {
        let expected: Vec<&str> = s.split_whitespace().collect();
        assert_eq!(
            words(s),
            expected,
            "words({:?}) не совпадает с oracle split_whitespace",
            s
        );
    }
}

// --- longest_word: базовые случаи ---

#[test]
fn longest_word_empty_string() {
    assert_eq!(longest_word(""), None);
}

#[test]
fn longest_word_only_spaces() {
    assert_eq!(longest_word("   "), None);
}

#[test]
fn longest_word_single_word() {
    assert_eq!(longest_word("rust"), Some("rust"));
}

#[test]
fn longest_word_two_words_second_longer() {
    assert_eq!(longest_word("hi hello"), Some("hello"));
}

#[test]
fn longest_word_two_words_first_longer() {
    assert_eq!(longest_word("hello hi"), Some("hello"));
}

#[test]
fn longest_word_equal_length_returns_first() {
    // "ab" и "cd" — одинаковой длины, должно вернуться первое
    assert_eq!(longest_word("ab cd"), Some("ab"));
}

#[test]
fn longest_word_three_words() {
    assert_eq!(longest_word("one two three"), Some("three"));
}

#[test]
fn longest_word_all_equal_length_returns_first() {
    assert_eq!(longest_word("aa bb cc"), Some("aa"));
}

#[test]
fn longest_word_leading_trailing_spaces() {
    assert_eq!(longest_word("  hello  "), Some("hello"));
}

// --- longest_word: проверка суб-среза через указатели ---

#[test]
fn longest_word_result_is_subslice_of_input() {
    let s = String::from("hello magnificent world");
    let base = s.as_ptr() as usize;
    let end = base + s.len();
    if let Some(w) = longest_word(&s) {
        let ptr = w.as_ptr() as usize;
        assert!(
            ptr >= base && ptr + w.len() <= end,
            "longest_word вернул срез за пределами исходной строки"
        );
        assert_eq!(w, "magnificent");
    } else {
        panic!("ожидали Some, получили None");
    }
}

#[test]
fn longest_word_result_ptr_points_into_input() {
    let s = String::from("abc defghi jk");
    let base = s.as_ptr() as usize;
    let end = base + s.len();
    let w = longest_word(&s).unwrap();
    let ptr = w.as_ptr() as usize;
    assert!(ptr >= base && ptr + w.len() <= end);
    assert_eq!(w, "defghi");
}

// ===========================================================================
// Часть B — рандомизированные свойства
// Детерминированный ГПСЧ (xorshift64*) — без внешних крейтов, воспроизводимо.
// ===========================================================================

fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}

fn in_range(state: &mut u64, lo: usize, hi: usize) -> usize {
    let span = (hi - lo + 1) as u64;
    lo + (next_u64(state) % span) as usize
}

/// Генерирует случайную строку из ASCII букв (a–z) и пробелов.
fn random_ascii_str(state: &mut u64, len: usize) -> String {
    const CHARS: &[u8] = b"abcdefghijklmnopqrstuvwxyz ";
    (0..len)
        .map(|_| {
            let idx = (next_u64(state) % CHARS.len() as u64) as usize;
            CHARS[idx] as char
        })
        .collect()
}

// ---------------------------------------------------------------------------
// Свойства words()
// ---------------------------------------------------------------------------

#[test]
fn prop_words_each_element_is_subslice_of_input() {
    // Каждое слово является суб-срезом входной строки (проверка через указатели).
    let mut rng: u64 = 0xDEAD_BEEF_CAFE_1234;
    for _ in 0..800 {
        let len = in_range(&mut rng, 0, 60);
        let s = random_ascii_str(&mut rng, len);
        let base = s.as_ptr() as usize;
        let end = base + s.len();
        for w in words(&s) {
            let ptr = w.as_ptr() as usize;
            assert!(
                ptr >= base && ptr + w.len() <= end,
                "words: слово {:?} выходит за пределы строки {:?}",
                w,
                s
            );
        }
    }
}

#[test]
fn prop_words_no_element_contains_whitespace() {
    // Ни одно слово не содержит пробельного символа.
    let mut rng: u64 = 0x1111_AAAA_2222_BBBB;
    for _ in 0..800 {
        let len = in_range(&mut rng, 0, 60);
        let s = random_ascii_str(&mut rng, len);
        for w in words(&s) {
            assert!(
                !w.contains(char::is_whitespace),
                "words: слово {:?} содержит пробел в строке {:?}",
                w,
                s
            );
        }
    }
}

#[test]
fn prop_words_agrees_with_oracle_split_whitespace() {
    // words(s) == s.split_whitespace().collect::<Vec<_>>() для любой строки.
    let mut rng: u64 = 0xC0FF_EE11_3344_5566;
    for _ in 0..1000 {
        let len = in_range(&mut rng, 0, 60);
        let s = random_ascii_str(&mut rng, len);
        let expected: Vec<&str> = s.split_whitespace().collect();
        let got = words(&s);
        assert_eq!(
            got,
            expected,
            "words не совпадает с oracle для {:?}",
            s
        );
    }
}

#[test]
fn prop_words_count_equals_split_whitespace_count() {
    // Количество слов совпадает с oracle.
    let mut rng: u64 = 0xFACE_B00C_FEED_0001;
    for _ in 0..800 {
        let len = in_range(&mut rng, 0, 60);
        let s = random_ascii_str(&mut rng, len);
        let expected = s.split_whitespace().count();
        let got = words(&s).len();
        assert_eq!(got, expected, "количество слов не совпадает для {:?}", s);
    }
}

#[test]
fn prop_words_empty_iff_no_non_whitespace() {
    // words(s) пуст тогда и только тогда, когда s не содержит не-пробельных символов.
    let mut rng: u64 = 0x9876_5432_10FE_DCBA;
    for _ in 0..800 {
        let len = in_range(&mut rng, 0, 60);
        let s = random_ascii_str(&mut rng, len);
        let has_word_chars = s.chars().any(|c| !c.is_whitespace());
        let result_empty = words(&s).is_empty();
        assert_eq!(
            result_empty,
            !has_word_chars,
            "words пуст: {result_empty}, но строка содержит не-пробелы: {has_word_chars} для {:?}",
            s
        );
    }
}

// ---------------------------------------------------------------------------
// Свойства longest_word()
// ---------------------------------------------------------------------------

#[test]
fn prop_longest_word_none_iff_no_words() {
    // longest_word возвращает None тогда и только тогда, когда words() пуст.
    let mut rng: u64 = 0xAAAA_BBBB_CCCC_DDDD;
    for _ in 0..800 {
        let len = in_range(&mut rng, 0, 60);
        let s = random_ascii_str(&mut rng, len);
        let no_words = words(&s).is_empty();
        let is_none = longest_word(&s).is_none();
        assert_eq!(
            is_none,
            no_words,
            "longest_word None: {is_none}, words пуст: {no_words} для {:?}",
            s
        );
    }
}

#[test]
fn prop_longest_word_is_member_of_words() {
    // Результат longest_word всегда является одним из элементов words().
    let mut rng: u64 = 0x1234_5678_9ABC_DEF0;
    for _ in 0..800 {
        let len = in_range(&mut rng, 0, 60);
        let s = random_ascii_str(&mut rng, len);
        if let Some(lw) = longest_word(&s) {
            let ws = words(&s);
            assert!(
                ws.contains(&lw),
                "longest_word {:?} не является словом в {:?}",
                lw,
                s
            );
        }
    }
}

#[test]
fn prop_longest_word_len_ge_all_words() {
    // Длина результата не меньше длины любого слова.
    let mut rng: u64 = 0x0F0F_F0F0_0F0F_F0F0;
    for _ in 0..800 {
        let len = in_range(&mut rng, 1, 60);
        let s = random_ascii_str(&mut rng, len);
        if let Some(lw) = longest_word(&s) {
            for w in words(&s) {
                assert!(
                    lw.len() >= w.len(),
                    "longest_word {:?} (len={}) короче слова {:?} (len={}) в {:?}",
                    lw,
                    lw.len(),
                    w,
                    w.len(),
                    s
                );
            }
        }
    }
}

#[test]
fn prop_longest_word_returns_first_on_tie() {
    // При равной длине возвращается первое слово.
    let mut rng: u64 = 0xBEEF_CAFE_1111_2222;
    for _ in 0..800 {
        let len = in_range(&mut rng, 1, 60);
        let s = random_ascii_str(&mut rng, len);
        let ws = words(&s);
        if ws.is_empty() {
            continue;
        }
        let max_len = ws.iter().map(|w| w.len()).max().unwrap();
        // Первое слово максимальной длины по oracle
        let oracle_first = ws.iter().find(|w| w.len() == max_len).copied();
        let got = longest_word(&s);
        assert_eq!(
            got,
            oracle_first,
            "longest_word не вернул первое из самых длинных слов для {:?}",
            s
        );
    }
}

#[test]
fn prop_longest_word_result_is_subslice_of_input() {
    // Результат является суб-срезом входной строки (проверка адреса).
    let mut rng: u64 = 0x5A5A_5A5A_A5A5_A5A5;
    for _ in 0..800 {
        let len = in_range(&mut rng, 0, 60);
        let s = random_ascii_str(&mut rng, len);
        if let Some(lw) = longest_word(&s) {
            let base = s.as_ptr() as usize;
            let end = base + s.len();
            let ptr = lw.as_ptr() as usize;
            assert!(
                ptr >= base && ptr + lw.len() <= end,
                "longest_word вернул срез за пределами {:?}",
                s
            );
        }
    }
}

#[test]
fn prop_longest_word_agrees_with_oracle() {
    // Oracle: words().iter().max_by_key(|w| w.len()), tie-break — первый.
    let mut rng: u64 = 0xDEAD_C0DE_DEAD_C0DE;
    for _ in 0..1000 {
        let len = in_range(&mut rng, 0, 60);
        let s = random_ascii_str(&mut rng, len);
        let ws = words(&s);
        let oracle: Option<&str> = if ws.is_empty() {
            None
        } else {
            let max_len = ws.iter().map(|w| w.len()).max().unwrap();
            ws.iter().find(|w| w.len() == max_len).copied()
        };
        assert_eq!(
            longest_word(&s),
            oracle,
            "longest_word не совпадает с oracle для {:?}",
            s
        );
    }
}
