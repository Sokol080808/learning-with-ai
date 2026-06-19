// Рандомизированные property-тесты для модуля 03-ownership.
// Используют детерминированный ГПСЧ без внешних крейтов — прогон полностью воспроизводим.

use m03_ownership::*;

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

// ---------- sum_vec properties ----------

/// sum_vec всегда равен std-итераторному .sum() для тех же данных (оракул).
#[test]
fn prop_sum_vec_matches_std_sum() {
    let mut rng: u64 = 0xDEAD_BEEF_1234_5678;
    for _ in 0..500 {
        let len = in_range(&mut rng, 0, 20) as usize;
        let v: Vec<i64> = (0..len).map(|_| in_range(&mut rng, -1000, 1000)).collect();
        let expected: i64 = v.iter().sum();
        assert_eq!(sum_vec(v), expected);
    }
}

/// sum_vec для вектора из нулей всегда 0.
#[test]
fn prop_sum_vec_all_zeros_is_zero() {
    let mut rng: u64 = 0xABCD_EF01_2345_6789;
    for _ in 0..300 {
        let len = in_range(&mut rng, 0, 50) as usize;
        let v = vec![0i64; len];
        assert_eq!(sum_vec(v), 0);
    }
}

/// sum_vec([x]) == x для любого x — одноэлементный вектор.
#[test]
fn prop_sum_vec_single_element() {
    let mut rng: u64 = 0x1122_3344_5566_7788;
    for _ in 0..500 {
        let x = in_range(&mut rng, -10_000, 10_000);
        assert_eq!(sum_vec(vec![x]), x);
    }
}

/// sum_vec(v) + sum_vec(w) == sum_vec(concat(v, w)) — аддитивность.
#[test]
fn prop_sum_vec_additive() {
    let mut rng: u64 = 0xCAFE_BABE_DEAD_0001;
    for _ in 0..500 {
        let len_v = in_range(&mut rng, 0, 15) as usize;
        let len_w = in_range(&mut rng, 0, 15) as usize;
        let v: Vec<i64> = (0..len_v).map(|_| in_range(&mut rng, -1000, 1000)).collect();
        let w: Vec<i64> = (0..len_w).map(|_| in_range(&mut rng, -1000, 1000)).collect();
        let sum_v: i64 = v.iter().sum();
        let sum_w: i64 = w.iter().sum();
        let combined: Vec<i64> = v.iter().chain(w.iter()).copied().collect();
        assert_eq!(sum_vec(combined), sum_v + sum_w);
    }
}

// ---------- concat_strings properties ----------

/// concat_strings(a, b).len() == a.len() + b.len() — длина результата.
#[test]
fn prop_concat_strings_length() {
    let mut rng: u64 = 0x0F0F_0F0F_0F0F_0F0F;
    // Алфавит из ASCII-символов для простоты.
    let alphabet: Vec<char> = ('a'..='z').collect();
    for _ in 0..500 {
        let len_a = in_range(&mut rng, 0, 20) as usize;
        let len_b = in_range(&mut rng, 0, 20) as usize;
        let a: String = (0..len_a)
            .map(|_| {
                let idx = next_u64(&mut rng) as usize % alphabet.len();
                alphabet[idx]
            })
            .collect();
        let b: String = (0..len_b)
            .map(|_| {
                let idx = next_u64(&mut rng) as usize % alphabet.len();
                alphabet[idx]
            })
            .collect();
        let expected_len = a.len() + b.len();
        let result = concat_strings(a, b);
        assert_eq!(result.len(), expected_len);
    }
}

/// concat_strings(a, b) начинается с a и заканчивается b.
#[test]
fn prop_concat_strings_starts_with_a_ends_with_b() {
    let mut rng: u64 = 0x1357_9BDF_2468_ACE0;
    let alphabet: Vec<char> = ('a'..='z').collect();
    for _ in 0..500 {
        let len_a = in_range(&mut rng, 1, 15) as usize;
        let len_b = in_range(&mut rng, 1, 15) as usize;
        let a: String = (0..len_a)
            .map(|_| {
                let idx = next_u64(&mut rng) as usize % alphabet.len();
                alphabet[idx]
            })
            .collect();
        let b: String = (0..len_b)
            .map(|_| {
                let idx = next_u64(&mut rng) as usize % alphabet.len();
                alphabet[idx]
            })
            .collect();
        let a_clone = a.clone();
        let b_clone = b.clone();
        let result = concat_strings(a, b);
        assert!(result.starts_with(&a_clone));
        assert!(result.ends_with(&b_clone));
    }
}

