// Интеграционные тесты для ValueStore (модуль 03-ownership).
// Файл разбит на две части:
//   (a) детерминированные примеры — конкретный ввод → конкретный вывод;
//   (b) рандомизированные property-тесты — инварианты / round-trip / оракул.
//
// Используем только публичное API: new, push, pop, drain_into, len, is_empty.
// ГПСЧ взят из props.rs (тот же xorshift64* с фиксированным сидом).

use m03_ownership::ValueStore;

// ---------------------------------------------------------------------------
// Вспомогательный ГПСЧ (xorshift64*, без внешних крейтов, воспроизводимо)
// ---------------------------------------------------------------------------

fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}

/// Случайный `usize` в диапазоне `[0, max]` включительно.
fn rand_usize(state: &mut u64, max: usize) -> usize {
    (next_u64(state) as usize) % (max + 1)
}

/// Строка из `len` случайных строчных ASCII-букв.
fn rand_string(state: &mut u64, len: usize) -> String {
    (0..len)
        .map(|_| {
            let idx = next_u64(state) as usize % 26;
            (b'a' + idx as u8) as char
        })
        .collect()
}

// ===========================================================================
// (a) Детерминированные тесты
// ===========================================================================

// --- new / is_empty / len ---------------------------------------------------

#[test]
fn new_store_is_empty() {
    let store = ValueStore::new();
    assert!(store.is_empty());
    assert_eq!(store.len(), 0);
}

// --- push / len / is_empty --------------------------------------------------

#[test]
fn push_one_increases_len() {
    let mut store = ValueStore::new();
    store.push(String::from("hello"));
    assert_eq!(store.len(), 1);
    assert!(!store.is_empty());
}

#[test]
fn push_many_increases_len_correctly() {
    let mut store = ValueStore::new();
    for i in 0..10usize {
        store.push(format!("item-{i}"));
        assert_eq!(store.len(), i + 1);
    }
}

// --- pop edge cases ---------------------------------------------------------

#[test]
fn pop_from_empty_returns_none() {
    let mut store = ValueStore::new();
    assert_eq!(store.pop(), None);
}

#[test]
fn pop_still_returns_none_repeatedly_on_empty() {
    let mut store = ValueStore::new();
    for _ in 0..5 {
        assert_eq!(store.pop(), None);
    }
    assert_eq!(store.len(), 0);
    assert!(store.is_empty());
}

// --- LIFO round-trip --------------------------------------------------------

#[test]
fn lifo_single_element() {
    let mut store = ValueStore::new();
    store.push(String::from("only"));
    assert_eq!(store.pop(), Some(String::from("only")));
    assert_eq!(store.pop(), None);
    assert!(store.is_empty());
}

#[test]
fn lifo_order_three_elements() {
    let mut store = ValueStore::new();
    store.push(String::from("first"));
    store.push(String::from("second"));
    store.push(String::from("third"));
    // LIFO: last in, first out
    assert_eq!(store.pop(), Some(String::from("third")));
    assert_eq!(store.pop(), Some(String::from("second")));
    assert_eq!(store.pop(), Some(String::from("first")));
    assert_eq!(store.pop(), None);
}

#[test]
fn push_pop_interleaved() {
    let mut store = ValueStore::new();
    store.push(String::from("a"));
    store.push(String::from("b"));
    assert_eq!(store.pop(), Some(String::from("b")));
    store.push(String::from("c"));
    // стек сейчас: ["a", "c"]  — top = "c"
    assert_eq!(store.pop(), Some(String::from("c")));
    assert_eq!(store.pop(), Some(String::from("a")));
    assert_eq!(store.pop(), None);
}

#[test]
fn len_decreases_after_pop() {
    let mut store = ValueStore::new();
    store.push(String::from("x"));
    store.push(String::from("y"));
    assert_eq!(store.len(), 2);
    store.pop();
    assert_eq!(store.len(), 1);
    store.pop();
    assert_eq!(store.len(), 0);
    assert!(store.is_empty());
}

// --- drain_into -------------------------------------------------------------

#[test]
fn drain_into_empty_returns_empty_vec() {
    let store = ValueStore::new();
    let v = store.drain_into();
    assert!(v.is_empty());
}

#[test]
fn drain_into_single_element() {
    let mut store = ValueStore::new();
    store.push(String::from("solo"));
    let v = store.drain_into();
    assert_eq!(v, vec![String::from("solo")]);
}

#[test]
fn drain_into_preserves_push_order() {
    let mut store = ValueStore::new();
    store.push(String::from("alpha"));
    store.push(String::from("beta"));
    store.push(String::from("gamma"));
    let v = store.drain_into();
    assert_eq!(
        v,
        vec![
            String::from("alpha"),
            String::from("beta"),
            String::from("gamma"),
        ]
    );
}

#[test]
fn drain_into_yields_all_elements() {
    let inputs: Vec<String> = (0..20).map(|i| format!("s{i}")).collect();
    let mut store = ValueStore::new();
    for s in &inputs {
        store.push(s.clone());
    }
    let v = store.drain_into();
    assert_eq!(v.len(), 20);
    assert_eq!(v, inputs);
}

// --- partial pop then drain_into -------------------------------------------

#[test]
fn partial_pop_then_drain_into() {
    let mut store = ValueStore::new();
    for i in 0..5 {
        store.push(format!("{i}"));
    }
    // pop два раза — останутся "0", "1", "2"
    assert_eq!(store.pop(), Some(String::from("4")));
    assert_eq!(store.pop(), Some(String::from("3")));
    let v = store.drain_into();
    assert_eq!(v, vec!["0", "1", "2"]);
}

