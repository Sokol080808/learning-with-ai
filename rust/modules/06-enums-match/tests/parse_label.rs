// Тесты для задания 6: parse_label("name:count") -> Option<(String, u32)>.
// Закрепляет идиомы let-else и if let ... else.
//
// Два класса тестов:
//   (a) детерминированные примеры — конкретный вход → конкретный результат (контракт);
//   (b) рандомизированные property-тесты — инварианты + оракул, детерминированный xorshift64*.
//
// Каждый файл в tests/ — отдельный интеграционный крейт; импортируем ТОЛЬКО публичный API.
// На стабе с todo!() тесты падают (паника в рантайме); на эталоне — зелёные.

use m06_enums::parse_label;

// ── (a) Детерминированные примеры (= контракт из README) ─────────────────────

#[test]
fn parses_simple_label() {
    assert_eq!(parse_label("alpha:7"), Some(("alpha".to_string(), 7)));
}

#[test]
fn parses_zero_count() {
    assert_eq!(parse_label("x:0"), Some(("x".to_string(), 0)));
}

#[test]
fn parses_multi_digit_count() {
    assert_eq!(parse_label("session:4096"), Some(("session".to_string(), 4096)));
}

#[test]
fn missing_colon_is_none() {
    assert_eq!(parse_label("alpha"), None);
    assert_eq!(parse_label("noseparator"), None);
}

#[test]
fn non_numeric_suffix_is_none() {
    assert_eq!(parse_label("alpha:x"), None);
    assert_eq!(parse_label("alpha:7x"), None);
    assert_eq!(parse_label("alpha:"), None); // пустой суффикс не парсится в u32
}

#[test]
fn negative_suffix_is_none() {
    // u32 не принимает отрицательные → None
    assert_eq!(parse_label("k:-1"), None);
}

#[test]
fn float_suffix_is_none() {
    // "1.5" не парсится в u32 → None
    assert_eq!(parse_label("k:1.5"), None);
}

#[test]
fn empty_string_is_none() {
    assert_eq!(parse_label(""), None);
}

#[test]
fn empty_name_is_none() {
    // Контракт: пустое имя запрещено (решено и задокументировано).
    assert_eq!(parse_label(":7"), None);
}

#[test]
fn extra_colon_is_none() {
    // split_once режет по первому ':', суффикс "b:1" не парсится в u32 → None.
    assert_eq!(parse_label("a:b:1"), None);
}

#[test]
fn name_keeps_inner_non_colon_chars() {
    // Имя может содержать что угодно, кроме того, что делает суффикс непарсящимся.
    assert_eq!(
        parse_label("my-name_42:5"),
        Some(("my-name_42".to_string(), 5))
    );
}

// ── (b) Рандомизированные property-тесты ─────────────────────────────────────

// Детерминированный ГПСЧ xorshift64* — тот же алгоритм, что в props.rs/expr.rs.
fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}

// Целое в диапазоне [lo, hi] включительно.
fn in_range(state: &mut u64, lo: i64, hi: i64) -> i64 {
    let span = (hi - lo + 1) as u64;
    lo + (next_u64(state) % span) as i64
}

/// Round-trip: для непустого имени без двоеточия и валидного u32
/// parse_label("name:count") восстанавливает ровно (name, count).
#[test]
fn prop_round_trip_valid_labels() {
    let mut rng: u64 = 0x0666_DEAD_BEEF_1357;
    // алфавит для имени — без двоеточия, чтобы split_once резал предсказуемо
    let alphabet: &[u8] = b"abcdefghijklmnopqrstuvwxyz0123456789_-";
    for _ in 0..1000 {
        // имя длиной 1..=10, гарантированно непустое
        let name_len = in_range(&mut rng, 1, 10) as usize;
        let name: String = (0..name_len)
            .map(|_| {
                let idx = in_range(&mut rng, 0, alphabet.len() as i64 - 1) as usize;
                alphabet[idx] as char
            })
            .collect();
        let count = in_range(&mut rng, 0, 1_000_000) as u32;
        let input = format!("{name}:{count}");
        assert_eq!(
            parse_label(&input),
            Some((name.clone(), count)),
            "round-trip провалился для {input:?}"
        );
    }
}