/// concat_strings(a, "") == a и concat_strings("", b) == b (нейтральный элемент).
#[test]
fn prop_concat_strings_empty_identity() {
    let mut rng: u64 = 0xFEDC_BA98_7654_3210;
    let alphabet: Vec<char> = ('a'..='z').collect();
    for _ in 0..300 {
        let len = in_range(&mut rng, 0, 30) as usize;
        let s: String = (0..len)
            .map(|_| {
                let idx = next_u64(&mut rng) as usize % alphabet.len();
                alphabet[idx]
            })
            .collect();
        // concat_strings(s, "") == s
        assert_eq!(concat_strings(s.clone(), String::new()), s);
        // concat_strings("", s) == s
        assert_eq!(concat_strings(String::new(), s.clone()), s);
    }
}

/// concat_strings(a, b) == a + &b (oracle: стандартная конкатенация через format!).
#[test]
fn prop_concat_strings_matches_format_oracle() {
    let mut rng: u64 = 0x9999_8888_7777_6666;
    let alphabet: Vec<char> = ('a'..='z').collect();
    for _ in 0..500 {
        let len_a = in_range(&mut rng, 0, 15) as usize;
        let len_b = in_range(&mut rng, 0, 15) as usize;
        let a: String = (0..len_a)
            .map(|_| {
                let idx = next_u64(&mut rng) as usize % alphabet.len();
                alphabet[idx]
            })
            .collect();
        let b: String = (0..len_b)
            .map(|_| {
                let idx = next_u64(&mut rng) as usize % alphabet.len();
                alphabet[idx]
            })
            .collect();
        let expected = format!("{}{}", a, b);
        let result = concat_strings(a, b);
        assert_eq!(result, expected);
    }
}

// ---------- repeat_string properties ----------

/// repeat_string(s, n).len() == s.len() * n.
#[test]
fn prop_repeat_string_length() {
    let mut rng: u64 = 0x2B2B_2B2B_2B2B_2B2B;
    let alphabet: Vec<char> = ('a'..='z').collect();
    for _ in 0..500 {
        let len_s = in_range(&mut rng, 0, 20) as usize;
        let times = in_range(&mut rng, 0, 10) as usize;
        let s: String = (0..len_s)
            .map(|_| {
                let idx = next_u64(&mut rng) as usize % alphabet.len();
                alphabet[idx]
            })
            .collect();
        let expected_len = s.len() * times;
        let result = repeat_string(s, times);
        assert_eq!(result.len(), expected_len);
    }
}

/// repeat_string(s, 0) == "" для любой строки.
#[test]
fn prop_repeat_string_zero_is_empty() {
    let mut rng: u64 = 0xAAAA_BBBB_CCCC_DDDD;
    let alphabet: Vec<char> = ('a'..='z').collect();
    for _ in 0..300 {
        let len_s = in_range(&mut rng, 0, 30) as usize;
        let s: String = (0..len_s)
            .map(|_| {
                let idx = next_u64(&mut rng) as usize % alphabet.len();
                alphabet[idx]
            })
            .collect();
        assert_eq!(repeat_string(s, 0), "");
    }
}

/// repeat_string(s, 1) == s для любой строки.
#[test]
fn prop_repeat_string_once_is_identity() {
    let mut rng: u64 = 0x1111_2222_3333_4444;
    let alphabet: Vec<char> = ('a'..='z').collect();
    for _ in 0..300 {
        let len_s = in_range(&mut rng, 0, 30) as usize;
        let s: String = (0..len_s)
            .map(|_| {
                let idx = next_u64(&mut rng) as usize % alphabet.len();
                alphabet[idx]
            })
            .collect();
        assert_eq!(repeat_string(s.clone(), 1), s);
    }
}

/// repeat_string(s, n) совпадает с ручным повтором через цикл (оракул).
#[test]
fn prop_repeat_string_matches_manual_concat() {
    let mut rng: u64 = 0x5A5A_5A5A_A5A5_A5A5;
    let alphabet: Vec<char> = ('a'..='z').collect();
    for _ in 0..500 {
        let len_s = in_range(&mut rng, 0, 10) as usize;
        let times = in_range(&mut rng, 0, 8) as usize;
        let s: String = (0..len_s)
            .map(|_| {
                let idx = next_u64(&mut rng) as usize % alphabet.len();
                alphabet[idx]
            })
            .collect();
        // Оракул: ручной цикл
        let mut expected = String::new();
        for _ in 0..times {
            expected.push_str(&s);
        }
        let result = repeat_string(s, times);
        assert_eq!(result, expected);
    }
}

