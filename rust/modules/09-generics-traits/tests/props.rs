// Рандомизированные property-тесты для модуля 09 (generics & traits).
// Используют детерминированный xorshift64* — без внешних крейтов, воспроизводимо.

use m09_generics::*;

// ---------------------------------------------------------------------------
// Детерминированный ГПСЧ (xorshift64*)
// ---------------------------------------------------------------------------

// маленький детерминированный ГПСЧ (xorshift64*), без внешних крейтов — воспроизводимо.
// Используй ЭТОТ helper во всех property-тестах модуля. Сид ФИКСИРОВАННЫЙ (hardcode), чтобы прогон был детерминированным.
fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}
// целое в диапазоне [lo, hi] включительно (держи диапазон скромным, чтобы оракул не переполнял i64)
fn in_range(state: &mut u64, lo: i64, hi: i64) -> i64 {
    let span = (hi - lo + 1) as u64;
    lo + (next_u64(state) % span) as i64
}

// ---------------------------------------------------------------------------
// my_max — property tests
// ---------------------------------------------------------------------------

/// Инвариант: my_max(a, b) == std::cmp::max(a, b) для всех i32.
#[test]
fn prop_my_max_agrees_with_std_max() {
    let mut rng: u64 = 0xDEAD_BEEF_1337_CAFE;
    for _ in 0..1000 {
        let a = in_range(&mut rng, -10_000, 10_000) as i32;
        let b = in_range(&mut rng, -10_000, 10_000) as i32;
        assert_eq!(
            my_max(a, b),
            std::cmp::max(a, b),
            "my_max({a}, {b}) should equal std::cmp::max({a}, {b})"
        );
    }
}

/// Инвариант: my_max(a, b) >= a  И  my_max(a, b) >= b.
#[test]
fn prop_my_max_is_at_least_both_inputs() {
    let mut rng: u64 = 0xCAFE_BABE_0000_0001;
    for _ in 0..1000 {
        let a = in_range(&mut rng, -10_000, 10_000) as i32;
        let b = in_range(&mut rng, -10_000, 10_000) as i32;
        let m = my_max(a, b);
        assert!(m >= a, "my_max({a},{b})={m} < a={a}");
        assert!(m >= b, "my_max({a},{b})={m} < b={b}");
    }
}

/// Инвариант: my_max(a, b) == один из двух входных значений (не третье).
#[test]
fn prop_my_max_returns_one_of_inputs() {
    let mut rng: u64 = 0x1111_2222_3333_4444;
    for _ in 0..1000 {
        let a = in_range(&mut rng, -10_000, 10_000) as i32;
        let b = in_range(&mut rng, -10_000, 10_000) as i32;
        let m = my_max(a, b);
        assert!(
            m == a || m == b,
            "my_max({a},{b})={m} — это не a и не b!"
        );
    }
}

/// Коммутативность: my_max(a, b) == my_max(b, a) для i32.
#[test]
fn prop_my_max_commutative() {
    let mut rng: u64 = 0xABCD_EF01_2345_6789;
    for _ in 0..1000 {
        let a = in_range(&mut rng, -10_000, 10_000) as i32;
        let b = in_range(&mut rng, -10_000, 10_000) as i32;
        assert_eq!(
            my_max(a, b),
            my_max(b, a),
            "my_max should be commutative: a={a}, b={b}"
        );
    }
}

/// Граничный случай: при a == b возвращается то же значение.
#[test]
fn prop_my_max_equal_inputs() {
    let mut rng: u64 = 0xFEED_FACE_DEAD_BEEF;
    for _ in 0..500 {
        let v = in_range(&mut rng, -10_000, 10_000) as i32;
        assert_eq!(my_max(v, v), v);
    }
}

/// my_max работает для i64 с отрицательными значениями.
#[test]
fn prop_my_max_negative_i64() {
    let mut rng: u64 = 0x0102_0304_0506_0708;
    for _ in 0..1000 {
        let a = in_range(&mut rng, -10_000, 0);
        let b = in_range(&mut rng, -10_000, 0);
        let m = my_max(a, b);
        assert!(m >= a && m >= b);
        assert!(m == a || m == b);
    }
}

// ---------------------------------------------------------------------------
// largest — property tests
// ---------------------------------------------------------------------------

/// Оракул: largest(&v) совпадает с наивным поиском максимума через итератор.
#[test]
fn prop_largest_agrees_with_naive_max() {
    let mut rng: u64 = 0x9999_AAAA_BBBB_CCCC;
    for _ in 0..500 {
        // длина среза — от 1 до 20 элементов
        let len = (in_range(&mut rng, 1, 20)) as usize;
        let v: Vec<i32> = (0..len)
            .map(|_| in_range(&mut rng, -10_000, 10_000) as i32)
            .collect();
        let expected = *v.iter().max().unwrap();
        assert_eq!(
            largest(&v),
            expected,
            "largest mismatch for {:?}",
            v
        );
    }
}

