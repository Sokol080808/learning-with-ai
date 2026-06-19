// Тесты для run_length_encode (задание 6 модуля 02).
// Содержит:
//   (a) детерминированные примеры — конкретные входы и ожидаемые выходы;
//   (b) рандомизированные property-тесты с xorshift64* ГПСЧ (фикс. сид, без крейтов).

use m02_control_flow::run_length_encode;

// ============================================================================
// (a) Детерминированные примеры
// ============================================================================

#[test]
fn rle_example_from_spec() {
    assert_eq!(
        run_length_encode("aabccc"),
        vec![('a', 2), ('b', 1), ('c', 3)]
    );
}

#[test]
fn rle_empty_string() {
    let result = run_length_encode("");
    assert!(result.is_empty(), "пустая строка должна давать пустой вектор");
}

#[test]
fn rle_single_char() {
    assert_eq!(run_length_encode("x"), vec![('x', 1)]);
}

#[test]
fn rle_all_same() {
    assert_eq!(run_length_encode("aaaa"), vec![('a', 4)]);
}

#[test]
fn rle_all_distinct() {
    assert_eq!(
        run_length_encode("abcd"),
        vec![('a', 1), ('b', 1), ('c', 1), ('d', 1)]
    );
}

#[test]
fn rle_alternating() {
    // Каждый символ — отдельная серия длиной 1
    assert_eq!(
        run_length_encode("ababab"),
        vec![('a', 1), ('b', 1), ('a', 1), ('b', 1), ('a', 1), ('b', 1)]
    );
}

#[test]
fn rle_two_runs() {
    assert_eq!(
        run_length_encode("aaabbb"),
        vec![('a', 3), ('b', 3)]
    );
}

#[test]
fn rle_unicode_chars() {
    // char в Rust — полноценный Unicode scalar value
    assert_eq!(
        run_length_encode("ааб"),
        vec![('а', 2), ('б', 1)]
    );
}

#[test]
fn rle_long_run_at_end() {
    // Последняя серия должна быть сброшена корректно
    assert_eq!(
        run_length_encode("abccccc"),
        vec![('a', 1), ('b', 1), ('c', 5)]
    );
}

#[test]
fn rle_two_char_string_same() {
    assert_eq!(run_length_encode("zz"), vec![('z', 2)]);
}

#[test]
fn rle_two_char_string_diff() {
    assert_eq!(run_length_encode("ab"), vec![('a', 1), ('b', 1)]);
}

#[test]
fn rle_reconstruction_spec_example() {
    // Проверяем: развёртка результата совпадает с исходной строкой
    let s = "aabccc";
    let encoded = run_length_encode(s);
    let decoded: String = encoded.iter().flat_map(|(c, n)| std::iter::repeat(*c).take(*n)).collect();
    assert_eq!(decoded, s);
}

#[test]
fn rle_run_counts_positive() {
    // Ни одна серия не может иметь длину 0
    for s in ["a", "aab", "abccc", "xxxxxyyyz"] {
        for (_, count) in run_length_encode(s) {
            assert!(count >= 1, "серия в '{s}' имеет длину 0");
        }
    }
}

// ============================================================================
// Вспомогательный детерминированный ГПСЧ (xorshift64*) — тот же что в props.rs
// ============================================================================

fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}

fn in_range_usize(state: &mut u64, lo: usize, hi: usize) -> usize {
    let span = (hi - lo + 1) as u64;
    lo + (next_u64(state) % span) as usize
}

/// Генерирует случайную строку из `len` символов алфавита ['a'..='e'].
/// Маленький алфавит гарантирует частые серии, которые интересны для RLE.
fn random_string(state: &mut u64, len: usize) -> String {
    (0..len)
        .map(|_| {
            let idx = in_range_usize(state, 0, 4);
            (b'a' + idx as u8) as char
        })
        .collect()
}

// ============================================================================
// (b) Рандомизированные property-тесты
// ============================================================================

