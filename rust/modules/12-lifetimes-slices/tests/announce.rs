// Тесты для announce_longest() из модуля 12 — кульминация главы 10.3 The Rust Book:
// время жизни 'a И trait-ограничение T: Display в одной сигнатуре.
//
// Два уровня проверки:
//   (a) детерминированные примеры — конкретные входы → ожидаемые выходы + проверка,
//       что результат это суб-срез одного из входов (через as_ptr), а не копия;
//   (b) рандомизированные свойства — инварианты на большом пространстве входов
//       через детерминированный ГПСЧ (xorshift64*), без внешних крейтов.
// Компилируется и с todo!() в lib.rs (тесты «красные»), и с реализацией («зелёные»).

use m12_lifetimes::*;

use std::fmt;

// Свой тип, реализующий Display — чтобы доказать, что announcement принимает
// ЛЮБОЙ Display, а не только &str/числа.
struct Tag(u32);

impl fmt::Display for Tag {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "[tag #{}]", self.0)
    }
}

// ===========================================================================
// Часть A — детерминированные примеры
// ===========================================================================

// --- выбор более длинной строки ---

#[test]
fn announce_second_is_longer() {
    assert_eq!(announce_longest("hi", "hello", "!"), "hello");
}

#[test]
fn announce_first_is_longer() {
    assert_eq!(announce_longest("hello", "hi", "!"), "hello");
}

#[test]
fn announce_equal_length_returns_first() {
    // При равной длине контракт требует вернуть ПЕРВЫЙ аргумент.
    assert_eq!(announce_longest("ab", "cd", "note"), "ab");
}

#[test]
fn announce_with_empty() {
    assert_eq!(announce_longest("", "x", "n"), "x");
    assert_eq!(announce_longest("x", "", "n"), "x");
}

#[test]
fn announce_counts_bytes_not_chars() {
    // "кот" — 6 байт, "котик" — 10 байт. Длиннее "котик".
    assert_eq!(announce_longest("кот", "котик", "byte-check"), "котик");
}

// --- announcement может быть любого Display-типа ---

#[test]
fn announce_accepts_str_announcement() {
    let r = announce_longest("alpha", "beta", "I'm a &str");
    assert_eq!(r, "alpha");
}

#[test]
fn announce_accepts_integer_announcement() {
    // announcement: i32 — временное Copy-значение, не ссылка. Должно компилироваться:
    // именно поэтому announcement НЕ связан с 'a.
    let r = announce_longest("hello", "hi", 42_i32);
    assert_eq!(r, "hello");
}

#[test]
fn announce_accepts_custom_display_announcement() {
    // Свой тип с impl Display — доказывает, что ограничение именно T: Display.
    let r = announce_longest("xyz", "longer-one", Tag(7));
    assert_eq!(r, "longer-one");
}

// --- результат — суб-срез одного из входов, а не копия ---

#[test]
fn announce_result_borrows_first_input() {
    // Когда возвращается первый аргумент, его адрес совпадает с a.as_ptr().
    let a = String::from("looong");
    let b = String::from("short");
    let r = announce_longest(&a, &b, "ptr-check");
    assert_eq!(r, "looong");
    assert_eq!(r.as_ptr(), a.as_ptr());
}

#[test]
fn announce_result_borrows_second_input() {
    // Когда возвращается второй аргумент, его адрес совпадает с b.as_ptr().
    let a = String::from("short");
    let b = String::from("loooonger");
    let r = announce_longest(&a, &b, 0_i32);
    assert_eq!(r, "loooonger");
    assert_eq!(r.as_ptr(), b.as_ptr());
}

#[test]
fn announce_equal_length_borrows_first_pointer() {
    // При равной длине возвращается ПЕРВЫЙ — проверяем по адресу, а не только по значению.
    let a = String::from("ab");
    let b = String::from("cd");
    let r = announce_longest(&a, &b, Tag(1));
    assert_eq!(r.as_ptr(), a.as_ptr());
}

// ===========================================================================
// Часть B — рандомизированные свойства
// ===========================================================================

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

/// Случайная ASCII-строка из букв a–z заданной длины.
fn random_ascii_str(state: &mut u64, len: usize) -> String {
    const CHARS: &[u8] = b"abcdefghijklmnopqrstuvwxyz";
    (0..len)
        .map(|_| {
            let idx = (next_u64(state) % CHARS.len() as u64) as usize;
            CHARS[idx] as char
        })
        .collect()
}