/// Инвариант: largest(&v) >= каждого элемента среза.
#[test]
fn prop_largest_is_upper_bound() {
    let mut rng: u64 = 0xDECA_FBAD_1234_5678;
    for _ in 0..500 {
        let len = (in_range(&mut rng, 1, 30)) as usize;
        let v: Vec<i32> = (0..len)
            .map(|_| in_range(&mut rng, -10_000, 10_000) as i32)
            .collect();
        let m = largest(&v);
        for &x in &v {
            assert!(m >= x, "largest={m} < element={x} in {:?}", v);
        }
    }
}

/// Инвариант: largest(&v) — это элемент, реально присутствующий в срезе.
#[test]
fn prop_largest_is_contained_in_slice() {
    let mut rng: u64 = 0x5A5A_5A5A_5A5A_5A5A;
    for _ in 0..500 {
        let len = (in_range(&mut rng, 1, 25)) as usize;
        let v: Vec<i32> = (0..len)
            .map(|_| in_range(&mut rng, -10_000, 10_000) as i32)
            .collect();
        let m = largest(&v);
        assert!(
            v.contains(&m),
            "largest={m} не содержится в {:?}",
            v
        );
    }
}

/// Граничный случай: срез из одного элемента — largest возвращает этот элемент.
#[test]
fn prop_largest_single_element_roundtrip() {
    let mut rng: u64 = 0xBEEF_1234_5678_9ABC;
    for _ in 0..500 {
        let v = in_range(&mut rng, -10_000, 10_000) as i32;
        assert_eq!(largest(&[v]), v);
    }
}

/// Largest работает с отрицательными числами (все элементы < 0).
#[test]
fn prop_largest_all_negative() {
    let mut rng: u64 = 0xF0F0_F0F0_0F0F_0F0F;
    for _ in 0..500 {
        let len = (in_range(&mut rng, 1, 20)) as usize;
        let v: Vec<i32> = (0..len)
            .map(|_| in_range(&mut rng, -10_000, -1) as i32)
            .collect();
        let m = largest(&v);
        let expected = *v.iter().max().unwrap();
        assert_eq!(m, expected, "all-negative slice {:?}", v);
    }
}

/// Largest работает с дублирующимися значениями.
#[test]
fn prop_largest_with_duplicates() {
    let mut rng: u64 = 0x1A2B_3C4D_5E6F_7A8B;
    for _ in 0..500 {
        // генерируем маленький набор уникальных значений, потом повторяем их
        let unique = [
            in_range(&mut rng, -100, 100) as i32,
            in_range(&mut rng, -100, 100) as i32,
            in_range(&mut rng, -100, 100) as i32,
        ];
        let len = (in_range(&mut rng, 2, 15)) as usize;
        let v: Vec<i32> = (0..len)
            .map(|i| unique[i % unique.len()])
            .collect();
        let m = largest(&v);
        let expected = *v.iter().max().unwrap();
        assert_eq!(m, expected);
    }
}

// ---------------------------------------------------------------------------
// Article / Summary trait — property tests
// ---------------------------------------------------------------------------

/// Round-trip: Article::summarize() возвращает ровно тот title, с которым создан.
#[test]
fn prop_article_summarize_is_title() {
    // Генерируем "заголовки" как строки с числовым суффиксом, без вызова API в compile-time.
    let mut rng: u64 = 0x7777_8888_9999_AAAA;
    for _ in 0..500 {
        let n = in_range(&mut rng, 0, 999_999);
        let title = format!("Статья номер {n}");
        let article = Article { title: title.clone() };
        assert_eq!(
            article.summarize(),
            title,
            "summarize() должен вернуть title"
        );
    }
}

/// Инвариант: Article::preview() всегда начинается с "Читать дальше: ".
#[test]
fn prop_article_preview_prefix() {
    let mut rng: u64 = 0xBBBB_CCCC_DDDD_EEEE;
    for _ in 0..500 {
        let n = in_range(&mut rng, 0, 999_999);
        let title = format!("Новость {n}");
        let article = Article { title };
        let preview = article.preview();
        assert!(
            preview.starts_with("Читать дальше: "),
            "preview() не начинается с 'Читать дальше: ': {:?}",
            preview
        );
    }
}

/// Инвариант: Article::preview() == "Читать дальше: " + summarize().
#[test]
fn prop_article_preview_equals_prefix_plus_summarize() {
    let mut rng: u64 = 0x0001_0002_0003_0004;
    for _ in 0..500 {
        let n = in_range(&mut rng, 0, 999_999);
        let title = format!("Тема выпуска {n}");
        let article = Article { title };
        let expected = format!("Читать дальше: {}", article.summarize());
        assert_eq!(
            article.preview(),
            expected,
            "preview() не совпадает с ожидаемым форматом"
        );
    }
}

/// Граничный случай: пустой заголовок.
#[test]
fn prop_article_empty_title() {
    let article = Article { title: String::new() };
    assert_eq!(article.summarize(), "");
    assert_eq!(article.preview(), "Читать дальше: ");
}

/// Граничный случай: очень длинный заголовок (1000 символов).
#[test]
fn prop_article_long_title() {
    let title: String = "А".repeat(1000);
    let article = Article { title: title.clone() };
    assert_eq!(article.summarize(), title);
    assert_eq!(article.preview(), format!("Читать дальше: {title}"));
}