/// СВОЙСТВО 1: развёртка результата всегда восстанавливает исходную строку.
#[test]
fn prop_rle_roundtrip() {
    let mut rng: u64 = 0xC0DE_CAFE_1234_5678;
    for _ in 0..1000 {
        let len = in_range_usize(&mut rng, 0, 50);
        let s = random_string(&mut rng, len);
        let encoded = run_length_encode(&s);
        let decoded: String = encoded
            .iter()
            .flat_map(|(c, n)| std::iter::repeat(*c).take(*n))
            .collect();
        assert_eq!(decoded, s, "round-trip failed for '{s}'");
    }
}

/// СВОЙСТВО 2: каждая серия имеет длину >= 1.
#[test]
fn prop_rle_counts_positive() {
    let mut rng: u64 = 0xDEAD_BEEF_4321_ABCD;
    for _ in 0..1000 {
        let len = in_range_usize(&mut rng, 0, 50);
        let s = random_string(&mut rng, len);
        for (_, count) in run_length_encode(&s) {
            assert!(count >= 1, "нулевая серия в '{s}'");
        }
    }
}

/// СВОЙСТВО 3: соседние серии имеют разные символы (иначе их не сжали бы в одну).
#[test]
fn prop_rle_adjacent_chars_differ() {
    let mut rng: u64 = 0xFEED_FACE_DEAD_BEEF;
    for _ in 0..1000 {
        let len = in_range_usize(&mut rng, 0, 50);
        let s = random_string(&mut rng, len);
        let encoded = run_length_encode(&s);
        for window in encoded.windows(2) {
            let (c1, _) = window[0];
            let (c2, _) = window[1];
            assert_ne!(
                c1, c2,
                "соседние серии имеют одинаковый символ '{c1}' в '{s}'"
            );
        }
    }
}

/// СВОЙСТВО 4: сумма длин серий равна длине исходной строки (в char, не bytes).
#[test]
fn prop_rle_total_length_equals_char_count() {
    let mut rng: u64 = 0x1111_AAAA_2222_BBBB;
    for _ in 0..1000 {
        let len = in_range_usize(&mut rng, 0, 60);
        let s = random_string(&mut rng, len);
        let encoded = run_length_encode(&s);
        let total: usize = encoded.iter().map(|(_, n)| n).sum();
        assert_eq!(
            total,
            s.chars().count(),
            "сумма длин серий != len строки для '{s}'"
        );
    }
}

/// СВОЙСТВО 5: сравнение с наивным оракулом — прямая группировка в цикле.
#[test]
fn prop_rle_vs_naive_oracle() {
    /// Наивная реализация RLE для верификации (независимый алгоритм).
    fn naive_rle(s: &str) -> Vec<(char, usize)> {
        let chars: Vec<char> = s.chars().collect();
        let mut out = Vec::new();
        let mut i = 0;
        while i < chars.len() {
            let c = chars[i];
            let mut j = i + 1;
            while j < chars.len() && chars[j] == c {
                j += 1;
            }
            out.push((c, j - i));
            i = j;
        }
        out
    }

    let mut rng: u64 = 0x9876_FEDC_5432_BA10;
    for _ in 0..1000 {
        let len = in_range_usize(&mut rng, 0, 50);
        let s = random_string(&mut rng, len);
        assert_eq!(
            run_length_encode(&s),
            naive_rle(&s),
            "расхождение с оракулом для '{s}'"
        );
    }
}

/// СВОЙСТВО 6: пустая строка всегда даёт пустой вектор (граничный случай, ~500 проходов).
#[test]
fn prop_rle_empty_always_empty() {
    for _ in 0..500 {
        assert!(run_length_encode("").is_empty());
    }
}

/// СВОЙСТВО 7: строка из одного символа, повторённого n раз → одна серия длиной n.
#[test]
fn prop_rle_single_run_string() {
    let mut rng: u64 = 0xA5A5_A5A5_5A5A_5A5A;
    for _ in 0..500 {
        let len = in_range_usize(&mut rng, 1, 40);
        let c_idx = in_range_usize(&mut rng, 0, 4);
        let c = (b'a' + c_idx as u8) as char;
        let s: String = std::iter::repeat(c).take(len).collect();
        let encoded = run_length_encode(&s);
        assert_eq!(encoded, vec![(c, len)], "строка '{s}' должна давать одну серию");
    }
}