#[test]
fn prop_announce_result_equals_one_of_inputs() {
    // Результат всегда совпадает с одним из двух входов (не третье значение,
    // не анонс) — announcement к результату отношения не имеет.
    let mut rng: u64 = 0x10DE_F00D_2024_1212;
    for _ in 0..600 {
        let la = in_range(&mut rng, 0, 25) as usize;
        let lb = in_range(&mut rng, 0, 25) as usize;
        let a = random_ascii_str(&mut rng, la);
        let b = random_ascii_str(&mut rng, lb);
        // announcement — случайное число; на результат влиять не должно.
        let ann = in_range(&mut rng, -1000, 1000);
        let r = announce_longest(&a, &b, ann);
        assert!(
            r == a.as_str() || r == b.as_str(),
            "announce_longest returned something other than one of the inputs"
        );
    }
}

#[test]
fn prop_announce_length_is_max_of_inputs() {
    // Длина результата всегда == max(a.len(), b.len()).
    let mut rng: u64 = 0x5EED_1234_ABCD_4321;
    for _ in 0..600 {
        let la = in_range(&mut rng, 0, 30) as usize;
        let lb = in_range(&mut rng, 0, 30) as usize;
        let a = random_ascii_str(&mut rng, la);
        let b = random_ascii_str(&mut rng, lb);
        let r = announce_longest(&a, &b, "ann");
        assert_eq!(
            r.len(),
            a.len().max(b.len()),
            "result length != max(a.len(), b.len()): a={a:?} b={b:?}"
        );
    }
}

#[test]
fn prop_announce_matches_longest_oracle() {
    // Оракул — уже проверенная функция longest(): тот же контракт выбора.
    // announce_longest отличается только печатью анонса, который не влияет на выбор.
    let mut rng: u64 = 0xC0DE_CAFE_BEEF_0007;
    for _ in 0..600 {
        let la = in_range(&mut rng, 0, 25) as usize;
        let lb = in_range(&mut rng, 0, 25) as usize;
        let a = random_ascii_str(&mut rng, la);
        let b = random_ascii_str(&mut rng, lb);
        let expected = longest(&a, &b);
        let got = announce_longest(&a, &b, Tag(la as u32));
        // Совпадение и по значению, и по адресу — это ОДНА И ТА ЖЕ ссылка-вход.
        assert_eq!(got, expected, "announce disagreed with longest oracle (value)");
        assert_eq!(
            got.as_ptr(),
            expected.as_ptr(),
            "announce disagreed with longest oracle (pointer)"
        );
    }
}

#[test]
fn prop_announce_result_borrows_a_chosen_input() {
    // Адрес результата всегда совпадает с адресом одного из входов (суб-срез,
    // не копия): либо a.as_ptr(), либо b.as_ptr().
    let mut rng: u64 = 0xABAD_1DEA_FACE_B00C;
    for _ in 0..600 {
        let la = in_range(&mut rng, 1, 25) as usize; // непустые, чтобы адреса были осмысленны
        let lb = in_range(&mut rng, 1, 25) as usize;
        let a = random_ascii_str(&mut rng, la);
        let b = random_ascii_str(&mut rng, lb);
        let r = announce_longest(&a, &b, "x");
        let rp = r.as_ptr();
        assert!(
            rp == a.as_ptr() || rp == b.as_ptr(),
            "result pointer matches neither input — looks like a copy, not a sub-slice"
        );
    }
}

#[test]
fn prop_announce_equal_length_returns_first() {
    // Когда оба входа одинаковой длины, должен вернуться ПЕРВЫЙ аргумент (по адресу).
    let mut rng: u64 = 0x7777_8888_9999_AAAA;
    for _ in 0..600 {
        let len = in_range(&mut rng, 1, 25) as usize;
        let a = random_ascii_str(&mut rng, len);
        let b = random_ascii_str(&mut rng, len);
        let r = announce_longest(&a, &b, 0_i32);
        assert_eq!(
            r.as_ptr(),
            a.as_ptr(),
            "equal-length: expected first argument to be returned"
        );
    }
}