// ===========================================================================
// (b) Рандомизированные property-тесты
// ===========================================================================

/// После N push-ей len() == N.
#[test]
fn prop_len_equals_push_count() {
    let mut rng: u64 = 0xDEAD_C0DE_1234_ABCD;
    for _ in 0..500 {
        let n = rand_usize(&mut rng, 50);
        let mut store = ValueStore::new();
        for i in 0..n {
            store.push(format!("item{i}"));
            assert_eq!(store.len(), i + 1);
        }
        assert_eq!(store.len(), n);
        if n == 0 {
            assert!(store.is_empty());
        } else {
            assert!(!store.is_empty());
        }
    }
}

/// is_empty() <=> len() == 0 — оба метода согласованы.
#[test]
fn prop_is_empty_consistent_with_len() {
    let mut rng: u64 = 0x0F0F_F0F0_ABAB_BABA;
    for _ in 0..500 {
        let n = rand_usize(&mut rng, 40);
        let mut store = ValueStore::new();
        for i in 0..n {
            store.push(format!("{i}"));
        }
        assert_eq!(store.is_empty(), store.len() == 0);
    }
}

/// push N строк, pop все N → получаем строки в обратном порядке.
#[test]
fn prop_lifo_round_trip() {
    let mut rng: u64 = 0xCAFE_BABE_FEED_FACE;
    for _ in 0..500 {
        let n = rand_usize(&mut rng, 30);
        let mut strings: Vec<String> = (0..n)
            .map(|_| {
                let len = rand_usize(&mut rng, 12);
                rand_string(&mut rng, len)
            })
            .collect();

        let mut store = ValueStore::new();
        for s in &strings {
            store.push(s.clone());
        }
        // pop даёт элементы в обратном порядке
        strings.reverse();
        for expected in &strings {
            let got = store.pop();
            assert_eq!(got.as_deref(), Some(expected.as_str()));
        }
        // после извлечения всех — пусто
        assert_eq!(store.pop(), None);
        assert!(store.is_empty());
    }
}

/// drain_into возвращает ровно те строки, что были push-нуты, в том же порядке.
#[test]
fn prop_drain_into_matches_push_order() {
    let mut rng: u64 = 0x1122_3344_5566_7788;
    for _ in 0..500 {
        let n = rand_usize(&mut rng, 40);
        let strings: Vec<String> = (0..n)
            .map(|_| {
                let len = rand_usize(&mut rng, 10);
                rand_string(&mut rng, len)
            })
            .collect();

        let mut store = ValueStore::new();
        for s in &strings {
            store.push(s.clone());
        }
        let result = store.drain_into();
        assert_eq!(result, strings);
    }
}

/// drain_into возвращает ровно len() элементов.
#[test]
fn prop_drain_into_len_matches_store_len() {
    let mut rng: u64 = 0xABCD_EF01_9876_5432;
    for _ in 0..500 {
        let n = rand_usize(&mut rng, 40);
        let mut store = ValueStore::new();
        for i in 0..n {
            store.push(format!("x{i}"));
        }
        let expected_len = store.len();
        let result = store.drain_into();
        assert_eq!(result.len(), expected_len);
    }
}

/// Инвариант: len() падает ровно на 1 после каждого успешного pop.
#[test]
fn prop_pop_decrements_len_by_one() {
    let mut rng: u64 = 0x7777_8888_9999_AAAA;
    for _ in 0..500 {
        let n = rand_usize(&mut rng, 40);
        let mut store = ValueStore::new();
        for i in 0..n {
            store.push(format!("{i}"));
        }
        let mut expected_len = n;
        while expected_len > 0 {
            let before = store.len();
            let got = store.pop();
            assert!(got.is_some());
            assert_eq!(store.len(), before - 1);
            expected_len -= 1;
        }
        assert_eq!(store.pop(), None);
        assert_eq!(store.len(), 0);
    }
}

/// Оракул: push/pop/drain_into ведут себя как Vec<String> в роли стека.
/// Сравниваем ValueStore с наивным Vec-стеком операция за операцией.
#[test]
fn prop_oracle_vs_naive_vec_stack() {
    let mut rng: u64 = 0xFEED_FACE_C0FF_EE00;
    for _ in 0..300 {
        let ops = rand_usize(&mut rng, 50) + 1; // хотя бы одна операция
        let mut store = ValueStore::new();
        let mut oracle: Vec<String> = Vec::new();

        for _ in 0..ops {
            // 0..2 = push, 2..4 = pop, 4 = drain (делаем drain редко)
            let choice = next_u64(&mut rng) % 5;
            match choice {
                0..=1 => {
                    let len = rand_usize(&mut rng, 8);
                    let s = rand_string(&mut rng, len);
                    oracle.push(s.clone());
                    store.push(s);
                }
                2..=3 => {
                    let store_got = store.pop();
                    let oracle_got = oracle.pop();
                    assert_eq!(store_got, oracle_got);
                }
                _ => {
                    // drain — завершаем итерацию
                    let result = store.drain_into();
                    assert_eq!(result, oracle);
                    // сбрасываем оракул и начинаем следующий раунд с нового store
                    oracle.clear();
                    store = ValueStore::new();
                }
            }
            // проверяем len и is_empty согласованы в любой момент
            assert_eq!(store.len(), oracle.len());
            assert_eq!(store.is_empty(), oracle.is_empty());
        }
    }
}