/// Инвариант: если parse_label вернул Some((name, count)), то реконструкция
/// "name:count" разбирается обратно в ту же пару (идемпотентность разбора).
#[test]
fn prop_some_implies_reparse_stable() {
    let mut rng: u64 = 0x1A2B_3C4D_5E6F_0718;
    let alphabet: &[u8] = b"abcXYZ_0123456789:.-"; // включает ':' '.' '-' — часть входов невалидна
    for _ in 0..1000 {
        let len = in_range(&mut rng, 0, 14) as usize;
        let input: String = (0..len)
            .map(|_| {
                let idx = in_range(&mut rng, 0, alphabet.len() as i64 - 1) as usize;
                alphabet[idx] as char
            })
            .collect();
        if let Some((name, count)) = parse_label(&input) {
            // имя непустое (контракт)
            assert!(!name.is_empty(), "имя пустое для входа {input:?}");
            // реконструкция разбирается обратно в ту же пару
            let reconstructed = format!("{name}:{count}");
            assert_eq!(
                parse_label(&reconstructed),
                Some((name.clone(), count)),
                "повторный разбор {reconstructed:?} (из {input:?}) дал другой результат"
            );
        }
    }
}

/// Инвариант: нет двоеточия → всегда None (оракул: содержит ли строка ':').
#[test]
fn prop_no_colon_is_always_none() {
    let mut rng: u64 = 0xC0FF_EE00_1234_5678;
    let alphabet: &[u8] = b"abcdefABCDEF0123456789_-."; // намеренно БЕЗ ':'
    for _ in 0..1000 {
        let len = in_range(&mut rng, 0, 16) as usize;
        let input: String = (0..len)
            .map(|_| {
                let idx = in_range(&mut rng, 0, alphabet.len() as i64 - 1) as usize;
                alphabet[idx] as char
            })
            .collect();
        assert!(!input.contains(':'), "тест сам сгенерировал ':' — почини алфавит");
        assert_eq!(
            parse_label(&input),
            None,
            "строка без ':' должна давать None: {input:?}"
        );
    }
}

/// Инвариант: пустое имя (":count") → всегда None, для любого валидного count.
#[test]
fn prop_empty_name_is_always_none() {
    let mut rng: u64 = 0x9E37_79B9_7F4A_7C15;
    for _ in 0..1000 {
        let count = in_range(&mut rng, 0, 1_000_000) as u32;
        let input = format!(":{count}");
        assert_eq!(parse_label(&input), None, "пустое имя должно давать None: {input:?}");
    }
}

/// Оракул против std: parse_label согласуется с ручной эталонной реализацией
/// (split_once + проверка непустого имени + parse::<u32>).
#[test]
fn prop_agrees_with_reference_oracle() {
    let mut rng: u64 = 0xDEAD_C0DE_FACE_B00C;
    let alphabet: &[u8] = b"ab09:.-"; // компактный алфавит с ':' '.' '-' — много граничных случаев
    for _ in 0..2000 {
        let len = in_range(&mut rng, 0, 12) as usize;
        let input: String = (0..len)
            .map(|_| {
                let idx = in_range(&mut rng, 0, alphabet.len() as i64 - 1) as usize;
                alphabet[idx] as char
            })
            .collect();
        let oracle = oracle_parse(&input);
        assert_eq!(
            parse_label(&input),
            oracle,
            "parse_label({input:?}) разошёлся с оракулом"
        );
    }
}

// Независимая эталонная реализация для prop_agrees_with_reference_oracle.
fn oracle_parse(s: &str) -> Option<(String, u32)> {
    let (name, digits) = s.split_once(':')?;
    if name.is_empty() {
        return None;
    }
    match digits.parse::<u32>() {
        Ok(n) => Some((name.to_string(), n)),
        Err(_) => None,
    }
}