/// repeat_string(s, n+m) == repeat_string(s, n) + repeat_string(s, m) — дистрибутивность.
#[test]
fn prop_repeat_string_distributive() {
    let mut rng: u64 = 0x7E7E_7E7E_1E1E_1E1E;
    let alphabet: Vec<char> = ('a'..='z').collect();
    for _ in 0..500 {
        let len_s = in_range(&mut rng, 0, 10) as usize;
        let n = in_range(&mut rng, 0, 6) as usize;
        let m = in_range(&mut rng, 0, 6) as usize;
        let s: String = (0..len_s)
            .map(|_| {
                let idx = next_u64(&mut rng) as usize % alphabet.len();
                alphabet[idx]
            })
            .collect();
        let left = repeat_string(s.clone(), n + m);
        let right = format!("{}{}", repeat_string(s.clone(), n), repeat_string(s.clone(), m));
        assert_eq!(left, right);
    }
}

// ---------- clone_and_push properties ----------

/// clone_and_push: результат имеет длину v.len() + 1.
#[test]
fn prop_clone_and_push_length() {
    let mut rng: u64 = 0xF00D_CAFE_BEEF_DEAD;
    for _ in 0..500 {
        let len = in_range(&mut rng, 0, 30) as usize;
        let v: Vec<i64> = (0..len).map(|_| in_range(&mut rng, -5000, 5000)).collect();
        let x = in_range(&mut rng, -5000, 5000);
        let result = clone_and_push(&v, x);
        assert_eq!(result.len(), v.len() + 1);
    }
}

/// clone_and_push: последний элемент результата равен x.
#[test]
fn prop_clone_and_push_last_is_x() {
    let mut rng: u64 = 0x1234_5678_9ABC_DEF0;
    for _ in 0..500 {
        let len = in_range(&mut rng, 0, 30) as usize;
        let v: Vec<i64> = (0..len).map(|_| in_range(&mut rng, -5000, 5000)).collect();
        let x = in_range(&mut rng, -5000, 5000);
        let result = clone_and_push(&v, x);
        assert_eq!(*result.last().unwrap(), x);
    }
}

/// clone_and_push: первые v.len() элементов результата совпадают с v (round-trip).
#[test]
fn prop_clone_and_push_prefix_matches_original() {
    let mut rng: u64 = 0xC001_C0DE_FEED_FACE;
    for _ in 0..500 {
        let len = in_range(&mut rng, 0, 30) as usize;
        let v: Vec<i64> = (0..len).map(|_| in_range(&mut rng, -5000, 5000)).collect();
        let x = in_range(&mut rng, -5000, 5000);
        let result = clone_and_push(&v, x);
        assert_eq!(&result[..v.len()], v.as_slice());
    }
}

/// clone_and_push: оригинал НЕ изменяется после вызова.
#[test]
fn prop_clone_and_push_original_unchanged() {
    let mut rng: u64 = 0xBAD_CAB_FEED_BABE_u64;
    for _ in 0..500 {
        let len = in_range(&mut rng, 0, 30) as usize;
        let v: Vec<i64> = (0..len).map(|_| in_range(&mut rng, -5000, 5000)).collect();
        let x = in_range(&mut rng, -5000, 5000);
        let v_snapshot = v.clone();
        let _result = clone_and_push(&v, x);
        // оригинал не изменился
        assert_eq!(v, v_snapshot);
    }
}

/// clone_and_push: sum(result) == sum(v) + x (сумма сохраняется плюс новый элемент).
#[test]
fn prop_clone_and_push_sum_invariant() {
    let mut rng: u64 = 0xEEEE_FFFF_AAAA_BBBB;
    for _ in 0..500 {
        let len = in_range(&mut rng, 0, 20) as usize;
        // Держим диапазон скромным, чтобы сумма не переполнила i64
        let v: Vec<i64> = (0..len).map(|_| in_range(&mut rng, -100, 100)).collect();
        let x = in_range(&mut rng, -100, 100);
        let sum_v: i64 = v.iter().sum();
        let result = clone_and_push(&v, x);
        let sum_result: i64 = result.iter().sum();
        assert_eq!(sum_result, sum_v + x);
    }
}
